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

#ifndef CodedBulk_ROUTING_ALGORITHM_H
#define CodedBulk_ROUTING_ALGORITHM_H

#include "CodedBulk-algorithm.h"

class CodedBulkRoutingAlgorithm : public CodedBulkAlgorithm {
public:
  CodedBulkRoutingAlgorithm (void);
  virtual ~CodedBulkRoutingAlgorithm ();

  virtual void ResetParameters (void);

  virtual void GetPaths (Ptr<CodedBulkTraffic> traffic);

protected:
  uint32_t NewPathID (void);

  uint32_t m_path_id_counter;
};

#endif /* CodedBulk_ROUTING_ALGORITHM_H */

