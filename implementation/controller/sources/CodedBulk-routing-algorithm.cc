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
#include "CodedBulk-routing-algorithm.h"

CodedBulkRoutingAlgorithm::CodedBulkRoutingAlgorithm (void) :
  m_path_id_counter(1000)  // cannot use port 0
{}

CodedBulkRoutingAlgorithm::~CodedBulkRoutingAlgorithm () {}

void
CodedBulkRoutingAlgorithm::ResetParameters (void)
{
  m_path_id_counter = 1000;
}

void CodedBulkRoutingAlgorithm::GetPaths (Ptr<CodedBulkTraffic> traffic) {}

uint32_t
CodedBulkRoutingAlgorithm::NewPathID (void)
{
  uint32_t id = m_path_id_counter;
  m_path_id_counter += 2;
  return id;
}