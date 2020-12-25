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

#ifndef CodedBulk_CONTROLLER_H
#define CodedBulk_CONTROLLER_H

#include "ns3/nstime.h"
#include "ns3/type-id.h"
#include "ns3/openflow-routing-controller.h"
#include "ns3/CodedBulk-routing-algorithm.h"
#include "ns3/CodedBulk-coding-algorithm.h"
#include "ns3/CodedBulk-graph.h"
#include "ns3/CodedBulk-traffic.h"
#include <list>

namespace ns3 {

class CodedBulkController : public SimpleRefCount<CodedBulkController> {
public:
  CodedBulkController (void);

  void SetOpenFlowController (Ptr<ofi::OpenFlowRoutingController> controller);
  void SetRoutingAlgorithm (Ptr<CodedBulkRoutingAlgorithm> algorithm);
  void SetCodingAlgorithm (Ptr<CodedBulkCodingAlgorithm> algorithm);
  // need to initialize algorithms whenever resetting the algorithms
  void InitializeAlgorithms ();

  Ptr<CodedBulkGraph> GetCodedBulkGraph (void);
  Ptr<CodedBulkTrafficManager> GetCodedBulkTrafficManager (void);

  void GetPaths (Ptr<CodedBulkTraffic> traffic);
  void ComputeAllPaths (void);

  void GenerateCodes (Ptr<CodedBulkTraffic> traffic);
  void GenerateAllCodes (void);

  void EstablishPaths (Ptr<CodedBulkTraffic> traffic);
  void EstablishAllPaths (void);

  void ResetAlgorithmParameters (void);

  void SetApplicationParameters (
    const TypeId& protocol,
    const Time&   start_time,
    const Time&   stop_time
  );

  void CreateApplication (Ptr<CodedBulkTraffic> traffic);
  void CreateAllApplications (void);

  uint64_t GetTotalProxyOccupation (void) const;

private:
  Ptr<ofi::OpenFlowRoutingController>  m_controller;
  Ptr<CodedBulkRoutingAlgorithm>              m_routing_algorithm;
  Ptr<CodedBulkCodingAlgorithm>               m_coding_algorithm;
  Ptr<CodedBulkGraph>                         m_graph;   // the network topology
  Ptr<CodedBulkTrafficManager>                m_traffic_manager; // the collection of multicast traffic

  // for application setup
  TypeId  m_app_protocol;           //!< Protocol TypeId
  Time    m_app_start_time;
  Time    m_app_stop_time;
  // for sender
  bool    m_applyCodedBulk;   // does the application always apply CodedBulk?
  double  m_CodedBulk_probability;
};

} // namespace ns3

#endif /* CodedBulk_CONTROLLER_H */

