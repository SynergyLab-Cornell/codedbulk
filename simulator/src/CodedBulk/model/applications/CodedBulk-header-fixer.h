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

#ifndef CodedBulk_HEADER_FIXER_H
#define CodedBulk_HEADER_FIXER_H

#include "ns3/header.h"
#include "ns3/ipv4-address.h"

namespace ns3 {

class CodedBulkHeaderFixer : public Header {
public:
  CodedBulkHeaderFixer();
  ~CodedBulkHeaderFixer();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetProtocol (uint8_t num);
  uint8_t GetProtocol (void) const;

  void SetSource (Ipv4Address source);
  Ipv4Address GetSource (void) const;

  void SetDestination (Ipv4Address destination);
  Ipv4Address GetDestination (void) const;

  void SetDestinationPort (uint16_t port);
  uint16_t GetDestinationPort (void) const;

  uint8_t  m_verIhl[4];
  uint32_t m_ipv4offset;
  uint8_t  m_ttl;
  uint8_t  m_protocol;
  uint16_t m_checksum;
  Ipv4Address m_source; //!< source address
  Ipv4Address m_destination; //!< destination address
  uint16_t m_sourcePort;   //!< Destination port
  uint16_t m_destinationPort;   //!< Destination port
};

} // namespace ns3

#endif /* CodedBulk_HEADER_FIXER_H */
