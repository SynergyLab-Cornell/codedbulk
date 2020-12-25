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
#ifndef CodedBulk_SIMPLE_NET_DEVICE_H
#define CodedBulk_SIMPLE_NET_DEVICE_H

#include "ns3/simple-net-device.h"
#include <map>
#include <deque>

namespace ns3 {

class CodedBulkSimpleNetDevice : public SimpleNetDevice
{
public:
  static uint8_t __priority_levels;  // number of priority queues
  static TypeId GetTypeId (void);

  void SetPriorityQueue (uint8_t priority, Ptr<Queue<Packet> > q);

  uint32_t GetNPackets (uint8_t priority) const;

  uint64_t GetMeasuredBytes (void);

  void InitializeMeasureParameters (void);
  void Measuring(void);

protected:
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  virtual void TransmitComplete (void);

  // priority queueing
  std::map<uint8_t, Ptr<Queue<Packet> > > m_priority_queues;

  uint64_t        m_totBytes;      //!< Total bytes received
  uint64_t        m_prevTotBytes;    // last recorded bytes
  uint64_t        m_bytes_interval;   // the bytes received within the interval
};

} // namespace ns3

#endif /* CodedBulk_SIMPLE_NET_DEVICE_H */
