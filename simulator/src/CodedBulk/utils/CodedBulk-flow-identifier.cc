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
 * Author: Shih-Hao Tseng <st688@cornell.edu>
 */

#include <iostream>
#include "CodedBulk-flow-identifier.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkFlowIdentifier");

NS_OBJECT_ENSURE_REGISTERED (CodedBulkFlowIdentifier);

CodedBulkFlowIdentifier::CodedBulkFlowIdentifier()
{
  SetValue(0);
}

CodedBulkFlowIdentifier::CodedBulkFlowIdentifier (const uint32_t c)
{
  SetValue(c);
}

CodedBulkFlowIdentifier::~CodedBulkFlowIdentifier ()
{
  SetValue(0);
}

TypeId 
CodedBulkFlowIdentifier::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodedBulkFlowIdentifier")
    .SetParent<Header> ()
    .AddConstructor<CodedBulkFlowIdentifier> ()
  ;
  return tid;
}
TypeId 
CodedBulkFlowIdentifier::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void 
CodedBulkFlowIdentifier::Print (std::ostream &os) const
{
  os << " value: " << m_value;
}

uint32_t 
CodedBulkFlowIdentifier::GetSerializedSize (void) const
{
  return 4;
}

void
CodedBulkFlowIdentifier::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_value);
}
uint32_t
CodedBulkFlowIdentifier::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_value = i.ReadNtohU32 ();

  return GetSerializedSize ();
}

void
CodedBulkFlowIdentifier::SetValue (uint32_t value)
{
  m_value = value;
}

uint32_t
CodedBulkFlowIdentifier::GetValue (void) const
{
  return m_value;
}

bool
operator== (const CodedBulkFlowIdentifier &lhs, const CodedBulkFlowIdentifier &rhs)
{
  return (
    lhs.m_value == rhs.m_value
  );
}

std::ostream&
operator<< (std::ostream& os, CodedBulkFlowIdentifier const & flow_id)
{
  flow_id.Print (os);
  return os;
}

} // namespace ns3
