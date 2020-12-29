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

#include "openflow-routing-controller.h"
#include "CodedBulk-routing-algorithm.h"
#include "CodedBulk-coding-algorithm.h"
#include "CodedBulk-graph.h"
#include "CodedBulk-traffic.h"
#include <list>
#include <fstream>
#include <map>

class CodedBulkController : public SimpleRefCount<CodedBulkController> {
public:
  static std::string __simualtion_name;

  CodedBulkController (void);

  void SetOpenFlowController (Ptr<OpenFlowRoutingController> controller);
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

  void CreateApplication (Ptr<CodedBulkTraffic> traffic);
  void CreateAllApplications (void);

  void RegisterProxyAddress (int node, std::string addr);

  void SetInteractiveWorkloadFile (std::string workload_filename);

  void GenerateAllFiles (void);

private:
  Ptr<OpenFlowRoutingController>  m_controller;
  Ptr<CodedBulkRoutingAlgorithm>         m_routing_algorithm;
  Ptr<CodedBulkCodingAlgorithm>          m_coding_algorithm;
  Ptr<CodedBulkGraph>                    m_graph;   // the network topology
  Ptr<CodedBulkTrafficManager>           m_traffic_manager; // the collection of multicast traffic

  std::map<int, std::vector<std::string> >  m_proxy_addresses;

  bool        m_has_workload;
  std::string m_workload_filename;

  // for file generation
  void IncludeAndMain(std::ofstream& fout, std::string library);
  void ExperimentType(std::ofstream& fout);
  void StoreAndForwardCheck(std::ofstream& fout);

  void SetupProxy(std::ofstream& fout, std::vector<std::string>& addresses);
  void SetupCodemaps(std::stringstream& fout, std::list<VirtualLink*>& codemaps);
  void SetupProxySender(
    std::stringstream* modes,
    bool is_interactive,
    bool is_steiner_tree,
    uint32_t path_id
  );
  void SetupProxyNextHop(
    std::stringstream* modes,
    bool is_interactive,
    bool is_steiner_tree,
    uint32_t path_id,
    std::string this_address, // address of this node
    std::string next_hop_address  // address of next hop
  );
  void SetupProxyReceiver(
    std::stringstream* modes,
    bool is_interactive,
    bool is_steiner_tree,
    uint32_t path_id,
    std::string dst_address,
    int application_id
  );
  void SetupProxyPaths(
    std::ofstream& fout,
    std::stringstream* modes
  );


  void SetupSender(
    std::ofstream& fout,
    int application_id,
    bool is_CodedBulk_traffic
  );
  void SetupSenderPaths(
    std::ofstream& fout, 
    std::string src_addreess,
    std::list<int>& dst_ids,
    std::map<int, Ptr<CodedBulkMultipathSet> >& path_sets,
    bool is_interactive
  );
  void SetupSenderSteinerPaths(
    std::ofstream& fout, 
    std::string src_addreess,
    std::list<int>& dst_ids,
    std::map<int, Ptr<CodedBulkMultipathSet> >& path_sets
  );

  void SetupReceiver(
    std::ofstream& fout,
    int application_id,
    std::string local_addreess,
    bool is_CodedBulk_traffic
  );

  void OutputToFile(std::ofstream& fout, std::string prefix, std::string app);
  void EndFile(std::ofstream& fout);
};

#endif /* CodedBulk_CONTROLLER_H */

