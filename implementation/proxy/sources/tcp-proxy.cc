/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Shih-Hao Tseng (st688@cornell.edu)
 */
#include <stdio.h>
#include <sys/types.h>

#include "tcp-proxy.h"
#include "CodedBulk-flow-identifier.h"
#include "system_parameters.h"

extern SystemParameters system_parameters;

TcpSendPeer::TcpSendPeer (TcpProxy* proxy, uint32_t path_id, bool address_is_ipc) :
  m_proxy(proxy),
  m_path_id(path_id),
  m_assigned_local(false),
  m_address_is_ipc(address_is_ipc),
  m_socket(nullptr),
  m_connected(false)
{
  m_buffer.clear();
}

TcpSendPeer::~TcpSendPeer ()
{
  if(m_connect_peer_thread.joinable())
    m_connect_peer_thread.join();
  DisconnectPeer();
  if(m_socket != nullptr) {
    delete m_socket;
    m_socket = nullptr;
  }

  for(std::list<Packet*>::iterator
    it  = m_buffer.begin();
    it != m_buffer.end();
    ++it
  ){
    (*it)->Delete();
  }
  m_buffer.clear();
}

void
TcpSendPeer::ConnectPeer (void)
{
  m_connect_peer_thread = std::thread(&TcpSendPeer::ConnectPeerThread, this);
}

void
TcpSendPeer::ConnectPeerThread (void)
{
  m_connected = false;
  while(system_parameters.isRunning() && !m_connected)
    {
      if (m_socket != nullptr) {
        m_socket->Close();
        delete m_socket;
        m_socket = nullptr;
      }
      m_proxy->m_socket_creation.lock();
      if(m_address_is_ipc) {
        m_socket = 
          Socket::CreateIPCSocket (
            AddressToIPCAddress(m_peer),
            m_proxy->m_receiver_port[m_path_id]
          );
      } else {
        while (system_parameters.isRunning() && (m_socket == nullptr)) {
          m_socket =
            Socket::CreateSocket (
              m_peer,
              PROXY_PORT
            );
        }
        if(m_assigned_local) {
          if( m_socket->BindLocal (
                m_local,
                3*PROXY_PORT+m_path_id
              ) == -1
          ) {
            perror ("Bound local fails");
            return;
          }
        }
      }
      m_proxy->m_socket_creation.unlock();
      m_socket->SetIpTos (6 << 4);
      while (system_parameters.isRunning()) {
        if (m_socket->Connect () >= 0)
          {
            m_socket->SetIpTos (m_priority << 4);
            m_connected = true;
            break;
          }
        //else
        //  {
        //    // retry
        //    perror ("Failed to connect socket");
        //    return;
        //  }
      }
    }
  if(m_connected) {
    PeerEstablished ();
  }
}

void
TcpSendPeer::DisconnectPeer (void)
{
  if(m_socket != nullptr)
    {
      m_socket->Close ();
      m_connected = false;
    }
}

void
TcpSendPeer::SetLocal (const Address& local)
{
  m_local = local;
  m_assigned_local = true;
}

void
TcpSendPeer::SetRemote (const Address& peer)
{
  m_peer = peer;
}

void
TcpSendPeer::SetPriority (const uint8_t& priority)
{
  m_priority = priority;
}

void
TcpSendPeer::PeerEstablished (void)
{
  Packet packet;
  CodedBulkFlowIdentifier mpls_flow_identifier (m_path_id);
  packet.AddHeader(mpls_flow_identifier);
  int ret = -1;
  while (system_parameters.isRunning() && (ret < 0)) {
    ret = m_socket->Send(&packet);
  }
  PeerReady ();
}

void
TcpSendPeer::PeerReady (void)
{
  while(system_parameters.isRunning()){
    if (m_socket->CheckPollEvent(POLLOUT)) {
      // tx buffer ready
      m_proxy->ProxySendLogic(this);
    } else {
      m_socket->PollEvent(POLLOUT);
    }
  }
}

TcpReceivePeer::TcpReceivePeer (TcpProxy* proxy, uint32_t path_id) :
  m_proxy(proxy),
  m_path_id(path_id),
  m_socket(nullptr)
{
}

TcpReceivePeer::~TcpReceivePeer ()
{
  DisconnectPeer ();
  if (m_socket != nullptr) {
    delete m_socket;
    m_socket = nullptr;
  }
}

void
TcpReceivePeer::DisconnectPeer (void)
{
  if(m_socket != nullptr)
    {
      m_socket->Close ();
    }
}

void
TcpReceivePeer::HandleRead (void)
{
  std::unordered_map<uint32_t, uint32_t>::iterator it = m_proxy->m_forward_paths.find(m_path_id);
  if(it == m_proxy->m_forward_paths.end()) {
    m_forward = false;
    m_forward_peer = nullptr;
    //m_socket->SetIpTos(4 << 4);
  } else {
    m_forward = true;
    m_forward_peer = m_proxy->GetSendPeer(it->second);
    //m_socket->SetIpTos(2 << 4);
  }
  while(system_parameters.isRunning()){
    if (m_socket->CheckPollEvent(POLLIN)) {
      // rx buffer ready
      m_proxy->ProxyRecvLogic(this);
    } else {
      m_socket->PollEvent(POLLIN);
    }
  }
}

TcpListenPeer::TcpListenPeer (TcpProxy* proxy, bool address_is_ipc) :
  m_proxy(proxy),
  m_listening_socket(nullptr),
  m_address_is_ipc(address_is_ipc)
{
  m_waiting_for_flow_identifier_threads.clear();
}

TcpListenPeer::TcpListenPeer (TcpProxy* proxy, const Address& local, bool address_is_ipc) :
  TcpListenPeer(proxy,address_is_ipc)
{
  SetLocal(local);
  m_waiting_for_flow_identifier_threads.clear();
}

TcpListenPeer::TcpListenPeer (const TcpListenPeer& copy_peer)
{
  m_proxy = copy_peer.m_proxy;
  m_local = copy_peer.m_local;
  m_listening_socket = copy_peer.m_listening_socket;
  m_address_is_ipc = copy_peer.m_address_is_ipc;
  // let the copy_peer handles
  // m_waiting_for_flow_identifier_threads
  // when it is deleted
}

TcpListenPeer::~TcpListenPeer ()
{
  StopListening ();
  if (m_listening_socket != nullptr) {
    if(m_address_is_ipc) {
      m_listening_socket->UnlinkIPCSocket();
    }
    delete m_listening_socket;
    m_listening_socket = nullptr;
  }

  m_waiting_for_flow_identifier_lock.lock();
  for(auto& item : m_waiting_for_flow_identifier_threads) {
    if (item.second.joinable())
      item.second.join();
  }
  m_waiting_for_flow_identifier_threads.clear();
  m_waiting_for_flow_identifier_lock.unlock();
}

void
TcpListenPeer::ListenLocal (void)
{
  if (m_listening_socket == nullptr)
    {
      m_proxy->m_socket_creation.lock();
      if(m_address_is_ipc) {
        m_listening_socket = 
          Socket::CreateIPCSocket (
            AddressToIPCAddress(m_local),
            PROXY_PORT
          );
      } else {
        while (system_parameters.isRunning() && (m_listening_socket == nullptr)) {
          m_listening_socket =
            Socket::CreateSocket (
              m_local,
              PROXY_PORT
            );
        }
      }
      m_proxy->m_socket_creation.unlock();
      m_listening_socket->SetIpTos (6 << 4);
      if (m_listening_socket->Bind () < 0)
        {
          perror ("Failed to bind socket");
          return;
        }
      if (m_listening_socket->Listen ())
        {
          perror ("Failed to listen");
          return;
        }
    }
  system_parameters.registerListeningSocket(m_listening_socket);
  Socket* accepted_socket = nullptr;
  while(system_parameters.isRunning()){
    accepted_socket = m_listening_socket->Accept ();
    if (accepted_socket != nullptr)
      {
        HandleAccept (accepted_socket);
      }
  }
}

void
TcpListenPeer::StopListening (void)
{
  if (m_listening_socket != nullptr) 
    {
      m_listening_socket->Close ();
    }
}

void
TcpListenPeer::SetLocal (const Address& local)
{
  m_local = local;
}

void
TcpListenPeer::HandleAccept (Socket* socket)
{
  socket->SetIpTos (6 << 4);
  m_waiting_for_flow_identifier_lock.lock();
  m_waiting_for_flow_identifier_threads[socket] = std::thread (&TcpListenPeer::WaitForFlowIdentifier, this, socket);
  m_waiting_for_flow_identifier_lock.unlock();
}

void
TcpListenPeer::WaitForFlowIdentifier (Socket* socket)
{
  // receive 4 bytes
  Packet packet;
  int recv_len = 0;
  bool flow_identifier_received = false;
  while (system_parameters.isRunning() && !flow_identifier_received) {
    recv_len += socket->Recv(&packet, 4);
    if(recv_len >= 4) {
      flow_identifier_received = true;
    }
  }
  CodedBulkFlowIdentifier mpls_flow_identifier;
  packet.RemoveHeader(mpls_flow_identifier);
  TcpReceivePeer* recv_peer = m_proxy->GetReceivePeer(mpls_flow_identifier.GetValue());
  recv_peer->m_socket = socket;

  //m_waiting_for_flow_identifier_lock.lock();
  //m_waiting_for_flow_identifier_threads.erase(socket);
  //m_waiting_for_flow_identifier_lock.unlock();

  recv_peer->HandleRead();
}

size_t TcpProxy::__data_segment_size = DATASEG_SIZE;

void
TcpProxy::SetDataSegSize (size_t size)
{
  // add the MPLS and IP header size
  __data_segment_size = size;
  if( __data_segment_size > Packet::GetMaxLen() )
  {
    __data_segment_size = Packet::GetMaxLen();
  }
}

size_t
TcpProxy::GetDataSegSize (void)
{
  return __data_segment_size;
}

TcpProxy::TcpProxy ()
{
  m_proxy_addr_map.clear();
  m_proxy_base_addr_map.clear();
  m_receiver_addresses.clear();

  m_sender_path_ids.clear();

  m_send_peers.clear();
  m_receive_peers.clear();

  m_listen_peer.clear();
  m_local_listen_peer.clear();

  m_listen_peer_thread.clear();
  m_local_listen_peer_thread.clear();

  m_forward_paths.clear();

  m_base_addr.clear();
}

TcpProxy::~TcpProxy () {
  for(std::unordered_map<uint32_t, TcpSendPeer*>::iterator
    it  = m_send_peers.begin();
    it != m_send_peers.end();
    ++it
  ) {
    delete it->second;
  }
  m_send_peers.clear();

  for(std::unordered_map<uint32_t, TcpReceivePeer*>::iterator
    it  = m_receive_peers.begin();
    it != m_receive_peers.end();
    ++it
  ) {
    delete it->second;
  }
  m_receive_peers.clear();

  for(int i = 0; i < m_listen_peer_thread.size(); ++i) {
    if (m_listen_peer_thread[i].joinable()) {
      m_listen_peer_thread[i].join();
    }
  }
  for(int i = 0; i < m_local_listen_peer_thread.size(); ++i) {
    if (m_local_listen_peer_thread[i].joinable()) {
      m_local_listen_peer_thread[i].join();
    }
  }
}

void
TcpProxy::SetForwardRule (uint32_t from_path_id, uint32_t to_path_id)
{
  m_forward_paths[from_path_id] = to_path_id;
}

void
TcpProxy::SetBaseAddr (const Address base_addr)
{
  // we don't check if the address has been duplicated 
  // as Address has no comparison operator.
  m_base_addr.emplace_back(base_addr);
  m_listen_peer      .emplace_back (this, base_addr, false);
  m_local_listen_peer.emplace_back (this, base_addr, true);
}

Address
TcpProxy::GetBaseAddr (uint32_t index)
{
  if(index < 0) {
    return Address();
  }
  if(index >= m_base_addr.size()) {
    return Address();
  }
  return m_base_addr[index];
}

TcpSendPeer*
TcpProxy::AddPeer (uint32_t path_id)
{
  // must check the peer does not exist first
  TcpSendPeer* sender = nullptr;
  if(m_proxy_addr_map.find(path_id) != m_proxy_addr_map.end()) {
    // output to another proxy
    sender = new TcpSendPeer (this, path_id);
    sender->SetRemote (m_proxy_addr_map[path_id]);
    sender->SetPriority (m_priority[path_id]);
    if(m_proxy_base_addr_map.find(path_id) != m_proxy_base_addr_map.end()) {
      sender->SetLocal(m_proxy_base_addr_map[path_id]);
    }
    //m_send_peers[path_id] = sender;
  } else if (m_receiver_addresses.find(path_id) != m_receiver_addresses.end()) {
    // output to application
    sender = new TcpSendPeer (this, path_id, true);
    sender->SetRemote (m_receiver_addresses[path_id]);
    sender->SetPriority (m_priority[path_id]);
    //m_send_peers[path_id] = sender;
  }
  return sender;
}

TcpReceivePeer*
TcpProxy::AddLocal (uint32_t path_id)
{
  TcpReceivePeer* receiver = new TcpReceivePeer (this, path_id);
  //m_receive_peers[path_id] = receiver;

  return receiver;
}

int
TcpProxy::DoSend (TcpSendPeer* send_peer)
{
  if(send_peer == nullptr) {
    return 0;
  }
  if( !send_peer->m_connected ) {
    return 0;
  }
  send_peer->m_buffer_lock.lock();
  if ( send_peer->m_buffer.empty() ) {
    send_peer->m_buffer_lock.unlock();
    return 0;
  }
  if( !send_peer->m_socket->CheckPollEvent(POLLOUT) ) {
    send_peer->m_buffer_lock.unlock();
    return 0;
  }

  Packet* packet = send_peer->m_buffer.front();

  int ret = send_peer->m_socket->Send(packet);
  if ( ret >= 0 ) {
    //CodedBulkFlowIdentifier mpls_hd(0);
    //packet->PeekHeader(mpls_hd);
    delete packet;
    send_peer->m_buffer.pop_front();
  }
  send_peer->m_buffer_lock.unlock();
  return ret;
} // TcpProxy::DoSend

int
TcpProxy::DoReceive (TcpReceivePeer* recv_peer)
{
  TcpSendPeer* send_peer = GetSendPeer (recv_peer->m_path_id);

  send_peer->m_buffer_lock.lock();
  if ( !send_peer->m_buffer.empty() ) {
    // sender busy
    send_peer->m_buffer_lock.unlock();
    return 0;
  }
  send_peer->m_buffer_lock.unlock();

  if( !recv_peer->m_socket->CheckPollEvent(POLLIN) ) {
    return 0;
  }
  Packet* packet = new Packet ();
  int recv_len = recv_peer->m_socket->Recv(packet, GetDataSegSize ());
  if( recv_len < 0 ) {
    delete packet;
    packet = nullptr;
    return 0;
  }
  if (packet->GetSize () == 0)
  { //EOF
    delete packet;
    packet = nullptr;
    return 0;
  }

  int ret = packet->GetSize ();

  send_peer->m_buffer_lock.lock();
  send_peer->m_buffer.push_back(packet);
  send_peer->m_buffer_lock.unlock();
  return ret;
} // TcpProxy::DoReceive

void
TcpProxy::ProxySendLogic (TcpSendPeer*    send_peer)
{
  // simplest proxy logic: forwarding
  DoSend (send_peer);
}

void
TcpProxy::ProxyRecvLogic (TcpReceivePeer* recv_peer)
{
  // simplest proxy logic: forwarding
  DoReceive (recv_peer);
}

void*
TcpProxy::GetSendInfo (uint32_t path_id)
{
  return nullptr;
}

void*
TcpProxy::GetRecvInfo (uint32_t path_id, bool use_large_buffer)
{
  return nullptr;
}

void
TcpProxy::NewSendPeer (uint32_t path_id, void* send_peer)
{
  return;
}

void
TcpProxy::NewRecvPeer (uint32_t path_id, void* recv_peer)
{
  return;
}

void
TcpProxy::RegisterProxyAddr (uint32_t path_id, Address addr)
{
  m_proxy_addr_map[path_id] = addr;
}

void
TcpProxy::RegisterProxyAddr (uint32_t path_id, Address src_addr, Address dst_addr)
{
  m_proxy_base_addr_map[path_id] = src_addr;
  m_proxy_addr_map[path_id] = dst_addr;
}

void
TcpProxy::RegisterSender   (uint32_t path_id)
{
  // to differentiate the network links and the link between proxy and application
  m_sender_path_ids.insert(path_id);
}

void
TcpProxy::RegisterReceiver (uint32_t path_id, Address addr, uint16_t port) 
{
  m_receiver_addresses[path_id] = addr;
  m_receiver_port     [path_id] = port;
}

void
TcpProxy::RegisterPriority (uint32_t path_id, uint8_t priority)
{
  m_priority[path_id] = priority;
}

void
TcpProxy::StartProxy (void)
{
  StartApplication ();
}

void
TcpProxy::StopProxy (void)
{
  StopApplication ();
}

void
TcpProxy::StartApplication (void)
{
  for(auto& peer : m_listen_peer) {
    m_listen_peer_thread.emplace_back(&TcpListenPeer::ListenLocal, &peer);
  }
  for(auto& peer : m_local_listen_peer) {
    m_local_listen_peer_thread.emplace_back(&TcpListenPeer::ListenLocal, &peer);
  }
}

void
TcpProxy::StopApplication (void)
{
  for(auto& peer : m_listen_peer) {
    peer.StopListening();
  }
  for(auto& peer : m_local_listen_peer) {
    peer.StopListening();
  }
  for(std::unordered_map<uint32_t,TcpSendPeer*>::iterator
    it  = m_send_peers.begin();
    it != m_send_peers.end();
    ++it)
  {
    it->second->DisconnectPeer();
  }
  for(std::unordered_map<uint32_t,TcpReceivePeer*>::iterator
    it  = m_receive_peers.begin();
    it != m_receive_peers.end();
    ++it)
  {
    it->second->DisconnectPeer();
  }
}

TcpSendPeer*
TcpProxy::GetSendPeer    (uint32_t path_id)
{
  TcpSendPeer* send_peer = nullptr;
  m_send_peer_lock.lock ();
/*
  std::unordered_map<uint32_t, TcpSendPeer*>::iterator it = m_send_peers.find(path_id);
  if(it == m_send_peers.end()) {
    send_peer = AddPeer(path_id);
    m_send_peers[path_id] = send_peer;
    m_send_peer_lock.unlock ();
    if(send_peer == nullptr) {
      return nullptr;
    }
    send_peer->ConnectPeer();
    return send_peer;
  }
  send_peer = it->second;
*/
  std::pair<std::unordered_map<uint32_t, TcpSendPeer*>::iterator, bool> result = m_send_peers.insert(std::pair<uint32_t, TcpSendPeer*>(path_id,nullptr));
  if(result.second) {
    send_peer = AddPeer(path_id);
    result.first->second = send_peer;
    if(send_peer == nullptr) {
      m_send_peer_lock.unlock ();
      return nullptr;
    }
    send_peer->m_send_info = GetSendInfo(path_id);
    // no neighboring proxy/receiver at host exists
    NewSendPeer(path_id,send_peer);
    send_peer->ConnectPeer();
    m_send_peer_lock.unlock ();
    return send_peer;
  }
  send_peer = result.first->second;
  m_send_peer_lock.unlock ();

  return send_peer;
}

TcpReceivePeer*
TcpProxy::GetReceivePeer (uint32_t path_id)
{
  TcpReceivePeer* recv_peer = nullptr;
  bool receive_from_network_link = (m_sender_path_ids.find(path_id) == m_sender_path_ids.end());

  m_receive_peer_lock.lock ();
/*
  std::unordered_map<uint32_t, TcpReceivePeer*>::iterator it = m_receive_peers.find(path_id);
  if(it == m_receive_peers.end()) {
    recv_peer = AddLocal(path_id);
    m_receive_peers[path_id] = recv_peer;
  } else {
    recv_peer = it->second;
  }
*/
  std::pair<std::unordered_map<uint32_t, TcpReceivePeer*>::iterator, bool> result = m_receive_peers.insert(std::pair<uint32_t, TcpReceivePeer*>(path_id,nullptr));
  if(result.second) {
    recv_peer = AddLocal(path_id);
    recv_peer->m_receive_from_network_link = receive_from_network_link;
    result.first->second = recv_peer;
    recv_peer->m_recv_info = GetRecvInfo(path_id, receive_from_network_link);
    NewRecvPeer(path_id,recv_peer);
  } else {
    recv_peer = result.first->second;
  }
  m_receive_peer_lock.unlock ();

  return recv_peer;
}