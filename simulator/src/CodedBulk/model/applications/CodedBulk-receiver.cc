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
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "CodedBulk-receiver.h"
#include "ns3/tcp-proxy.h"
#include "ns3/CodedBulk-flow-identifier.h"

#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkReceiver");

NS_OBJECT_ENSURE_REGISTERED (CodedBulkReceiver);

CodedBulkReceiverPath::CodedBulkReceiverPath (CodedBulkReceiver* application, uint32_t path_id) :
  m_application(application),
  m_path_id(path_id)
{
  NS_LOG_FUNCTION (this);
}

CodedBulkReceiverPath::~CodedBulkReceiverPath ()
{
  NS_LOG_FUNCTION (this);
  m_socket = NULL;
}

CodedBulkReceiverListener::CodedBulkReceiverListener (CodedBulkReceiver* application) :
  m_application(application)
{
  NS_LOG_FUNCTION (this);
}

CodedBulkReceiverListener::~CodedBulkReceiverListener ()
{
  NS_LOG_FUNCTION (this);
  StopListening ();
  m_listening_socket = 0;
}

void
CodedBulkReceiverListener::ListenLocal (const Ptr<Node> node, const TypeId& tid)
{
  if (!m_listening_socket)
    {
      m_listening_socket = Socket::CreateSocket (node, tid);
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

  //m_listening_socket->SetRecvCallback (MakeCallback (&CodedBulkReceiver::HandleRead, m_application));
  m_listening_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&CodedBulkReceiverListener::HandleAccept, this));
  m_listening_socket->SetCloseCallbacks (
    MakeCallback (&CodedBulkReceiverListener::HandlePeerClose, this),
    MakeCallback (&CodedBulkReceiverListener::HandlePeerError, this));
}

void
CodedBulkReceiverListener::StopListening (void)
{
  if (m_listening_socket) 
    {
      m_listening_socket->Close ();
      m_listening_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void
CodedBulkReceiverListener::SetLocal (const Address& local)
{
  m_local = local;
}

Ptr<Socket>
CodedBulkReceiverListener::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_listening_socket;
}

void
CodedBulkReceiverListener::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  // let ack have high priority
  // m_listening_socket->SetIpTos (m_application->GetPriority () << 2);
  s->SetIpTos (6 << 2);
  s->SetRecvCallback (MakeCallback (&CodedBulkReceiverListener::WaitForFlowIdentifier, this));
}

void
CodedBulkReceiverListener::WaitForFlowIdentifier (Ptr<Socket> socket) const
{
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
  NS_LOG_INFO("receive path id " << mpls_flow_identifier.GetValue());
  Ptr<CodedBulkReceiverPath> path = m_application->AddPath(mpls_flow_identifier.GetValue());
  if(m_application->m_through_proxy) {
    Ptr<TcpSocketBase> socket_base = DynamicCast<TcpSocketBase> (socket);
    socket_base->SetSlowAck();
  }
  path->m_socket = socket;
  socket->SetRecvCallback (MakeCallback (&CodedBulkReceiver::HandleRead, m_application));
}

void
CodedBulkReceiverListener::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
void
CodedBulkReceiverListener::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

TypeId 
CodedBulkReceiver::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodedBulkReceiver")
    .SetParent<CodedBulkApplication> ()
    .SetGroupName("Applications")
    .AddConstructor<CodedBulkReceiver> ()
  ;
  return tid;
}

CodedBulkReceiver::CodedBulkReceiver () :
  m_listener(this)
{
  NS_LOG_FUNCTION (this);
  m_paths.clear();
}

CodedBulkReceiver::~CodedBulkReceiver()
{
  NS_LOG_FUNCTION (this);
}

void
CodedBulkReceiver::SetLocal(const Ipv4Address& local)
{
  NS_LOG_DEBUG("local address " << local << " application id = " << m_application_id);
  m_local = local;
  m_listener.SetLocal(Address(InetSocketAddress (m_local, m_application_id)));
}

Ptr<CodedBulkReceiverPath>
CodedBulkReceiver::AddPath (const uint32_t path_id)
{
  Ptr<CodedBulkReceiverPath> path = Create<CodedBulkReceiverPath> (this, path_id);
  m_paths.push_back(path);
  return path;
}

Ptr<Socket>
CodedBulkReceiver::GetListeningSocket (void)
{
  NS_LOG_FUNCTION (this);
  return m_listener.m_listening_socket;
}

void
CodedBulkReceiver::HandleRead (Ptr<Socket> socket)
{
  //NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (TcpProxy::GetDataSegSize (),0,from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
//      NS_LOG_DEBUG("Receive packet with size " << packet->GetSize ());

      m_totBytes += packet->GetSize ();

      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s CodedBulk receiver received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totBytes << " bytes");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s CodedBulk receiver received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totBytes << " bytes");
        }
    }
}

void
CodedBulkReceiver::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  m_listener.ListenLocal(GetNode(),m_tid);
}

void
CodedBulkReceiver::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  m_listener.StopListening();
  for(std::vector<Ptr<CodedBulkReceiverPath> >::iterator
    it  = m_paths.begin();
    it != m_paths.end();
    ++it)
  {
    (*it)->m_socket->Close ();
  }
  
}

} // Namespace ns3
