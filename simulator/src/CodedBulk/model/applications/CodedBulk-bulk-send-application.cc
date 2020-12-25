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
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"

#include "CodedBulk-bulk-send-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkBulkSendApplication");

NS_OBJECT_ENSURE_REGISTERED (CodedBulkBulkSendApplication);

TypeId
CodedBulkBulkSendApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodedBulkBulkSendApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications") 
    .AddConstructor<CodedBulkBulkSendApplication> ()
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                   UintegerValue (512),
                   MakeUintegerAccessor (&CodedBulkBulkSendApplication::m_sendSize),
                   MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}

CodedBulkBulkSendApplication::CodedBulkBulkSendApplication()
{
  m_ready_paths.clear();
}

void
CodedBulkBulkSendApplication::InteractiveInputAt (Time time, uint64_t bytes)
{
  Simulator::Schedule (time, &CodedBulkBulkSendApplication::InteractiveInput, this, bytes);
}

void
CodedBulkBulkSendApplication::InteractiveInput (uint64_t bytes)
{
  m_maxBytes += bytes;
  uint32_t num_all_ready_paths = m_ready_paths.size();
  while(!m_ready_paths.empty() && (num_all_ready_paths > 0)) {
    Ptr<CodedBulkMulticastSenderMulticastPath> path = m_ready_paths.front();
    m_ready_paths.pop_front();
    --num_all_ready_paths;
    if( SendData (path) == -1 ) {
      continue;
    }
  }
  if(m_totBytes < m_maxBytes) {
    // still have something to send
    Simulator::Schedule (MilliSeconds(1), &CodedBulkBulkSendApplication::InteractiveInput, this, 0);
  }
}

int
CodedBulkBulkSendApplication::SendData (Ptr<CodedBulkMulticastSenderMulticastPath> path)
{
  NS_LOG_FUNCTION (this);

  if ((GetPriority() <= 2) || (m_totBytes < m_maxBytes))
    { // Time to send more

      // uint64_t to allow the comparison later.
      // the result is in a uint32_t range anyway, because
      // m_sendSize is uint32_t.
      uint64_t toSend = m_sendSize;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (toSend, m_maxBytes - m_totBytes);
        }

      Ptr<Packet> packet = Create<Packet> (toSend);
      
      return SendPacket(path, packet);
    }
  else
    {
      m_ready_paths.push_back(path);
    }


  // Check if time to close (all sent)
/*
  if (m_totBytes == m_maxBytes)
    {
      StopApplication ();
    }
*/
  return -1;
}

void
CodedBulkBulkSendApplication::PathReady(Ptr<CodedBulkMulticastSenderMulticastPath> path)
{
  SendData (path);
  //while (SendData (path) > 0) {}
}

} // Namespace ns3