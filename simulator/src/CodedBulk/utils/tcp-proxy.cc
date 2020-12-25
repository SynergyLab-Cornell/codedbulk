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
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket.h"
#include "ns3/udp-socket-factory.h"

#include "ns3/simulator.h"

#include "tcp-proxy.h"
#include "CodedBulk-flow-identifier.h"
#include "ns3/CodedBulk-system-parameters.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpProxy");

NS_OBJECT_ENSURE_REGISTERED (TcpProxy);

TcpSendPeer::TcpSendPeer (TcpProxy* proxy, uint32_t path_id) :
  m_proxy(proxy),
  m_path_id(path_id),
  m_assigned_local(false),
  m_socket(NULL),
  m_connected(false)
{
  //NS_LOG_FUNCTION (this);
  m_buffer.clear();
}

TcpSendPeer::~TcpSendPeer ()
{
  //NS_LOG_FUNCTION (this);
  DisconnectPeer();
  m_socket = NULL;
  m_buffer.clear();
}

void
TcpSendPeer::ConnectPeer (const Ptr<Node> node)
{
  // Create the socket if not already
  NS_LOG_FUNCTION_NOARGS();
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (node, TcpSocketFactory::GetTypeId ());
      m_socket->SetIpTos (6 << 2);
      if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
        }
      else if (InetSocketAddress::IsMatchingType (m_peer) ||
               PacketSocketAddress::IsMatchingType (m_peer))
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
        }
      m_socket->Connect (m_peer);
      m_socket->SetAllowBroadcast (true);
      m_socket->ShutdownRecv ();

      m_socket->SetConnectCallback (
        MakeCallback (&TcpSendPeer::ConnectionSucceeded, this),
        MakeCallback (&TcpSendPeer::ConnectionFailed, this));

      m_socket->SetSendCallback (
        MakeCallback (&TcpSendPeer::PeerEstablished, this));
    }
}

void
TcpSendPeer::DisconnectPeer (void)
{
  if(m_socket != NULL)
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
TcpSendPeer::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void
TcpSendPeer::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void
TcpSendPeer::PeerEstablished (Ptr<Socket>, uint32_t)
{
  Ptr<Packet> packet = Create<Packet> ();
  CodedBulkFlowIdentifier mpls_flow_identifier (m_path_id);
  packet->AddHeader(mpls_flow_identifier);
  int ret = 0;
  while (ret <= 0) {
    ret = m_socket->Send(packet);
  }
  m_socket->SetIpTos (m_proxy->m_priority[m_path_id] << 2);
  m_socket->SetSendCallback (
    MakeCallback (&TcpSendPeer::PeerReady, this));
}

void
TcpSendPeer::PeerReady (Ptr<Socket>, uint32_t num)
{
  m_proxy->ProxySendLogic(this);
}

TcpReceivePeer::TcpReceivePeer (TcpProxy* proxy, uint32_t path_id) :
  m_proxy(proxy),
  m_path_id(path_id),
  m_socket(NULL)
{
  //NS_LOG_FUNCTION (this);
}

TcpReceivePeer::~TcpReceivePeer ()
{
  //NS_LOG_FUNCTION (this);
  DisconnectPeer ();
  m_socket = NULL;
}

void
TcpReceivePeer::DisconnectPeer (void)
{
  if(m_socket != NULL)
    {
      m_socket->Close ();
    }
}

void
TcpReceivePeer::HandleRead (Ptr<Socket> socket)
{
  //NS_LOG_FUNCTION (this << socket);
  m_proxy->ProxyRecvLogic(this);
}

TcpListenPeer::TcpListenPeer (TcpProxy* proxy) :
  m_proxy(proxy),
  m_listening_socket(NULL)
{
  //NS_LOG_FUNCTION (this);
}

TcpListenPeer::TcpListenPeer (TcpProxy* proxy, const Address& local) :
  TcpListenPeer(proxy)
{
  //NS_LOG_FUNCTION (this);
  SetLocal(local);
}

TcpListenPeer::~TcpListenPeer ()
{
  //NS_LOG_FUNCTION (this);
  StopListening ();
  m_listening_socket = NULL;
}

void
TcpListenPeer::ListenLocal (const Ptr<Node> node)
{
  if (!m_listening_socket)
    {
      m_listening_socket = Socket::CreateSocket (node, TcpSocketFactory::GetTypeId ());
      m_listening_socket->SetIpTos (6 << 2);
      if (m_listening_socket->Bind (m_local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_listening_socket->Listen ();
      m_listening_socket->ShutdownSend ();
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_listening_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }
  // receive callback is set when accepted
  m_listening_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&TcpListenPeer::HandleAccept, this));
  m_listening_socket->SetCloseCallbacks (
    MakeCallback (&TcpListenPeer::HandlePeerClose, this),
    MakeCallback (&TcpListenPeer::HandlePeerError, this));
}

void
TcpListenPeer::StopListening (void)
{
  if (m_listening_socket) 
    {
      m_listening_socket->Close ();
      m_listening_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void
TcpListenPeer::SetLocal (const Address& local)
{
  m_local = local;
}

void
TcpListenPeer::HandleAccept (Ptr<Socket> socket, const Address& from)
{
  //NS_LOG_FUNCTION (this << socket << from);
  socket->SetIpTos(6 << 2);
  socket->SetRecvCallback (MakeCallback (&TcpListenPeer::WaitForFlowIdentifier, this));
}

void
TcpListenPeer::WaitForFlowIdentifier (Ptr<Socket> socket) const
{
  //NS_LOG_DEBUG ("path " << m_path_id << " is established at node " << m_proxy->GetNode()->GetId());
  Address from;
  // receive 4 bytes
  Ptr<Packet> packet = NULL;
  bool flow_identifier_received = false;
  while (!flow_identifier_received) {
    packet = socket->RecvFrom (4,0,from);
    if(packet != NULL) {
      if(packet->GetSize() >= 4) {
        flow_identifier_received = true;
      }
    }
  }
  CodedBulkFlowIdentifier mpls_flow_identifier;
  packet->PeekHeader(mpls_flow_identifier);
  TcpReceivePeer* recv_peer = m_proxy->GetReceivePeer(mpls_flow_identifier.GetValue());
  Ptr<TcpSocketBase> socket_base = DynamicCast<TcpSocketBase> (socket);
  socket_base->SetSlowAck();

  recv_peer->m_socket = socket;

  std::unordered_map<uint32_t, uint32_t>::iterator it = m_proxy->m_forward_paths.find(recv_peer->m_path_id);
  if(it == m_proxy->m_forward_paths.end()) {
    recv_peer->m_forward = false;
    recv_peer->m_forward_peer = NULL;
    //recv_peer->m_socket->SetIpTos(4 << 2);
  } else {
    recv_peer->m_forward = true;
    recv_peer->m_forward_peer = m_proxy->GetSendPeer(it->second);
    //recv_peer->m_socket->SetIpTos(2 << 2);
  }

  socket->SetRecvCallback (MakeCallback (&TcpReceivePeer::HandleRead, recv_peer));
}

void
TcpListenPeer::HandlePeerClose (Ptr<Socket> socket)
{
  //NS_LOG_FUNCTION (this << socket);
}

void
TcpListenPeer::HandlePeerError (Ptr<Socket> socket)
{
  //NS_LOG_FUNCTION (this << socket);
}

uint32_t TcpProxy::__data_segment_size = DATASEG_SIZE;
Time     TcpProxy::__max_rto = Seconds(60.0);

TypeId 
TcpProxy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpProxy")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<TcpProxy> ()
  ;
  return tid;
} // TcpProxy::GetTypeId

void
TcpProxy::SetDataSegSize (uint32_t size)
{
  // add the MPLS and IP header size
  __data_segment_size = size;
}

uint32_t
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

  m_forward_paths.clear();

  m_base_addr.clear();

  SetStartTime(Seconds(0));
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
}

void
TcpProxy::SetForwardRule (uint32_t from_path_id, uint32_t to_path_id)
{
  m_forward_paths[from_path_id] = to_path_id;
}

void
TcpProxy::ListForwardRule (std::ostream& os) const
{
  for(std::unordered_map<uint32_t, uint32_t>::const_iterator
    it  = m_forward_paths.begin();
    it != m_forward_paths.end();
    ++it
  ) {
    os << "forward: "  << std::dec << it->first << " -> " << it->second << std::endl;
  }
}

void
TcpProxy::SetBaseAddr (const Ipv4Address base_addr)
{
  m_base_addr.emplace_back(base_addr);
  m_listen_peer.emplace_back(this,Address(InetSocketAddress (base_addr, PROXY_PORT)));
}

Ipv4Address
TcpProxy::GetBaseAddr (uint32_t index)
{
  if(index < 0) {
    return Ipv4Address();
  }
  if(index >= m_base_addr.size()) {
    return Ipv4Address();
  }
  return m_base_addr[index];
}

TcpSendPeer*
TcpProxy::AddPeer (uint32_t path_id)
{
  // must check the peer does not exist first
  NS_LOG_DEBUG ("peer " << path_id << " at node " << GetNode()->GetId());
  TcpSendPeer* sender = NULL;
  if(m_proxy_addr_map.find(path_id) != m_proxy_addr_map.end()) {
    NS_LOG_DEBUG ("found in proxy map");
    // output to another proxy
    sender = new TcpSendPeer (this, path_id);
    sender->SetRemote (Address(InetSocketAddress (m_proxy_addr_map[path_id], PROXY_PORT)));
    sender->SetPriority (m_priority[path_id]);
    if(m_proxy_base_addr_map.find(path_id) != m_proxy_base_addr_map.end()) {
      sender->SetLocal(Address(InetSocketAddress (m_proxy_base_addr_map[path_id], path_id)));
    }
    //m_send_peers[path_id] = sender;
  } else if(m_receiver_addresses.find(path_id) != m_receiver_addresses.end()) {
    NS_LOG_DEBUG ("found in receiver addresses");
    // output to application
    sender = new TcpSendPeer (this, path_id);
    sender->SetRemote (Address(InetSocketAddress (m_receiver_addresses[path_id], m_receiver_port[path_id])));
    sender->SetPriority (m_priority[path_id]);
    //m_send_peers[path_id] = sender;
  }
  // no neighboring proxy/receiver at host exists
  NewSendPeer(path_id,sender);
  return sender;
}

TcpReceivePeer*
TcpProxy::AddLocal (uint32_t path_id)
{
  //NS_LOG_DEBUG ("local " << path_id << " at node " << GetNode()->GetId() << " addr = " << m_base_addr);
  TcpReceivePeer* receiver = new TcpReceivePeer (this, path_id);
  //m_receive_peers[path_id] = receiver;
  NewRecvPeer(path_id,receiver);

  return receiver;
}

int
TcpProxy::DoSend (TcpSendPeer* send_peer)
{
  if(send_peer == NULL) {
    return 0;
  }
  if( !send_peer->m_connected ) {
    //NS_LOG_DEBUG("sender for path id " << path_id << " is not connected at " << m_base_addr);
    return 0;
  }
  if ( send_peer->m_buffer.empty() ) {
    //NS_LOG_DEBUG("buffer for path id " << path_id << " is empty at " << m_base_addr);
    return 0;
  }

  Ptr<Packet> packet = send_peer->m_buffer.front();

  int ret = send_peer->m_socket->Send(packet);
  if ( ret > 0 ) {
    send_peer->m_buffer.pop_front();
  }
  return ret;
} // TcpProxy::DoSend

int
TcpProxy::DoReceive (TcpReceivePeer* recv_peer)
{
  TcpSendPeer* send_peer = GetSendPeer (recv_peer->m_path_id);

  if ( !send_peer->m_buffer.empty() ) {
    // sender busy
    return 0;
  }

  Address from;
  Ptr<Packet> packet = recv_peer->m_socket->RecvFrom (GetDataSegSize (),0,from);
  if (packet == NULL) {
    //NS_LOG_DEBUG("nothing to receive from path " << recv_peer->m_path_id << " at " << m_base_addr);
    return 0;
  }
  if (packet->GetSize () == 0)
  { //EOF
    //NS_LOG_DEBUG("receive to the end from path " << recv_peer->m_path_id << " at " << m_base_addr);
    return 0;
  }

  int ret = packet->GetSize ();

  send_peer->m_buffer.push_back(packet);

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
  return NULL;
}

void*
TcpProxy::GetRecvInfo (uint32_t path_id)
{
  return NULL;
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
TcpProxy::RegisterProxyAddr (uint32_t path_id, Ipv4Address addr)
{
  m_proxy_addr_map[path_id] = addr;
}

void
TcpProxy::RegisterProxyAddr (uint32_t path_id, Ipv4Address src_addr, Ipv4Address dst_addr)
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
TcpProxy::RegisterReceiver (uint32_t path_id, Ipv4Address addr, uint16_t port) 
{
  m_receiver_addresses[path_id] = addr;
  m_receiver_port     [path_id] = port;
}

void
TcpProxy::RegisterPriority (uint32_t path_id, uint8_t priority)
{
  m_priority[path_id] = priority;
}

uint64_t
TcpProxy::GetBufferedBytes (void) const
{
  uint64_t total_bytes = 0;
  for(std::unordered_map<uint32_t, TcpSendPeer* >::const_iterator
    it_peer  = m_send_peers.begin();
    it_peer != m_send_peers.end();
    ++it_peer
  ) {
    for(std::list<Ptr<Packet> >::const_iterator
      it_p  = it_peer->second->m_buffer.begin();
      it_p != it_peer->second->m_buffer.end();
      ++it_p
    ) {
      total_bytes += (*it_p)->GetSize();
    }
  }
  return total_bytes;
}

void
TcpProxy::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  for(auto& peer : m_listen_peer) {
    peer.ListenLocal(GetNode());
  }
}

void
TcpProxy::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  for(auto& peer : m_listen_peer) {
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
  TcpSendPeer* send_peer = NULL;
/*
  std::unordered_map<uint32_t, TcpSendPeer*>::iterator it = m_send_peers.find(path_id);
  if(it == m_send_peers.end()) {
    send_peer = AddPeer(path_id);
    m_send_peers[path_id] = send_peer;
    if(send_peer == NULL) {
      return NULL;
    }
    send_peer->ConnectPeer(GetNode());
    return send_peer;
  }
  send_peer = it->second;
*/
  std::pair<std::unordered_map<uint32_t, TcpSendPeer*>::iterator, bool> result = m_send_peers.insert(std::pair<uint32_t, TcpSendPeer*>(path_id,NULL));
  if(result.second) {
    send_peer = AddPeer(path_id);
    result.first->second = send_peer;
    if(send_peer == NULL) {
      return NULL;
    }
    send_peer->m_send_info = GetSendInfo(path_id);
    send_peer->ConnectPeer(GetNode());
    return send_peer;
  }
  send_peer = result.first->second;
  return send_peer;
}

TcpReceivePeer*
TcpProxy::GetReceivePeer (uint32_t path_id)
{
  TcpReceivePeer* recv_peer = NULL;
/*
  std::unordered_map<uint32_t, TcpReceivePeer*>::iterator it = m_receive_peers.find(path_id);
  if(it == m_receive_peers.end()) {
    recv_peer = AddLocal(path_id);
    m_receive_peers[path_id] = recv_peer;
  } else {
    recv_peer = it->second;
  }
*/
  bool receive_from_network_link = (m_sender_path_ids.find(path_id) == m_sender_path_ids.end());
  std::pair<std::unordered_map<uint32_t, TcpReceivePeer*>::iterator, bool> result = m_receive_peers.insert(std::pair<uint32_t, TcpReceivePeer*>(path_id,NULL));
  if(result.second) {
    recv_peer = AddLocal(path_id);
    recv_peer->m_receive_from_network_link = receive_from_network_link;
    result.first->second = recv_peer;
    recv_peer->m_recv_info = GetRecvInfo(path_id);
  } else {
    recv_peer = result.first->second;
  }
  return recv_peer;
}

}