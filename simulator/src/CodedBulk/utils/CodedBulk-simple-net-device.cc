/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
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
 * Author: Shih-Hao Tseng <st688@cornell.edu>
 */
#include "ns3/error-model.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/log.h"
#include "ns3/ipv4-header.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/socket.h"
#include "ns3/tag.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/inet-socket-address.h"
#include "ns3/queue.h"

#include "CodedBulk-flow-identifier.h"
#include "CodedBulk-simple-net-device.h"

// should be the same as in openflow-routing-switch.h
#define OPENFLOW_ROUTING_SWITCH_ANY_PORT 0xffff
#define ETH_TYPE_MPLS_UNICAST 0x8847
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkSimpleNetDevice");

NS_OBJECT_ENSURE_REGISTERED (CodedBulkSimpleNetDevice);

uint8_t CodedBulkSimpleNetDevice::__priority_levels = 8;

TypeId 
CodedBulkSimpleNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodedBulkSimpleNetDevice")
    .SetParent<SimpleNetDevice> ()
    .AddConstructor<CodedBulkSimpleNetDevice> ()
  ;
  return tid;
}

void
CodedBulkSimpleNetDevice::SetPriorityQueue (uint8_t priority, Ptr<Queue<Packet> > q)
{
  m_priority_queues[priority] = q;
}

uint32_t
CodedBulkSimpleNetDevice::GetNPackets (uint8_t priority) const
{
  return m_priority_queues.at(priority)->GetNPackets();
}

bool
CodedBulkSimpleNetDevice::SendFrom (Ptr<Packet> p, const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << p << source << dest << protocolNumber);
  if (p->GetSize () > GetMtu ())
    {
      return false;
    }
  Ptr<Packet> packet = p->Copy ();
 
  uint8_t priority = 0;
  Ipv4Header ip_hd;
  packet->PeekHeader(ip_hd);
  priority = ip_hd.GetTos () >> 2;
  NS_LOG_DEBUG("the priority is " << (int)priority);
  
  if (m_priority_queues.find(priority) == m_priority_queues.end()) {
    // does not support the priority
    return false;
  }

  Mac48Address to = Mac48Address::ConvertFrom (dest);
  Mac48Address from = Mac48Address::ConvertFrom (source);

  SimpleTag tag;
  tag.SetSrc (from);
  tag.SetDst (to);
  tag.SetProto (protocolNumber);

  packet->AddPacketTag (tag);

  if (m_priority_queues[priority]->Enqueue (packet))
    {
      if (!TransmitCompleteEvent.IsRunning ())
        {
          TransmitComplete ();
        }
      return true;
    }

  return false;
}

void
CodedBulkSimpleNetDevice::TransmitComplete (void)
{
  NS_LOG_FUNCTION (this);
  bool no_packet = true;
  uint8_t priority = __priority_levels;
  for(; priority > 0; --priority )
    {
      if (m_priority_queues[priority-1]->GetNPackets () != 0)
        {
          no_packet = false;
          break;
        }
    }
  if ( no_packet )
    {
      return;
    }
  --priority;
/*  
  if (m_queue->GetNPackets () == 0)
    {
      return;
    }
*/
  Ptr<Packet> packet = m_priority_queues[priority]->Dequeue ();

  SimpleTag tag;
  packet->RemovePacketTag (tag);

  Mac48Address src = tag.GetSrc ();
  Mac48Address dst = tag.GetDst ();
  uint16_t proto = tag.GetProto ();

  m_channel->Send (packet, proto, dst, src, this);
  m_totBytes += packet->GetSize();

  // every time sending out a packet, we need to set the sending period
  Time txTime = Time (0);
  if (m_bps > DataRate (0))
    {
      txTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
    }
  TransmitCompleteEvent = Simulator::Schedule (txTime, &CodedBulkSimpleNetDevice::TransmitComplete, this);

  return;
}

uint64_t
CodedBulkSimpleNetDevice::GetMeasuredBytes (void)
{
  // return the measured bytes within the previous interval
  return m_bytes_interval;
}

void
CodedBulkSimpleNetDevice::InitializeMeasureParameters (void)
{
  m_totBytes = 0;
  m_prevTotBytes = 0;
  m_bytes_interval = 0;
}

void
CodedBulkSimpleNetDevice::Measuring(void)
{
  //NS_LOG_FUNCTION_NOARGS();
  m_bytes_interval = m_totBytes - m_prevTotBytes;
  m_prevTotBytes = m_totBytes;
  Simulator::Schedule (Seconds(1.0), &CodedBulkSimpleNetDevice::Measuring, this);
}

} // namespace ns3
