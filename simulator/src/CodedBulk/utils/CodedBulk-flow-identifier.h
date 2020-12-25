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

#ifndef CodedBulk_FLOW_IDENTIFIER_H
#define CodedBulk_FLOW_IDENTIFIER_H

#include "ns3/header.h"

namespace ns3 {

class CodedBulkFlowIdentifier : public Header {
public:
    CodedBulkFlowIdentifier();
    CodedBulkFlowIdentifier(const uint32_t c);
    ~CodedBulkFlowIdentifier();

    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual uint32_t GetSerializedSize (void) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);

    void SetValue (uint32_t value);
    uint32_t GetValue (void) const;

    uint32_t m_value;
};

} // namespace ns3

#endif /* CodedBulk_FLOW_IDENTIFIER_H */
