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
#include "ns3/udp-socket-factory.h"
#include "ns3/simulator.h"

#include "CodedBulk-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkApplication");

NS_OBJECT_ENSURE_REGISTERED (CodedBulkApplication);

TypeId 
CodedBulkApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodedBulkApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<CodedBulkApplication> ()
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&CodedBulkApplication::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("MeasurementInterval", "The measurement interval",
                   TimeValue (MilliSeconds (1000.0)),
                   MakeTimeAccessor (&CodedBulkApplication::m_measure_interval),
                   MakeTimeChecker ())
  ;
  return tid;
}

CodedBulkApplication::CodedBulkApplication () :
  m_through_proxy(false),
  m_totBytes(0),
  m_prevTotBytes(0),
  m_bytes_interval(0),
  m_measure_interval(MilliSeconds(1000.0))
{
  NS_LOG_FUNCTION (this);
  Measuring();
}

CodedBulkApplication::~CodedBulkApplication()
{
  NS_LOG_FUNCTION (this);
}

void
CodedBulkApplication::SetApplicationID (uint16_t application_id)
{
  m_application_id = application_id;
}

void
CodedBulkApplication::SetProtocol (const TypeId& tid)
{
  m_tid = tid;
}

void
CodedBulkApplication::SetPriority (uint8_t priority)
{
  m_priority = priority;
}

uint8_t
CodedBulkApplication::GetPriority (void)
{
  return m_priority;
}

uint64_t
CodedBulkApplication::GetTotBytes (void)
{
  return m_totBytes;
}

uint64_t
CodedBulkApplication::GetMeasuredBytes (void)
{
  // return the measured bytes within the previous interval
  return m_bytes_interval;
}

void
CodedBulkApplication::Measuring(void)
{
  //NS_LOG_FUNCTION_NOARGS();
  NS_LOG_DEBUG( "total = " << m_totBytes << " prev = " << m_prevTotBytes);
  m_bytes_interval = m_totBytes - m_prevTotBytes;
  m_prevTotBytes = m_totBytes;
  NS_LOG_DEBUG( "after measurement: total = " << m_totBytes << " prev = " << m_prevTotBytes << " bytes = " << m_bytes_interval);
  Simulator::Schedule (m_measure_interval, &CodedBulkApplication::Measuring, this);
}

void
CodedBulkApplication::SetThroughProxy (bool through_proxy)
{
  m_through_proxy = through_proxy;
}

void
CodedBulkApplication::MeasureThroughput (std::ofstream& fout)
{
  /*
  Measuring ();
  GetTimeStamp (fout);
  fout << m_bytes_interval << std::endl;
  */
}

} // Namespace ns3
