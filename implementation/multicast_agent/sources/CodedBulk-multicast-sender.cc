/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Shih-Hao Tseng <st688@cornell.edu>
//
#include "test_tools.h"
#include "CodedBulk-flow-identifier.h"
#include "system_parameters.h"
#include "measure_tools.h"
#include "CodedBulk-multicast-sender.h"

#include <limits>

extern SystemParameters system_parameters;

CodedBulkMulticastSenderPath::CodedBulkMulticastSenderPath (Ptr<CodedBulkMulticastSenderMulticastPath> root) :
  m_root(root),
  m_path_id(0),
  m_socket(nullptr),
  m_connected(false),
  m_ready(true)
{
}

CodedBulkMulticastSenderPath::CodedBulkMulticastSenderPath (const CodedBulkMulticastSenderPath& path)
{
  m_root      = path.m_root; 
  m_path_id   = path.m_path_id;
  m_peer      = path.m_peer;
  m_socket    = path.m_socket;
  m_connected = path.m_connected;
  m_ready     = path.m_ready;
}

CodedBulkMulticastSenderPath::~CodedBulkMulticastSenderPath()
{
  if(m_connect_peer_thread.joinable())
    m_connect_peer_thread.join();
  DisconnectPeer();
  if(m_socket != nullptr) {
    if(m_root->m_application->m_through_proxy) {
      m_socket->UnlinkIPCSocket();
    }
    delete m_socket;
    m_socket = nullptr;
  }
}

void
CodedBulkMulticastSenderPath::ConnectPeer(void)
{
  m_connect_peer_thread = std::thread(&CodedBulkMulticastSenderPath::ConnectPeerThread, this);
}

void
CodedBulkMulticastSenderPath::ConnectPeerThread(void)
{
  m_connected = false;
  while(system_parameters.isRunning() && !m_connected)
    {
      if (m_socket != nullptr) {
        m_socket->Close();
        delete m_socket;
        m_socket = nullptr;
      }
      if(m_root->m_application->m_through_proxy) {
        m_socket =
          Socket::CreateIPCSocket (
            AddressToIPCAddress(m_peer),
            PROXY_PORT
          );
      } else {
        while( system_parameters.isRunning() && (m_socket == nullptr) ) {
          m_socket =
            Socket::CreateSocket (
              m_peer,
              INTERACTIVE_PORT+m_root->m_application->GetApplicationID()
            );
        }
      }
      m_socket->SetIpTos (6 << 4);
      m_connected_lock.lock();
      if (m_socket->Connect () < 0)
        {
          perror ("Failed to connect socket, retry");
        }
      else
        {
          m_connected = true;
        }
      m_connected_lock.unlock();
    }
  PathEstablished ();
}

void
CodedBulkMulticastSenderPath::DisconnectPeer (void)
{
  if(m_socket != nullptr)
    {
      m_socket->Close ();
      m_connected = false;
      m_ready     = false;
    }
}

void
CodedBulkMulticastSenderPath::SetRemote (const Address& peer)
{
  m_peer = peer;
}

void
CodedBulkMulticastSenderPath::SetPathID (const uint32_t path_id)
{
  m_path_id = path_id;
}

uint32_t
CodedBulkMulticastSenderPath::GetPathID (void) const
{
  return m_path_id;
}

Socket*
CodedBulkMulticastSenderPath::GetSocket (void) const
{
  return m_socket;
}

int
CodedBulkMulticastSenderPath::Send (Packet* packet)
{
  m_current_packet_ref._set_lock.lock();
  m_ready_lock.lock();
  if (!m_ready || (m_current_packet_ref._ref_packet != nullptr)) {
    m_ready_lock.unlock();
    m_current_packet_ref._set_lock.unlock();
    return -1;
  }
  m_ready = false;
  m_ready_lock.unlock();
  m_current_packet_ref.ReferToPacket(packet);
  int ret = m_socket->Send (&m_current_packet_ref);
  if( ret > 0 ){
    m_current_packet_ref.Dereference();
    m_ready_lock.lock();
    m_ready = true;
    m_ready_lock.unlock();
  }
  m_current_packet_ref._set_lock.unlock();
  return ret;
}

void
CodedBulkMulticastSenderPath::Resend (void)
{
  int ret = -1;
  m_current_packet_ref._set_lock.lock();
  if(m_current_packet_ref._ref_packet != nullptr) {
    ret = m_socket->Send (&m_current_packet_ref);
  } else {
    m_current_packet_ref._set_lock.unlock();
    return;
  }
  if(ret > 0) {
    m_ready_lock.lock();
    m_ready = true;
    m_ready_lock.unlock();
    m_current_packet_ref.Dereference();
    m_current_packet_ref._set_lock.unlock();
    m_root->DataSent(this);
  } else {
    m_current_packet_ref._set_lock.unlock();
  }
}

void
CodedBulkMulticastSenderPath::PathEstablished (void)
{
  Packet packet;
  CodedBulkFlowIdentifier mpls_flow_identifier (m_path_id);
  packet.AddHeader(mpls_flow_identifier);
  while (system_parameters.isRunning() && (m_socket->Send (&packet) < 0)) {}
  m_socket->SetIpTos (m_root->GetPriority () << 4);
  PathReady ();
}

void
CodedBulkMulticastSenderPath::PathReady (void)
{
  while(system_parameters.isRunning()){
    if (m_socket->CheckPollEvent(POLLOUT)) {
      m_current_packet_ref._set_lock.lock();
      if(m_current_packet_ref._ref_packet == nullptr) {
        m_ready_lock.lock();
        // tx buffer ready
        m_ready = true;
        m_ready_lock.unlock();
        m_current_packet_ref._set_lock.unlock();
        m_root->DataSent(this);
      } else {
        m_current_packet_ref._set_lock.unlock();
        SleepForOneMilliSec();
        Resend();
      }
    } else {
      m_socket->PollEvent(POLLOUT);
    }
  }
}

CodedBulkMulticastSenderMulticastPath::CodedBulkMulticastSenderMulticastPath (Ptr<CodedBulkMulticastSender> application) :
  m_connected(false),
  m_ready(false),
  m_application(application)
{
  m_paths.clear();
}

CodedBulkMulticastSenderMulticastPath::CodedBulkMulticastSenderMulticastPath(const CodedBulkMulticastSenderMulticastPath& path)
{
  m_connected   = path.m_connected;
  m_application = path.m_application;
  m_paths       = path.m_paths;
}

void
CodedBulkMulticastSenderMulticastPath::ConnectPeers(void) {
  for(std::vector<Ptr<CodedBulkMulticastSenderPath> >::iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
    (*it)->ConnectPeer();
  }
}

void
CodedBulkMulticastSenderMulticastPath::DisconnectPeers(void)
{
  m_connected_lock.lock();
  for(std::vector<Ptr<CodedBulkMulticastSenderPath> >::iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
    (*it)->DisconnectPeer();
  }
  m_connected = false;
  m_connected_lock.unlock();
}

void
CodedBulkMulticastSenderMulticastPath::AddRemote(const Address& peer, uint32_t path_id) {
  Ptr<CodedBulkMulticastSenderPath> path = Create<CodedBulkMulticastSenderPath> (this);
  path->SetRemote(peer);
  path->SetPathID(path_id);
  m_paths.push_back(path);
}


Socket*
CodedBulkMulticastSenderMulticastPath::GetSocket (unsigned int index) const
{
  return m_paths[index]->GetSocket();
}

int
CodedBulkMulticastSenderMulticastPath::Send (Packet* packet)
{
  int min_actual_sent = 0;
  if(m_paths.size() > 0) {
    min_actual_sent = std::numeric_limits<int>::max();
  }
  int actual_sent = 0;
  for(std::vector<Ptr<CodedBulkMulticastSenderPath> >::iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
    actual_sent = (*it)->Send(packet);
    if(actual_sent < min_actual_sent) {
      min_actual_sent = actual_sent;
    }
  }
  return min_actual_sent;
}

void
CodedBulkMulticastSenderMulticastPath::DataSent (CodedBulkMulticastSenderPath* path)
{
  // one of the socket sent a data packet
  if ( !m_connected ) {
    CheckConnection ();
    if ( !m_connected ) {
      // if not all paths are ready
      return;
    }
  }
  m_check_ready_lock.lock();
  if ( CheckReady () ){
    // notify that the path is ready to send the next packet
    m_application->PathReady(this);
/*
    if( m_application->m_through_proxy ) {
      m_application->AllReady();
    } else {
      m_application->PathReady(this);
    }
*/
  }
  m_check_ready_lock.unlock();
}

void
CodedBulkMulticastSenderMulticastPath::CheckConnection (void)
{
  m_connected_lock.lock();
  for(std::vector<Ptr<CodedBulkMulticastSenderPath> >::iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
    if( !(*it)->m_connected ) {
      m_connected = false;
      m_connected_lock.unlock();
      return;
    }
  }
  m_connected = true;
  m_connected_lock.unlock();
}

bool
CodedBulkMulticastSenderMulticastPath::CheckReady (void)
{
  m_ready_lock.lock();
  for(std::vector<Ptr<CodedBulkMulticastSenderPath> >::iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
    if( !(*it)->m_ready ) {
      m_ready = false;
      m_ready_lock.unlock();
      return false;
    }
  }
  m_ready = true;
  m_ready_lock.unlock();
  return true;
}

CodedBulkMulticastSender::CodedBulkMulticastSender (void) :
  m_maxBytes(0)
{
}

CodedBulkMulticastSender::~CodedBulkMulticastSender (void)
{
  m_multicast_paths.clear();
}

void
CodedBulkMulticastSender::SetRemoteAddresses(const std::list<Address>& addresses)
{
  m_multicast_paths.clear();
  m_remote_addresses = addresses;
}

Ptr<CodedBulkMulticastSenderMulticastPath>
CodedBulkMulticastSender::AddMulticastPath (const std::list<uint32_t>& path_ids, uint8_t path_type)
{
  Ptr<CodedBulkMulticastSenderMulticastPath> path = nullptr;
  switch(path_type) {
    case 6:
      {
        // single path
        path = Create<CodedBulkMulticastSenderMulticastPath> (this);
        path->SetPriority(GetPriority());
        m_multicast_paths.push_back(path);
        std::list<Address>::const_iterator it_addr = m_remote_addresses.begin();
        for(std::list<uint32_t>::const_iterator it_port = path_ids.begin(); it_port != path_ids.end(); ++it_port) {
          path->AddRemote(*it_addr, *it_port);
          ++it_addr;
        }
      }
      break;
    case 4:
      {
        // multipath coded
        for(uint8_t i = 2; i <= 4; i += 2) {
          path = Create<CodedBulkMulticastSenderMulticastPath> (this);
          path->SetPriority (i);
          m_multicast_paths.push_back(path);
          std::list<Address>::const_iterator it_addr = m_remote_addresses.begin();
          for(std::list<uint32_t>::const_iterator it_port = path_ids.begin(); it_port != path_ids.end(); ++it_port) {
            uint32_t path_id = (i == 2) ? *it_port : *it_port + 1;
            path->AddRemote(*it_addr, path_id);
            ++it_addr;
          }
        }
      }
      break;
    case 3:
      {
        // Steiner tree
        path = Create<CodedBulkMulticastSenderMulticastPath> (this);
        path->SetPriority (4);
        m_multicast_paths.push_back(path);
        std::list<Address>::const_iterator it_addr = m_remote_addresses.begin();
        for(std::list<uint32_t>::const_iterator it_port = path_ids.begin(); it_port != path_ids.end(); ++it_port) {
          path->AddRemote(*it_addr, *it_port + 1);
          ++it_addr;
        }
      }
      break;
    case 2:
      {
        // multipath non-coded
        path = Create<CodedBulkMulticastSenderMulticastPath> (this);
        path->SetPriority (2);
        m_multicast_paths.push_back(path);
        std::list<Address>::const_iterator it_addr = m_remote_addresses.begin();
        for(std::list<uint32_t>::const_iterator it_port = path_ids.begin(); it_port != path_ids.end(); ++it_port) {
          path->AddRemote(*it_addr, *it_port);
          ++it_addr;
        }
      }
      break;
  }
  return path;
}

Ptr<CodedBulkMulticastSenderMulticastPath>
CodedBulkMulticastSender::GetMulticastPath (unsigned int path_index)
{
  return m_multicast_paths[path_index];
}

void
CodedBulkMulticastSender::SetMaxBytes (uint64_t maxBytes)
{
  m_maxBytes = maxBytes;
}

Socket*
CodedBulkMulticastSender::GetSocket (unsigned int multicast_path_index, unsigned int path_index) const
{
  return m_multicast_paths[multicast_path_index]->GetSocket(path_index);
}

void
CodedBulkMulticastSender::StartApplication (void)
{
  for(std::vector<Ptr<CodedBulkMulticastSenderMulticastPath> >::iterator
    it  = m_multicast_paths.begin();
    it != m_multicast_paths.end();
    ++it)
  {
    (*it)->ConnectPeers();
  }
}

void
CodedBulkMulticastSender::StopApplication (void)
{
  for(std::vector<Ptr<CodedBulkMulticastSenderMulticastPath> >::iterator
    it  = m_multicast_paths.begin();
    it != m_multicast_paths.end();
    ++it)
  {
    (*it)->DisconnectPeers();
  }
}

void
CodedBulkMulticastSender::AllReady (void)
{
  m_all_ready_lock.lock();
  // encode all paths...
  for(std::vector<Ptr<CodedBulkMulticastSenderMulticastPath> >::iterator
    it  = m_multicast_paths.begin();
    it != m_multicast_paths.end();
    ++it
  ) {
    if ( !(*it)->m_ready ) {
      m_all_ready_lock.unlock();
      return;
    }
  }
  // all ready
  for(std::vector<Ptr<CodedBulkMulticastSenderMulticastPath> >::iterator
    it  = m_multicast_paths.begin();
    it != m_multicast_paths.end();
    ++it
  ) {
    // the relationship with marker ready is kind of weak
    (*it)->m_ready = false;
    PathReady(PeekPointer(*it));
  }
  m_all_ready_lock.unlock();
}

void
CodedBulkMulticastSender::PathReady(CodedBulkMulticastSenderMulticastPath* path)
{
  // the path is ready
}

int
CodedBulkMulticastSender::SendPacket(CodedBulkMulticastSenderMulticastPath* path, Packet* packet)
{
  int actual = path->Send (packet);
  m_totBytes_lock.lock();
  if (actual > 0)
    {
      m_totBytes += actual;
    }
  m_totBytes_lock.unlock();
  return actual;
}
