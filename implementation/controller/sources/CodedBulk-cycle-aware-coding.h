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

#ifndef CodedBulk_CYCLE_AWARE_CODING_H
#define CodedBulk_CYCLE_AWARE_CODING_H

#include "CodedBulk-coding-algorithm.h"

/*
 * It is the algorithm generalized from
 * Jaggi et al., ``Polynomial Time Algorithms for Multicast Network Code Construction,'' IEEE Trans. Inf. Theory, 2005.
 * aim to not be constrained by the requirement of topological order
 */
class CodedBulkCycleAwareCoding : public CodedBulkCodingAlgorithm {
public:
  virtual void GenerateCodes (Ptr<CodedBulkTraffic> traffic);
};

#endif /* CodedBulk_CYCLE_AWARE_CODING_H */

