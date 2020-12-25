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

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/data-rate.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "CodedBulk-multicast-sender.h"
#include "ns3/CodedBulk-flow-identifier.h"

#include <limits>
#include <algorithm>

#define PROXY_PORT 1000

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkMulticastSender");

NS_OBJECT_ENSURE_REGISTERED (CodedBulkMulticastSender);

Time CodedBulkMulticastSenderPath::__resend_period = MilliSeconds(100);

CodedBulkMulticastSenderPath::CodedBulkMulticastSenderPath (Ptr<CodedBulkMulticastSenderMulticastPath> root) :
  m_root(root),
  m_current_packet(NULL),
  m_path_id(0),
  m_socket(NULL),
  m_connected(false),
  m_ready(false)
{
  //NS_LOG_FUNCTION (this);
}

CodedBulkMulticastSenderPath::CodedBulkMulticastSenderPath (const CodedBulkMulticastSenderPath& path)
{
  //NS_LOG_FUNCTION (this << " copy constructor");
  m_root      = path.m_root; 
  m_path_id   = path.m_path_id;
  m_peer      = path.m_peer;
  m_socket    = path.m_socket;
  m_connected = path.m_connected;
  m_ready     = path.m_ready;
}

CodedBulkMulticastSenderPath::~CodedBulkMulticastSenderPath()
{
  //NS_LOG_FUNCTION (this);
  DisconnectPeer();
  m_socket = 0;
}

void
CodedBulkMulticastSenderPath::ConnectPeer(const Ptr<Node> node, const TypeId& tid)
{
  // Create the socket if not already
  NS_LOG_DEBUG("connect peer " << m_path_id);
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (node, tid);
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
        MakeCallback (&CodedBulkMulticastSenderPath::ConnectionSucceeded, this),
        MakeCallback (&CodedBulkMulticastSenderPath::ConnectionFailed, this));

      m_socket->SetSendCallback (
        MakeCallback (&CodedBulkMulticastSenderPath::PathEstablished, this));
    }
}

void
CodedBulkMulticastSenderPath::DisconnectPeer (void)
{
  if(m_socket != 0)
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

Ptr<Socket>
CodedBulkMulticastSenderPath::GetSocket (void) const
{
  return m_socket;
}

void
CodedBulkMulticastSenderPath::ConnectionSucceeded (Ptr<Socket> socket)
{
  //NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void
CodedBulkMulticastSenderPath::ConnectionFailed (Ptr<Socket> socket)
{
  //NS_LOG_FUNCTION (this << socket);
}

int
CodedBulkMulticastSenderPath::Send (Ptr<Packet> packet)
{
  if (!m_ready || (m_current_packet != NULL)) {
    return -1;
  }
  int ret = m_socket->Send (packet);
  if( ret < 0 ) {
    m_ready = false;
    m_current_packet = packet;
    m_resend_event = Simulator::Schedule (__resend_period, &CodedBulkMulticastSenderPath::Resend, this);
  } else {
    m_ready = true;
  }
  return ret;
}

void
CodedBulkMulticastSenderPath::Resend ()
{
  Simulator::Cancel(m_resend_event);
  int ret = m_socket->Send (m_current_packet);
  if(ret != -1) {
    m_current_packet = NULL;
    m_ready = true;
    m_root->DataSent(this);
  } else {
    m_resend_event = Simulator::Schedule (__resend_period, &CodedBulkMulticastSenderPath::Resend, this);
  }
}

void
CodedBulkMulticastSenderPath::PathEstablished (Ptr<Socket>, uint32_t)
{
  NS_LOG_DEBUG("establish path " << m_path_id);
  Ptr<Packet> packet = Create<Packet> ();
  CodedBulkFlowIdentifier mpls_flow_identifier (m_path_id);
  packet->AddHeader(mpls_flow_identifier);
  while (m_socket->Send (packet) <= 0) {}
  m_socket->SetIpTos (m_root->GetPriority () << 2);
  m_socket->SetSendCallback (
    MakeCallback (&CodedBulkMulticastSenderPath::PathReady, this));
}

void
CodedBulkMulticastSenderPath::PathReady (Ptr<Socket>, uint32_t)
{
  if(m_current_packet == NULL) {
    m_ready = true;
    m_root->DataSent(this);
  } else {
    Resend ();
  }
}

CodedBulkMulticastSenderMulticastPath::CodedBulkMulticastSenderMulticastPath (Ptr<CodedBulkMulticastSender> application) :
  m_connected(false),
  m_application(application)
{
  //NS_LOG_DEBUG("m_application = " << m_application);
  m_paths.clear();
}

CodedBulkMulticastSenderMulticastPath::CodedBulkMulticastSenderMulticastPath(const CodedBulkMulticastSenderMulticastPath& path)
{
  m_connected   = path.m_connected;
  m_application = path.m_application;
  m_paths       = path.m_paths;
}

void
CodedBulkMulticastSenderMulticastPath::ConnectPeers(const Ptr<Node> node, const TypeId& tid) {
  //NS_LOG_FUNCTION (this);
  for(std::vector<Ptr<CodedBulkMulticastSenderPath> >::iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
    //NS_LOG_FUNCTION ("connect a path");
    (*it)->ConnectPeer(node,tid);
  }
}

void
CodedBulkMulticastSenderMulticastPath::DisconnectPeers(void)
{
  for(std::vector<Ptr<CodedBulkMulticastSenderPath> >::iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
    (*it)->DisconnectPeer();
  }
  m_connected = false;
}

void
CodedBulkMulticastSenderMulticastPath::AddRemote(const Address& peer, uint32_t path_id) {
  Ptr<CodedBulkMulticastSenderPath> path = Create<CodedBulkMulticastSenderPath> (this);
  path->SetRemote(peer);
  path->SetPathID(path_id);
  m_paths.push_back(path);
}


Ptr<Socket>
CodedBulkMulticastSenderMulticastPath::GetSocket (unsigned int index) const
{
  NS_ASSERT( (index >= 0) && (index < m_paths.size()) );
  return m_paths[index]->GetSocket();
}

int
CodedBulkMulticastSenderMulticastPath::Send (Ptr<Packet> packet)
{
  if(!m_ready) {
    return 0;
  }
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
CodedBulkMulticastSenderMulticastPath::DataSent (Ptr<CodedBulkMulticastSenderPath> path)
{
  // one of the socket sent a data packet
  if ( !m_connected ) {
    CheckConnection ();
    if ( !m_connected ) {
      // if not all paths are ready
      return;
    }
  }

  if ( CheckReady () ){
    // notify that the path is ready to send the next packet
    m_application->PathReady(this);
/*
    if( m_priority == 4 ) {
      // bulk coded traffic
      m_application->AllReady(this);
    } else {
      m_application->PathReady(this);
      //NS_LOG_DEBUG("single path ready");
    }
*/
  }
}

void
CodedBulkMulticastSenderMulticastPath::CheckConnection (void)
{
  m_connected = true;
  for(std::vector<Ptr<CodedBulkMulticastSenderPath> >::iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
    if( !(*it)->m_connected ) {
      m_connected = false;
      return;
    }
  }
}

bool
CodedBulkMulticastSenderMulticastPath::CheckReady (void)
{
  for(std::vector<Ptr<CodedBulkMulticastSenderPath> >::iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
    if( !(*it)->m_ready ) {
      return false;
    }
  }
  m_ready = true;
  return true;
}

//Ptr<UniformRandomVariable> CodedBulkMulticastSender::m_unif_rv = CreateObject<UniformRandomVariable> ();

TypeId
CodedBulkMulticastSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodedBulkMulticastSender")
    .SetParent<CodedBulkApplication> ()
    .SetGroupName("Applications")
    .AddConstructor<CodedBulkMulticastSender> ()
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. "
                   "Once these bytes are sent, "
                   "no data  is sent again. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CodedBulkMulticastSender::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
  ;
  return tid;
}


CodedBulkMulticastSender::CodedBulkMulticastSender (void) :
  m_maxBytes(0)
{
  //NS_LOG_FUNCTION (this);
  //m_ready_paths.clear();
}

CodedBulkMulticastSender::~CodedBulkMulticastSender (void)
{
  //NS_LOG_FUNCTION (this);
  m_multicast_paths.clear();
}

void
CodedBulkMulticastSender::SetRemoteAddresses(const std::list<Ipv4Address>& addresses)
{
  m_multicast_paths.clear();
  m_remote_addresses = addresses;
}

Ptr<CodedBulkMulticastSenderMulticastPath>
CodedBulkMulticastSender::AddMulticastPath (const std::list<uint32_t>& path_ids, uint8_t path_type)
{
  //NS_LOG_FUNCTION (this);
  NS_ASSERT( m_remote_addresses.size() == path_ids.size() );

  Ptr<CodedBulkMulticastSenderMulticastPath> path = NULL;
  switch(path_type) {
    case 6:
      {
        // single path
        path = Create<CodedBulkMulticastSenderMulticastPath> (this);
        path->SetPriority (GetPriority());
        m_multicast_paths.push_back(path);
        std::list<Ipv4Address>::const_iterator it_addr = m_remote_addresses.begin();
        for(std::list<uint32_t>::const_iterator it_port = path_ids.begin(); it_port != path_ids.end(); ++it_port) {
          //NS_LOG_DEBUG("Add path to " << *it_addr << " with port " << *it_port);
          if(m_through_proxy) {
            path->AddRemote(Address(InetSocketAddress (*it_addr, PROXY_PORT)), *it_port);
          } else {
            path->AddRemote(Address(InetSocketAddress (*it_addr, m_application_id)), *it_port);
          }
          ++it_addr;
        }
      }
      break;
    case 4:
      {
        // coded
        for(uint8_t i = 2; i <= 4; i += 2) {
          path = Create<CodedBulkMulticastSenderMulticastPath> (this);
          path->SetPriority (i);
          //if (i == 4) {
          //  // add a multicast path
          //  ++m_num_coded_multicast_paths;
          //}
          m_multicast_paths.push_back(path);
          std::list<Ipv4Address>::const_iterator it_addr = m_remote_addresses.begin();
          for(std::list<uint32_t>::const_iterator it_port = path_ids.begin(); it_port != path_ids.end(); ++it_port) {
            //NS_LOG_DEBUG("Add path to " << *it_addr << " with port " << *it_port);
            // *it_port is always odd
            // odd  -> non coded multipath
            // even -> coded
            uint32_t path_id = (i == 2) ? *it_port : *it_port + 1;
            // coded traffic must go through proxy
            path->AddRemote(Address(InetSocketAddress (*it_addr, PROXY_PORT)), path_id);
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
        //++m_num_coded_multicast_paths;
        m_multicast_paths.push_back(path);
        std::list<Ipv4Address>::const_iterator it_addr = m_remote_addresses.begin();
        for(std::list<uint32_t>::const_iterator it_port = path_ids.begin(); it_port != path_ids.end(); ++it_port) {
          path->AddRemote(Address(InetSocketAddress (*it_addr, PROXY_PORT)), *it_port + 1);
          ++it_addr;
        }
      }
      break;
    case 2:
      {
        // multipath
        path = Create<CodedBulkMulticastSenderMulticastPath> (this);
        path->SetPriority (2);
        m_multicast_paths.push_back(path);
        std::list<Ipv4Address>::const_iterator it_addr = m_remote_addresses.begin();
        for(std::list<uint32_t>::const_iterator it_port = path_ids.begin(); it_port != path_ids.end(); ++it_port) {
          // must go through proxy
          path->AddRemote(Address(InetSocketAddress (*it_addr, PROXY_PORT)), *it_port);
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
  NS_ASSERT( (path_index >= 0) && (path_index < m_multicast_paths.size()) );
  return m_multicast_paths[path_index];
}

void
CodedBulkMulticastSender::SetMaxBytes (uint64_t maxBytes)
{
  //NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
CodedBulkMulticastSender::GetSocket (unsigned int multicast_path_index, unsigned int path_index) const
{
  //NS_LOG_FUNCTION (this);
  NS_ASSERT( (multicast_path_index >= 0) && (multicast_path_index < m_multicast_paths.size()) );
  return m_multicast_paths[multicast_path_index]->GetSocket(path_index);
}

void
CodedBulkMulticastSender::StartApplication (void)
{
  //NS_LOG_FUNCTION (this);
  /*
  if(m_through_proxy) {
    NS_LOG_DEBUG("An Proxy flow");
  } else {
    NS_LOG_DEBUG("A TCP/IP flow");
  }
  */
  for(std::vector<Ptr<CodedBulkMulticastSenderMulticastPath> >::iterator
    it  = m_multicast_paths.begin();
    it != m_multicast_paths.end();
    ++it)
  {
    (*it)->ConnectPeers(GetNode(),m_tid);
  }
}

void
CodedBulkMulticastSender::StopApplication (void)
{
  //NS_LOG_FUNCTION (this);
  for(std::vector<Ptr<CodedBulkMulticastSenderMulticastPath> >::iterator
    it  = m_multicast_paths.begin();
    it != m_multicast_paths.end();
    ++it)
  {
    (*it)->DisconnectPeers();
  }
}

void
CodedBulkMulticastSender::AllReady (Ptr<CodedBulkMulticastSenderMulticastPath> path)
{
/*
  // this is important for low bandwidth ns3 simulation, which converges very slowly
  // only check the bulk coded traffic
  if (std::find(m_ready_paths.begin(), m_ready_paths.end(), path) == m_ready_paths.end()) {
    // the last come gets sent earlier
    m_ready_paths.push_front(path);
  }

  if (m_ready_paths.size() == m_num_coded_multicast_paths) {
    std::list<Ptr<CodedBulkMulticastSenderMulticastPath> > ready_paths = std::move(m_ready_paths);
    m_ready_paths.clear();
    for (auto& ready_path : ready_paths) {
      PathReady(ready_path);
    }
  }
*/
}

void
CodedBulkMulticastSender::PathReady(Ptr<CodedBulkMulticastSenderMulticastPath> path)
{
  // the path is ready
  //NS_LOG_FUNCTION (this);
}

int
CodedBulkMulticastSender::SendPacket(Ptr<CodedBulkMulticastSenderMulticastPath>& path, Ptr<Packet>& packet)
{
  //NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());
  int actual = path->Send (packet);
  if (actual > 0)
    {
      m_totBytes += actual;
    }
  return actual;
}

} // Namespace ns3