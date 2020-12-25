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
#include "system_parameters.h"
#include "CodedBulk-controller.h"
#include "CodedBulk-greedy-routing.h"
#include "CodedBulk-Watel2014-Steiner-routing.h"
#include "CodedBulk-cycle-aware-coding.h"
#include <limits>
#include <string>
#include <sstream>

std::string CodedBulkController::__simualtion_name = "";

CodedBulkController::CodedBulkController (void) : 
  m_has_workload(false)
{
  // create default CodedBulk algorithms
  SetRoutingAlgorithm (Create<CodedBulkGreedyRouting> ());
  SetCodingAlgorithm (Create<CodedBulkCycleAwareCoding> ());
  m_graph = Create<CodedBulkGraph> ();
  m_traffic_manager = Create<CodedBulkTrafficManager> ();
  m_proxy_addresses.clear();
}

void
CodedBulkController::SetOpenFlowController (Ptr<OpenFlowRoutingController> controller)
{
  m_controller = controller;
  InitializeAlgorithms ();
}

void
CodedBulkController::SetRoutingAlgorithm (Ptr<CodedBulkRoutingAlgorithm> algorithm)
{
  m_routing_algorithm = algorithm;
}

void
CodedBulkController::SetCodingAlgorithm (Ptr<CodedBulkCodingAlgorithm> algorithm)
{
  m_coding_algorithm = algorithm;
}

void
CodedBulkController::InitializeAlgorithms ()
{
  m_routing_algorithm->Initialize(m_controller, m_graph);
  m_coding_algorithm->Initialize(m_controller, m_graph);
}

Ptr<CodedBulkGraph>
CodedBulkController::GetCodedBulkGraph (void)
{
  return m_graph;
}

Ptr<CodedBulkTrafficManager>
CodedBulkController::GetCodedBulkTrafficManager (void)
{
  return m_traffic_manager;
}

void
CodedBulkController::GetPaths (Ptr<CodedBulkTraffic> traffic)
{
  m_routing_algorithm->GetPaths (traffic);
}

void
CodedBulkController::ComputeAllPaths (void) {
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it  = m_traffic_manager->m_traffic.begin();
    it != m_traffic_manager->m_traffic.end();
    ++it
  ) {
    GetPaths(*it);
    if((*it)->_is_unicast) {
      // throw all other paths away
      for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
        it_map  = (*it)->_path_sets.begin();
        it_map != (*it)->_path_sets.end();
        ++it_map
      ) {
        if( it_map->second->_paths.size() > 1 ) {
          it_map->second->_paths.resize(1);
        }
      }
    }
  }
}

void
CodedBulkController::GenerateCodes (Ptr<CodedBulkTraffic> traffic)
{
  m_coding_algorithm->GenerateCodes (traffic);
}

void
CodedBulkController::GenerateAllCodes (void)
{
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it  = m_traffic_manager->m_traffic.begin();
    it != m_traffic_manager->m_traffic.end();
    ++it
  ) {
    if((*it)->_is_CodedBulk_traffic) {
      GenerateCodes(*it);
    }
  }
}

void
CodedBulkController::EstablishPaths (Ptr<CodedBulkTraffic> traffic)
{
  for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
    it  = traffic->_path_sets.begin();
    it != traffic->_path_sets.end();
    ++it
  ) {
    for(std::vector<Ptr<CodedBulkUnicastPath> >::iterator
      it_path  = it->second->_paths.begin();
      it_path != it->second->_paths.end();
      ++it_path
    ) {
      // establish coded/non coded paths
      m_controller->EstablishPath(*it_path, traffic->_is_CodedBulk_traffic);
    }
  }
}

void
CodedBulkController::EstablishAllPaths (void)
{
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it  = m_traffic_manager->m_traffic.begin();
    it != m_traffic_manager->m_traffic.end();
    ++it
  ) {
    EstablishPaths(*it);
  }
}

void
CodedBulkController::ResetAlgorithmParameters (void)
{
  m_coding_algorithm->ResetParameters();
  m_routing_algorithm->ResetParameters();
}

void
CodedBulkController::CreateApplication (Ptr<CodedBulkTraffic> traffic)
{
/*
  // setup the sender
  // using CodedBulkBulkSendApplication as the default
  uint16_t application_id = (uint16_t)traffic->_id + 1000;

  Ptr<CodedBulkMulticastSender>& sender = traffic->_sender;
  Ptr<CodedBulkBulkSendApplication> bulk_send = CreateObject<CodedBulkBulkSendApplication> ();
  if((!traffic->_is_CodedBulk_traffic) && (traffic->_priority == CodedBulkTraffic::Interactive)) {
    for(std::list<CodedBulkTraffic::CodedBulkTrafficLoad>::iterator
      it  = traffic->_interactive_workload.begin();
      it != traffic->_interactive_workload.end();
      ++it
    ) {
      bulk_send->InteractiveInputAt( MilliSeconds((*it)._time_ms), (*it)._size );
    }
  }

  sender = bulk_send;
  sender->SetApplicationID (application_id);
  sender->SetThroughProxy  (traffic->_is_CodedBulk_traffic || !traffic->_is_unicast);
  sender->SetProtocol  (m_app_protocol);
  sender->SetStartTime (m_app_start_time);
  sender->SetStopTime  (m_app_stop_time);
  sender->SetPriority  (traffic->_priority);

  std::list<Ipv4Address> remote_addresses;

  if(traffic->_is_CodedBulk_traffic || !traffic->_is_unicast) {
    Ipv4Address src_proxy_address = m_controller->GetProxyAt(traffic->_src_id)->GetBaseAddr();

    for(std::list<int>::iterator
      it_dst_id  = traffic->_dst_id.begin();
      it_dst_id != traffic->_dst_id.end();
      ++it_dst_id
    ) {
      remote_addresses.push_back(src_proxy_address);
    }
  } else {
    for(std::list<int>::iterator
      it_dst_id  = traffic->_dst_id.begin();
      it_dst_id != traffic->_dst_id.end();
      ++it_dst_id
    ) {
      remote_addresses.push_back(m_controller->GetHostAddress(*it_dst_id));
    }
  }

  sender->SetRemoteAddresses (remote_addresses);

  // establish the multicast path (tree-like)
  // only pick the same number of paths from each multipath set
  uint32_t h = std::numeric_limits<uint32_t>::max();
  std::map<int, Ptr<CodedBulkMultipathSet> >& path_sets = traffic->_path_sets;
  for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
    it  = path_sets.begin();
    it != path_sets.end();
    ++it) {
    if(h > it->second->_paths.size()) {
      h = it->second->_paths.size();
    }
  }
  // setup the multicast paths
  for(uint32_t i = 0; i < h; ++i) {
    std::list<uint32_t> path_ids;
    path_ids.clear();
    for(std::list<int>::iterator
      it_dst_id  = traffic->_dst_id.begin();
      it_dst_id != traffic->_dst_id.end();
      ++it_dst_id
    ) {
      path_ids.push_back(path_sets[*it_dst_id]->_paths[i]->_path_id);
    }
    sender->AddMulticastPath(path_ids);
  }

  Ptr<Node>& host_src = m_graph->_all_nodes[traffic->_src_id]._host;
  if(host_src != nullptr) {
    host_src->AddApplication(sender);
  }

  // setup the receivers
  traffic->_receivers.clear();
  for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
    it  = traffic->_path_sets.begin();
    it != traffic->_path_sets.end();
    ++it
  ) {
    Ptr<CodedBulkReceiver> receiver = CreateObject<CodedBulkReceiver> ();
    receiver->SetApplicationID (application_id);
    receiver->SetThroughProxy  (traffic->_is_CodedBulk_traffic || !traffic->_is_unicast);    receiver->SetProtocol  (m_app_protocol);
    receiver->SetStartTime (m_app_start_time);
    receiver->SetStopTime  (m_app_stop_time);
    receiver->SetPriority  (traffic->_priority);

    receiver->SetLocal(m_controller->GetHostAddress(it->first));

    traffic->_receivers.push_back(receiver);
    Ptr<Node>& host_dst = m_graph->_all_nodes[it->first]._host;
    if(host_dst != nullptr) {
      host_dst->AddApplication(receiver);
    }
  }
*/
}

void
CodedBulkController::CreateAllApplications (void)
{
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it  = m_traffic_manager->m_traffic.begin();
    it != m_traffic_manager->m_traffic.end();
    ++it
  ) {
    CreateApplication(*it);
  }
}

void
CodedBulkController::RegisterProxyAddress (int node, std::string addr)
{
  if(m_proxy_addresses.find(node) == m_proxy_addresses.end()) {
    m_proxy_addresses[node].clear();
  }
  for(auto&& existing_addr : m_proxy_addresses[node]) {
    if(addr.compare(existing_addr) == 0) {
      // the address has been registered
      return;
    }
  }
  m_proxy_addresses [node].emplace_back(addr);
}

void
CodedBulkController::SetInteractiveWorkloadFile (std::string workload_filename)
{
  m_workload_filename = "workloads/"+workload_filename+".txt";
  m_has_workload = true;
}

void
CodedBulkController::GenerateAllFiles (void)
{
  // Create folders
  std::string folder_base = "../apps/"+__simualtion_name;
  const int num_proxies = m_proxy_addresses.size();
  // Create makefile
  system(("mkdir -p "+folder_base+"/results").c_str());
  system(("cp ../apps/Makefile.temp "+folder_base+"/Makefile").c_str());
  system(("touch "+folder_base+"/results/__PLACE_HOLDER__").c_str());

  // Generate interactive applications
  if(m_has_workload) {
    system(("mkdir -p "+folder_base+"/workloads").c_str());
    // clean carefully
    system(("rm -f "+folder_base+"/workloads/*").c_str());

    Ptr<CodedBulkTraffic>** interactive_traffic = new Ptr<CodedBulkTraffic>*[num_proxies];
    std::ofstream** finteractive_traffic = new std::ofstream*[num_proxies];
    for(int fs = 0; fs < num_proxies; ++fs) {
      interactive_traffic[fs] = new Ptr<CodedBulkTraffic>[num_proxies];
      finteractive_traffic[fs] = new std::ofstream[num_proxies];
      for(int fd = 0; fd < num_proxies; ++fd) {
        interactive_traffic[fs][fd] = nullptr;
      }
    }

    std::ifstream workload(m_workload_filename.c_str());
    int num_nodes = 0;
    int num_flows = 0;
    workload >> num_nodes >> num_flows;
    if( num_nodes == num_proxies ) {
      int int_flow_id;
      long long int int_time_ms;
      int int_src_id;
      int int_dst_id;
      uint64_t int_flow_size;
      for(int f = 0; f < num_flows; ++f) {
        //<Flow ID> <Time of arrival (ms)> <Source ID> <Destination ID> <Flow Size (KB)>
        workload >> int_flow_id >> int_time_ms >> int_src_id >> int_dst_id >> int_flow_size;
        int_flow_size *= 1000;
        if(interactive_traffic[int_src_id][int_dst_id] == nullptr) {
          interactive_traffic[int_src_id][int_dst_id] = m_traffic_manager->addCodedBulkTraffic(int_src_id);
          interactive_traffic[int_src_id][int_dst_id]->addDst(int_dst_id);
          interactive_traffic[int_src_id][int_dst_id]->ApplyCodedBulk(false);
          interactive_traffic[int_src_id][int_dst_id]->SetUnicast(true);
          interactive_traffic[int_src_id][int_dst_id]->SetPriority(CodedBulkTraffic::Interactive);

          std::stringstream convert_workload;
          convert_workload << folder_base << "/workloads/interactive_traffic" << interactive_traffic[int_src_id][int_dst_id]->_id << ".dat";
          finteractive_traffic[int_src_id][int_dst_id].open(convert_workload.str().c_str());
        }
        finteractive_traffic[int_src_id][int_dst_id] << int_time_ms << " " << int_flow_size << std::endl;
      }
    }
    workload.close();

    for(int fs = 0; fs < num_proxies; ++fs) {
      for(int fd = 0; fd < num_proxies; ++fd) {
        if(interactive_traffic[fs][fd] != nullptr) {
          finteractive_traffic[fs][fd].close();
        }
      }
    }

    for(int fs = 0; fs < num_proxies; ++fs) {
      delete [] interactive_traffic[fs];
      delete [] finteractive_traffic[fs];
    }
    delete [] interactive_traffic;
    delete [] finteractive_traffic;
  }

  std::ofstream* fproxy = new std::ofstream[num_proxies];
  std::stringstream** ssproxy_modes = new std::stringstream*[num_proxies];
  std::string*   folder = new std::string[num_proxies];

  // create files
  for(int i = 0; i < num_proxies; ++i) {
    std::stringstream convert_folder;
    convert_folder << folder_base << "/v" << i;
    folder[i] = convert_folder.str();

    system(("mkdir -p "+folder[i]).c_str());
    // clean carefully
    system(("rm -f "+folder[i]+"/*").c_str());

    fproxy[i].open((folder[i]+"/proxy.cc").c_str());
    if(!fproxy[i]) {
      delete [] fproxy;
      delete [] folder;
      perror("Cannot open file");
      return;
    }

    ssproxy_modes[i] = new std::stringstream[5]; 
    // for different modes
    // interactive, single path and multipath, codedbulk, steiner tree, steiner tree noncoded
  }

  const int num_traffic = m_traffic_manager->getNumCodedBulkTraffic();
  std::ofstream*  ftraffic       = new std::ofstream[num_traffic];
  std::ofstream** ftraffic_recv = new std::ofstream*[num_traffic];
  int traf_id = 0;
  int traf_recv_id = 0;
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it  = m_traffic_manager->m_traffic.begin();
    it != m_traffic_manager->m_traffic.end();
    ++it, ++traf_id
  ) {  
    std::stringstream convert;
    convert << folder[(*it)->_src_id] 
            << "/traffic" << (*it)->_id << "_send.cc";
    ftraffic[traf_id].open(convert.str().c_str());

    ftraffic_recv[traf_id] = new std::ofstream[(*it)->GetNumDst()];
    traf_recv_id = 0;
    for(std::list<int>::iterator
      it_dst  = (*it)->_dst_id.begin();
      it_dst != (*it)->_dst_id.end();
      ++it_dst, ++traf_recv_id
    ) {
      std::stringstream convert_recv;
      convert_recv << folder[(*it_dst)] 
                   << "/traffic" << (*it)->_id << "_recv.cc";
      ftraffic_recv[traf_id][traf_recv_id].open (convert_recv.str().c_str());
    }
  }

  // setup the files
  for(int i = 0; i < num_proxies; ++i) {
    IncludeAndMain(fproxy[i],"proxy");
    StoreAndForwardCheck(fproxy[i]);
    SetupProxy(fproxy[i],m_proxy_addresses[i]);
  }

  traf_id = 0;
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it  = m_traffic_manager->m_traffic.begin();
    it != m_traffic_manager->m_traffic.end();
    ++it, ++traf_id
  ) {  
    IncludeAndMain(ftraffic[traf_id],"bulk-send-application");
    SetupSender(
      ftraffic[traf_id],
      (*it)->_id + PROXY_PORT + 1,
      (*it)->_is_CodedBulk_traffic
    );

    traf_recv_id = 0;
    for(std::list<int>::iterator
      it_dst  = (*it)->_dst_id.begin();
      it_dst != (*it)->_dst_id.end();
      ++it_dst, ++traf_recv_id
    ) {
      IncludeAndMain(ftraffic_recv[traf_id][traf_recv_id],"receiver");
      SetupReceiver(
        ftraffic_recv[traf_id][traf_recv_id],
        (*it)->_id + PROXY_PORT + 1,
        m_proxy_addresses[(*it_dst)].front(),
        (*it)->_is_CodedBulk_traffic
      );
    }
  }

  SetRoutingAlgorithm (Create<CodedBulkGreedyRouting> ());
  InitializeAlgorithms();

  // generation
  ComputeAllPaths ();
  GenerateAllCodes ();

  // setup sender paths
  traf_id = 0;
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it  = m_traffic_manager->m_traffic.begin();
    it != m_traffic_manager->m_traffic.end();
    ++it, ++traf_id
  ) {
    SetupSenderPaths(
      ftraffic[traf_id],
      m_proxy_addresses[(*it)->_src_id].front(),
      (*it)->_dst_id,
      (*it)->_path_sets,
      ((*it)->_priority == CodedBulkTraffic::Interactive)
    );
  }

  // setup proxy paths
  for(int i = 0; i < num_proxies; ++i) {
    SetupCodemaps(ssproxy_modes[i][2],m_controller->m_all_codecs[i]);
  }
  // paths
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it_traffic  = m_traffic_manager->m_traffic.begin();
    it_traffic != m_traffic_manager->m_traffic.end();
    ++it_traffic
  ) {
    bool is_interactive = ((*it_traffic)->_priority == CodedBulkTraffic::Interactive);
    for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
      it_ps  = (*it_traffic)->_path_sets.begin();
      it_ps != (*it_traffic)->_path_sets.end();
      ++it_ps
    ) {
      for(std::vector<Ptr<CodedBulkUnicastPath> >::iterator
        it_path  = it_ps->second->_paths.begin();
        it_path != it_ps->second->_paths.end();
        ++it_path
      ) {
        CodedBulkUnicastPath& path = (*(*it_path));
        if( path._nodes.empty() ) {
          continue;
        }
        // establish coded/non coded paths
        std::list<int>::iterator it = path._nodes.begin();
        std::list<int>::iterator it_next = path._nodes.begin(); 
        ++it_next;

        SetupProxySender(
          ssproxy_modes[*it],
          is_interactive,
          false,
          path._path_id
        );
        for(; it_next != path._nodes.end(); ++it, ++it_next)
        {
          CodedBulkEdge& e = m_graph->_all_edges[m_graph->findEdgeId(*it,*it_next)];
          if(e._use_edge_ip) {
            SetupProxyNextHop(
              ssproxy_modes[*it],
              is_interactive,
              false,
              path._path_id,
              e._node_head_ip, // address of this node
              e._node_tail_ip  // address of next hop
            );
          } else {
            SetupProxyNextHop(
              ssproxy_modes[*it],
              is_interactive,
              false,
              path._path_id,
              m_proxy_addresses[*it].front(), // address of this node
              m_proxy_addresses[*it_next].front()  // address of next hop
            );
          }
        }
        SetupProxyReceiver(
          ssproxy_modes[*it],
          is_interactive,
          false,
          path._path_id,
          m_proxy_addresses[*it].front(),
          (*it_traffic)->_id + PROXY_PORT + 1
        );
      }
    }
  }

  // get Steiner tree proxies
  SetRoutingAlgorithm (Create<CodedBulkWatel2014SteinerRouting> ());
  InitializeAlgorithms();
  // reset codecs
  m_controller->m_all_codecs.clear();

  // generation
  ComputeAllPaths ();
  GenerateAllCodes ();  

  // setup sender Steiner paths
  traf_id = 0;
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it  = m_traffic_manager->m_traffic.begin();
    it != m_traffic_manager->m_traffic.end();
    ++it, ++traf_id
  ) {
    if ((*it)->_priority == CodedBulkTraffic::Interactive) {
      continue;
    }
    SetupSenderSteinerPaths(
      ftraffic[traf_id],
      m_proxy_addresses[(*it)->_src_id].front(),
      (*it)->_dst_id,
      (*it)->_path_sets
    );
  }

  // setup steiner tree paths
  for(int i = 0; i < num_proxies; ++i) {
    SetupCodemaps(ssproxy_modes[i][3],m_controller->m_all_codecs[i]);
  }
  // paths
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it_traffic  = m_traffic_manager->m_traffic.begin();
    it_traffic != m_traffic_manager->m_traffic.end();
    ++it_traffic
  ) {
    bool is_interactive = ((*it_traffic)->_priority == CodedBulkTraffic::Interactive);
    if (is_interactive)  continue; // interactive traffic has been taken care of
    for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
      it_ps  = (*it_traffic)->_path_sets.begin();
      it_ps != (*it_traffic)->_path_sets.end();
      ++it_ps
    ) {
      for(std::vector<Ptr<CodedBulkUnicastPath> >::iterator
        it_path  = it_ps->second->_paths.begin();
        it_path != it_ps->second->_paths.end();
        ++it_path
      ) {
        CodedBulkUnicastPath& path = (*(*it_path));
        if( path._nodes.empty() ) {
          continue;
        }
        // establish coded/non coded paths
        std::list<int>::iterator it = path._nodes.begin();
        std::list<int>::iterator it_next = path._nodes.begin(); 
        ++it_next;

        SetupProxySender(
          ssproxy_modes[*it],
          false,
          true,
          path._path_id
        );
        for(; it_next != path._nodes.end(); ++it, ++it_next)
        {
          CodedBulkEdge& e = m_graph->_all_edges[m_graph->findEdgeId(*it,*it_next)];
          if(e._use_edge_ip) {
            SetupProxyNextHop(
              ssproxy_modes[*it],
              false,
              true,
              path._path_id,
              e._node_head_ip, // address of this node
              e._node_tail_ip  // address of next hop
            );
          } else {
            SetupProxyNextHop(
              ssproxy_modes[*it],
              false,
              true,
              path._path_id,
              m_proxy_addresses[*it].front(), // address of this node
              m_proxy_addresses[*it_next].front()  // address of next hop
            );
          }
        }
        SetupProxyReceiver(
          ssproxy_modes[*it],
          false,
          true,
          path._path_id,
          m_proxy_addresses[*it].front(),
          (*it_traffic)->_id + PROXY_PORT + 1
        );
      }
    }
  }

  // close
  traf_id = 0;
  for(std::list<Ptr<CodedBulkTraffic> >::iterator
    it  = m_traffic_manager->m_traffic.begin();
    it != m_traffic_manager->m_traffic.end();
    ++it, ++traf_id
  ) {
    ftraffic[traf_id] << 
"  sender->StartApplication ();\n";
    if((*it)->_priority == CodedBulkTraffic::Interactive) {
      ftraffic[traf_id] << 
"  sender->StartInteractiveInput (\"workloads/interactive_traffic" << (*it)->_id << ".dat\");\n";
    }

    std::stringstream prefix;
    if((*it)->_priority == CodedBulkTraffic::Interactive) {
      prefix << "interactive_traffic" << (*it)->_id << "_send";
    } else {
      prefix << "traffic" << (*it)->_id << "_send";
    }
    OutputToFile(ftraffic[traf_id], prefix.str(), "sender");
    EndFile(ftraffic[traf_id]);

    traf_recv_id = 0;
    for(std::list<int>::iterator
      it_dst  = (*it)->_dst_id.begin();
      it_dst != (*it)->_dst_id.end();
      ++it_dst, ++traf_recv_id
    ) {
      ftraffic_recv[traf_id][traf_recv_id] <<
"  receiver->StartApplication ();\n";

      std::stringstream prefix;
      if((*it)->_priority == CodedBulkTraffic::Interactive) {
        prefix << "interactive_traffic" << (*it)->_id << "_recv" << traf_recv_id;
      } else {
        prefix << "traffic" << (*it)->_id << "_recv" << traf_recv_id;
      }
      OutputToFile(ftraffic_recv[traf_id][traf_recv_id], prefix.str(), "receiver");
      EndFile(ftraffic_recv[traf_id][traf_recv_id]);
    }
    delete [] ftraffic_recv[traf_id];
  }
  delete [] ftraffic_recv;
  delete [] ftraffic;
  
  for(int i = 0; i < num_proxies; ++i) {
    SetupProxyPaths(fproxy[i],ssproxy_modes[i]);

    fproxy[i] << 
"  proxy->SetCodecManager (codec_manager);\n" <<
"  proxy->StartProxy ();\n" <<
"  system_parameters.waitForInputThread();\n" << 
"  proxy->WakeAllThreads();\n" <<
"  if (scenario == 6) {\n" <<
"    if (proxy->IsRecvBufferFull()){\n" <<
"      std::ofstream ferr(\"results/Steiner_tree_buffer_full\");\n" <<
"      ferr.close();\n" <<
"      return -1;\n" <<
"    }\n" <<
"  }\n";
// patched version
//"  std::this_thread::sleep_for(std::chrono::seconds(60));\n" <<
//"  exit(0);\n";
    EndFile(fproxy[i]);

    delete [] ssproxy_modes[i];
  }
  delete [] fproxy;
  delete [] ssproxy_modes;
  delete [] folder;

  // output traffic_summary
  std::stringstream summary_name;
  summary_name << folder_base << "/traffic_summary.txt";
  std::ofstream traffic_summary(summary_name.str().c_str());
  
  m_traffic_manager->listTrafficSummary(traffic_summary);

  traffic_summary.close();
}

void
CodedBulkController::IncludeAndMain(std::ofstream& fout, std::string library)
{
  fout <<
"#include \"measure_tools.h\"\n" <<
"#include \"CodedBulk-" << library << ".h\"\n" <<
"extern SystemParameters system_parameters;\n" <<
"int main(int argc, char* argv[]){\n" <<
"  system_parameters.setRunning(true);\n";
  ExperimentType(fout);

/* 
  // patched version
  fout <<
"#include \"measure_tools.h\"\n";

  if (library == "proxy") {
    fout <<
"#include <chrono>\n" <<
"#include <thread>\n";
  }

  fout <<
"#include \"CodedBulk-" << library << ".h\"\n" <<
"int main(int argc, char* argv[]){\n";
  ExperimentType(fout);
*/
}

void
CodedBulkController::SetupProxy(std::ofstream& fout, std::vector<std::string>& addresses)
{
  fout <<
"  Ptr<CodedBulkProxy> proxy = Create<CodedBulkProxy> ();\n";
  for(std::string& address : addresses) {
    fout <<
"  proxy->SetBaseAddr (IpToAddress(\"" << address << "\"));\n";
  }
  fout <<
"  Ptr<CodedBulkCodecManager> codec_manager = Create<CodedBulkCodecManager> ();\n" << 
"  codec_manager->SetMaxNumWorker (10);\n" <<
"  VirtualLink* code_map;\n";
}

void
CodedBulkController::SetupCodemaps(std::stringstream& fout, std::list<VirtualLink*>& codemaps)
{
  for(std::list<VirtualLink*>::iterator
    it_c  = codemaps.begin();
    it_c != codemaps.end();
    ++it_c
  ) {
    int row_dimension = (*it_c)->getRowDimension();
    int col_dimension = (*it_c)->getColDimension();
    fout << 
"      code_map = new VirtualLink (" << row_dimension << "," << col_dimension << ");\n";
    for (int ri = 0; ri < row_dimension; ++ri) {
      for (int cj = 0; cj < col_dimension; ++cj) {
        fout << 
"      (*code_map)[" << ri << "][" << cj << "] = " << (int) +(*(*it_c))[ri][cj] << ";\n";
      }
    }
    fout << 
"      code_map->_input_paths ";
    for(int j = 0; j < (*it_c)->_input_paths._size; ++j) {
      fout << 
" << " << (*it_c)->_input_paths._path_ids[j];
    }

    fout << 
";\n      code_map->_output_paths";
    for(int j = 0; j < (*it_c)->_output_paths._size; ++j) {
      fout << 
" << " << (*it_c)->_output_paths._path_ids[j];
    }
    fout << 
";\n" <<
"      codec_manager->addCodedBulkCodec (code_map);\n";
  }
}

void
CodedBulkController::SetupProxySender(
  std::stringstream* modes,
  bool is_interactive,
  bool is_steiner_tree,
  uint32_t path_id
){
  if(is_interactive) {
    // interactive: use small buffers for every node
    //modes[0] <<
//"  proxy->RegisterSender (" << path_id << ");\n";
  } else if (is_steiner_tree) {
    // coded Steiner tree
    modes[3] <<
"      proxy->RegisterSender (" << path_id + 1 << ");\n";
    // noncoded Steiner tree
    modes[4] <<
"      proxy->RegisterSender (" << path_id << ");\n";
  } else {
    // single path and multipath
    modes[1] <<
"      proxy->RegisterSender (" << path_id << ");\n";
    // coded
    modes[2] <<
"      proxy->RegisterSender (" << path_id + 1 << ");\n";
  }
}

void
CodedBulkController::SetupProxyNextHop(
  std::stringstream* modes,
  bool is_interactive,
  bool is_steiner_tree,
  uint32_t path_id,
  std::string this_address, // address of this node
  std::string next_hop_address  // address of next hop
){
  if(is_interactive) {
    // interactive
    modes[0] <<
"  proxy->RegisterSender (" << path_id << ");\n" <<
"  proxy->RegisterProxyAddr (" << path_id << 
",IpToAddress(\"" << this_address << "\")" <<
",IpToAddress(\"" << next_hop_address << "\"));\n" << 
"  proxy->RegisterPriority ("  << path_id << ",6);\n" <<
"  proxy->SetForwardRule ("  << path_id << "," << path_id << ");\n";
  } else if (is_steiner_tree) {
    // here the content is different from modes[2] as the codecs are recomputed
    // coded Steiner tree
    modes[3] <<
"      proxy->RegisterProxyAddr (" << path_id + 1 <<
",IpToAddress(\"" << this_address << "\")" <<
",IpToAddress(\"" << next_hop_address << "\"));\n" << 
"      proxy->RegisterPriority("  << path_id + 1 << ",4);\n";
    // noncoded Steiner tree
    modes[4] <<
"      proxy->RegisterProxyAddr (" << path_id << 
",IpToAddress(\"" << this_address << "\")" <<
",IpToAddress(\"" << next_hop_address << "\"));\n" << 
"      proxy->RegisterPriority ("  << path_id << ",2);\n" <<
"      proxy->SetForwardRule ("  << path_id << "," << path_id << ");\n";
  } else {
    // single path and multipath
    modes[1] <<
"      proxy->RegisterProxyAddr (" << path_id << 
",IpToAddress(\"" << this_address << "\")" <<
",IpToAddress(\"" << next_hop_address << "\"));\n" << 
"      proxy->RegisterPriority ("  << path_id << ",2);\n" <<
"      proxy->SetForwardRule ("  << path_id << "," << path_id << ");\n";
    // coded
    modes[2] <<
"      proxy->RegisterProxyAddr (" << path_id + 1 <<
",IpToAddress(\"" << this_address << "\")" <<
",IpToAddress(\"" << next_hop_address << "\"));\n" << 
"      proxy->RegisterPriority("  << path_id + 1 << ",4);\n";
  }
}

void
CodedBulkController::SetupProxyReceiver(
  std::stringstream* modes,
  bool is_interactive,
  bool is_steiner_tree,
  uint32_t path_id,
  std::string dst_address,
  int application_id
){
  if(is_interactive) {
    // interactive
    modes[0] <<
"  proxy->RegisterReceiver (" << path_id << ",IpToAddress(\"" << dst_address << "\")," << application_id << ");\n" <<
"  proxy->RegisterPriority (" << path_id << ",6);\n" <<
"  proxy->SetForwardRule ("  << path_id << "," << path_id << ");\n";
  } else if (is_steiner_tree) {
    // coded Steiner tree
    modes[3] <<
"      proxy->RegisterReceiver (" << path_id + 1 << ",IpToAddress(\"" << dst_address << "\")," << application_id << ");\n" <<
"      proxy->RegisterPriority (" << path_id + 1 << ",4);\n";
    // noncoded Steiner tree
    modes[4] <<
"      proxy->RegisterReceiver (" << path_id << ",IpToAddress(\"" << dst_address << "\")," << application_id << ");\n" <<
"      proxy->RegisterPriority (" << path_id << ",2);\n" <<
"      proxy->SetForwardRule ("  << path_id << "," << path_id << ");\n";
  } else {
    // single path and multipath
    modes[1] <<
"      proxy->RegisterReceiver (" << path_id << ",IpToAddress(\"" << dst_address << "\")," << application_id << ");\n" <<
"      proxy->RegisterPriority (" << path_id << ",2);\n" <<
"      proxy->SetForwardRule ("  << path_id << "," << path_id << ");\n";
    // coded
    modes[2] <<
"      proxy->RegisterReceiver (" << path_id + 1 << ",IpToAddress(\"" << dst_address << "\")," << application_id << ");\n" <<
"      proxy->RegisterPriority (" << path_id + 1 << ",4);\n";
  }
}

void
CodedBulkController::SetupProxyPaths(
  std::ofstream& fout,
  std::stringstream* modes
){
  /*
   * scenario:
   *   0: Steiner_tree_multicast
   *   1: single_path_multicast
   *   2: multi_path_multicast
   *   3: coded_multicast
   *   4: Steiner_tree_without_merging_(coding)
   *   5: Steiner_tree_store-and-forward_disk
   *   6: Steiner_tree_store-and-forward_memory
   */
  // interactive
  fout << modes[0].str() <<
"  switch (scenario) {\n" <<
"    case 0:\n" << // Steiner Tree
"    case 5:\n" << // Steiner_tree_store-and-forward_disk
"    case 6:\n" << // Steiner_tree_store-and-forward_memory
       modes[3].str() <<
"      break;\n" <<
"    case 3:\n" << // CodedBulk
       modes[2].str() <<
"    case 1:\n" << // single path
"    case 2:\n" << // multipath
       modes[1].str() <<
"      break;\n" <<
"    case 4:\n" << // Steiner Tree noncoded
       modes[4].str() <<
"      break;\n" <<
"  }\n";
}

void
CodedBulkController::SetupSender(
  std::ofstream& fout,
  int application_id,
  bool is_CodedBulk_traffic
){
  fout <<
"  Ptr<CodedBulkBulkSendApplication> sender = Create<CodedBulkBulkSendApplication> ();\n" <<
"  sender->SetApplicationID (" << application_id << ");\n";
  if(is_CodedBulk_traffic) {
    fout <<
"  sender->SetPriority (2);\n" <<
"  sender->SetThroughProxy (true);\n";
  } else {
    fout <<
"  sender->SetPriority (6);\n" <<
"  sender->SetThroughProxy (true);\n";
  }
}


void
CodedBulkController::SetupSenderPaths(
  std::ofstream& fout, 
  std::string src_addreess,
  std::list<int>& dst_ids,
  std::map<int, Ptr<CodedBulkMultipathSet> >& path_sets,
  bool is_interactive
){
  fout <<
"  std::list<Address> remote_addresses;\n";
  for(std::list<int>::iterator
    it_dst  = dst_ids.begin();
    it_dst != dst_ids.end();
    ++it_dst
  ) {
    fout <<
"  remote_addresses.push_back (IpToAddress(\"" << src_addreess << "\"));\n";
  }
  fout <<
"  sender->SetRemoteAddresses (remote_addresses);\n";

  uint32_t h = std::numeric_limits<uint32_t>::max();
  for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
    it_p  = path_sets.begin();
    it_p != path_sets.end();
    ++it_p) {
    if(h > it_p->second->_paths.size()) {
      h = it_p->second->_paths.size();
    }
  }

  // setup the multicast paths
  fout <<
"  std::list<uint32_t> path_ids;\n";

  if (is_interactive) {
    if (h > 0) {
      // just to make sure we have at least one path
      fout <<
"  path_ids.clear();\n";
      for(std::list<int>::iterator
        it_dst_id  = dst_ids.begin();
        it_dst_id != dst_ids.end();
        ++it_dst_id
      ) {
        fout <<
"  path_ids.push_back (" << path_sets[*it_dst_id]->_paths[0]->_path_id << ");\n";
      }
      fout <<
"  sender->AddMulticastPath (path_ids, 6);\n";
    }
  } else {
    fout <<
"  switch (scenario){\n";
    if (h > 0) {
      fout <<
"    case 1:\n" <<
    // just to make sure we have at least one path
"      path_ids.clear();\n";
      for(std::list<int>::iterator
        it_dst_id  = dst_ids.begin();
        it_dst_id != dst_ids.end();
        ++it_dst_id
      ) {
        fout <<
"      path_ids.push_back (" << path_sets[*it_dst_id]->_paths[0]->_path_id << ");\n";
      }
      fout << 
"      sender->AddMulticastPath (path_ids, path_type);\n" <<
"      break;\n";
    }
    fout <<
"    case 2:\n" <<
"    case 3:\n";
    for(uint32_t i = 0; i < h; ++i) {
        fout <<
"      path_ids.clear();\n";
      for(std::list<int>::iterator
        it_dst_id  = dst_ids.begin();
        it_dst_id != dst_ids.end();
        ++it_dst_id
      ) {
        fout <<
"      path_ids.push_back (" << path_sets[*it_dst_id]->_paths[i]->_path_id << ");\n";
      }
    fout <<
"      sender->AddMulticastPath (path_ids, path_type);\n";
    }
    fout <<
"      break;\n";
  }
}

void
CodedBulkController::SetupSenderSteinerPaths(
  std::ofstream& fout, 
  std::string src_addreess,
  std::list<int>& dst_ids,
  std::map<int, Ptr<CodedBulkMultipathSet> >& path_sets
){
  fout <<
"    case 0:\n" <<
"    case 4:\n" <<
"    case 5:\n" <<
"    case 6:\n";
  // Steiner tree should have only one path set
  fout <<
"      path_ids.clear();\n";
    for(std::list<int>::iterator
      it_dst_id  = dst_ids.begin();
      it_dst_id != dst_ids.end();
      ++it_dst_id
    ) {
      fout <<
"      path_ids.push_back (" << path_sets[*it_dst_id]->_paths[0]->_path_id << ");\n";
    }
    fout << 
"      sender->AddMulticastPath (path_ids, path_type);\n" <<
"      break;\n" <<
"  }\n";
}

void
CodedBulkController::SetupReceiver(
  std::ofstream& fout,
  int application_id,
  std::string local_addreess,
  bool is_CodedBulk_traffic
){
  fout <<
"  Ptr<CodedBulkReceiver> receiver = Create<CodedBulkReceiver> ();\n" <<
"  receiver->SetApplicationID (" << application_id << ");\n" <<
"  receiver->SetLocal (IpToAddress(\"" << local_addreess << "\"));\n";
  if(is_CodedBulk_traffic) {
    fout <<
"  receiver->SetPriority (2);\n" <<
"  receiver->SetThroughProxy (true);\n";
  } else {
    fout <<
"  receiver->SetPriority (6);\n" <<
"  receiver->SetThroughProxy (true);\n";
  }
}

void
CodedBulkController::ExperimentType(std::ofstream& fout)
{
  /*
   * scenario:
   *   0: Steiner_tree_multicast
   *   1: single_path_multicast
   *   2: multi_path_multicast
   *   3: coded_multicast
   *   4: Steiner_tree_without_merging_(coding)
   *   5: Steiner_tree_store-and-forward_disk
   *   6: Steiner_tree_store-and-forward_memory
   */
  fout <<
"  uint8_t scenario = 6;\n" <<
"  uint8_t path_type = 6;\n" <<
"  if(argc > 1){\n" <<
"    scenario = argv[1][0] - '0';\n" << 
"    if (scenario > 6){\n" <<
"      scenario = 1;\n" <<
"    }\n" <<
"    switch (scenario){\n" <<
"      case 0:  path_type = 3;  break;\n" <<
"      case 1:  path_type = 2;  break;\n" <<
"      case 2:  path_type = 2;  break;\n" <<
"      case 3:  path_type = 4;  break;\n" <<
"      case 4:  path_type = 2;  break;\n" <<
"      case 5:  path_type = 3;  break;\n" <<
"      case 6:  path_type = 3;  break;\n" <<
"    }\n" <<
"  }\n";
}

void
CodedBulkController::StoreAndForwardCheck(std::ofstream& fout)
{
  fout <<
"  CodedBulkCodec::setStoreAndForward(scenario == 5);\n" <<
"  if(scenario == 6){\n" <<
"    CodedBulkProxy::setInputBufferSize(MAX_PROXY_INPUT_BUFFER_SIZE);\n" <<
"  } else {\n" <<
"    CodedBulkProxy::setDefaultInputBufferSize();\n" <<
"  }\n";
}

void
CodedBulkController::OutputToFile(std::ofstream& fout, std::string prefix, std::string app)
{
  fout <<
"  system_parameters.waitForInput();\n" <<
"  while(system_parameters.isRunning()){\n" << 
"    SleepForOneSec();\n" <<
"    std::ofstream fout;\n" <<
"    switch(scenario){\n" <<
"      case 0:\n" <<
"        fout.open(\"results/" << prefix << "_throughput_Steiner_tree_multicast.dat\", std::ios::app);\n" <<
"        break;\n" <<
"      case 1:\n" <<
"        fout.open(\"results/" << prefix << "_throughput_single_path_multicast.dat\", std::ios::app);\n" <<
"        break;\n" <<
"      case 2:\n" <<
"        fout.open(\"results/" << prefix << "_throughput_multi_path_multicast.dat\", std::ios::app);\n" <<
"        break;\n" <<
"      case 3:\n" <<
"        fout.open(\"results/" << prefix << "_throughput_coded_multicast.dat\", std::ios::app);\n" <<
"        break;\n" <<
"      case 4:\n" <<
"        fout.open(\"results/" << prefix << "_throughput_Steiner_tree_without_coding.dat\", std::ios::app);\n" <<
"        break;\n" <<
"      case 5:\n" <<
"        fout.open(\"results/" << prefix << "_throughput_Steiner_tree_store-and-forward_disk.dat\", std::ios::app);\n" <<
"        break;\n" <<
"      case 6:\n" <<
"        fout.open(\"results/" << prefix << "_throughput_Steiner_tree_store-and-forward_memory.dat\", std::ios::app);\n" <<
"        break;\n" <<
"    }\n" <<
"    " << app << "->MeasureThroughput (fout);\n" <<
"    fout.close();\n" <<
"  }\n";
}

void
CodedBulkController::EndFile(std::ofstream& fout)
{
  fout <<
"  return 0;\n}";
  fout.close();
}