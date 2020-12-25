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
#include "CodedBulk-header-fixer.h"
#include "ns3/log.h"
#include "ns3/tcp-l4-protocol.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkHeaderFixer");

NS_OBJECT_ENSURE_REGISTERED (CodedBulkHeaderFixer);

CodedBulkHeaderFixer::CodedBulkHeaderFixer(){}

CodedBulkHeaderFixer::~CodedBulkHeaderFixer (){}

TypeId 
CodedBulkHeaderFixer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CodedBulkHeaderFixer")
    .SetParent<Header> ()
    .SetGroupName ("Internet")
    .AddConstructor<CodedBulkHeaderFixer> ()
  ;
  return tid;
}
TypeId 
CodedBulkHeaderFixer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void 
CodedBulkHeaderFixer::Print (std::ostream &os) const
{
  os << " port: " << GetDestinationPort () << " "
     << m_source << " > " << m_destination
  ;
}

uint32_t 
CodedBulkHeaderFixer::GetSerializedSize (void) const
{
  return 24;
}

void
CodedBulkHeaderFixer::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_verIhl[0]);
  i.WriteU8 (m_verIhl[1]);
  i.WriteU8 (m_verIhl[2]);
  i.WriteU8 (m_verIhl[3]);
  i.WriteHtonU32 (m_ipv4offset);
  i.WriteU8 (m_ttl);
  i.WriteU8 (m_protocol);
  i.WriteHtonU16 (m_checksum);
  i.WriteHtonU32 (m_source.Get ());
  i.WriteHtonU32 (m_destination.Get ());
  i.WriteHtonU16 (m_sourcePort);
  i.WriteHtonU16 (m_destinationPort);
}
uint32_t
CodedBulkHeaderFixer::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_verIhl[0] = i.ReadU8 ();
  m_verIhl[0] = (4 << 4) + m_verIhl[0] % 16;
  m_verIhl[1] = i.ReadU8 ();
  m_verIhl[2] = i.ReadU8 ();
  m_verIhl[3] = i.ReadU8 ();
  m_ipv4offset = i.ReadNtohU32 ();
  m_ttl = i.ReadU8 ();
  m_protocol = i.ReadU8 ();
  m_protocol = TcpL4Protocol::PROT_NUMBER;
  m_checksum = i.ReadU16 ();
  m_source.Set (i.ReadNtohU32 ());
  m_destination.Set (i.ReadNtohU32 ());
  m_sourcePort = i.ReadNtohU16 ();
  m_destinationPort = i.ReadNtohU16 ();

  return GetSerializedSize ();
}

uint8_t 
CodedBulkHeaderFixer::GetProtocol (void) const
{
  NS_LOG_FUNCTION (this);
  return m_protocol;
}
void 
CodedBulkHeaderFixer::SetProtocol (uint8_t protocol)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (protocol));
  m_protocol = protocol;
}

void 
CodedBulkHeaderFixer::SetSource (Ipv4Address source)
{
  NS_LOG_FUNCTION (this << source);
  m_source = source;
}
Ipv4Address
CodedBulkHeaderFixer::GetSource (void) const
{
  NS_LOG_FUNCTION (this);
  return m_source;
}

void 
CodedBulkHeaderFixer::SetDestination (Ipv4Address dst)
{
  NS_LOG_FUNCTION (this << dst);
  m_destination = dst;
}
Ipv4Address
CodedBulkHeaderFixer::GetDestination (void) const
{
  NS_LOG_FUNCTION (this);
  return m_destination;
}

void
CodedBulkHeaderFixer::SetDestinationPort (uint16_t port)
{
  m_destinationPort = port;
}
uint16_t
CodedBulkHeaderFixer::GetDestinationPort (void) const
{
  return m_destinationPort;
}

bool
operator== (const CodedBulkHeaderFixer &lhs, const CodedBulkHeaderFixer &rhs)
{
  return (
    lhs.m_destinationPort == rhs.m_destinationPort
  );
}

std::ostream&
operator<< (std::ostream& os, CodedBulkHeaderFixer const & mpls)
{
  mpls.Print (os);
  return os;
}

} // namespace ns3
