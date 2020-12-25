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

#ifndef CodedBulk_GREEDY_ROUTING_H
#define CodedBulk_GREEDY_ROUTING_H

#include "CodedBulk-routing-algorithm.h"

namespace ns3 {

class CodedBulkGreedyRouting : public CodedBulkRoutingAlgorithm {
public:
  virtual void GetPaths (Ptr<CodedBulkTraffic> traffic);

private:
  void GetPaths (Ptr<CodedBulkTraffic> traffic, int dst_id);
};

} // namespace ns3

#endif /* CodedBulk_GREEDY_ROUTING_H */

