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
#include "ns3/log.h"
#include "CodedBulk-controller.h"
#include "ns3/CodedBulk-bulk-send-application.h"
#include "ns3/CodedBulk-greedy-routing.h"
#include "ns3/CodedBulk-cycle-aware-coding.h"
#include <limits>
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkController");

CodedBulkController::CodedBulkController (void)
{
  // create default CodedBulk algorithms
  SetRoutingAlgorithm (Create<CodedBulkGreedyRouting> ());
  SetCodingAlgorithm (Create<CodedBulkCycleAwareCoding> ());
  m_graph = Create<CodedBulkGraph> ();
  m_traffic_manager = Create<CodedBulkTrafficManager> ();
}

void
CodedBulkController::SetOpenFlowController (Ptr<ofi::OpenFlowRoutingController> controller)
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
      if((traffic->_is_unicast) || (traffic->_priority == CodedBulkTraffic::Interactive)) {
        m_controller->EstablishPath(*it_path, 6);
      } else if(traffic->_is_codedbulk) {
        m_controller->EstablishPath(*it_path, 4);
      } else if (traffic->_is_Steiner_tree) {
        m_controller->EstablishPath(*it_path, 3);
      } else if (traffic->_is_CodedBulk_traffic) {
        m_controller->EstablishPath(*it_path, 2);
      } else {
        // default
        m_controller->EstablishPath(*it_path, 6);
      }
    }
/*
    if((it->second->_backward_path != NULL) && (!traffic->_is_CodedBulk_traffic)) {
      // the backward path is set
      m_controller->EstablishPath(it->second->_backward_path,traffic->_is_CodedBulk_traffic);
    }
*/
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
CodedBulkController::SetApplicationParameters (
  const TypeId& protocol,
  const Time&   start_time,
  const Time&   stop_time
) {
  m_app_protocol   = protocol;
  m_app_start_time = start_time;
  m_app_stop_time  = stop_time;
  ResetAlgorithmParameters ();
}

void
CodedBulkController::CreateApplication (Ptr<CodedBulkTraffic> traffic)
{
  if((!traffic->_is_CodedBulk_traffic) && (traffic->_priority == CodedBulkTraffic::Interactive)) {
    if(traffic->_interactive_workload.empty()) {
      // no traffic
      return;
    }
  }

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
  bool through_proxy = false;
  if(traffic->_is_codedbulk) {
    through_proxy = (traffic->_priority == CodedBulkTraffic::Bulk);
  } else {
    through_proxy = (traffic->_is_CodedBulk_traffic);
  }
  sender->SetThroughProxy (through_proxy);
  sender->SetProtocol (m_app_protocol);
  if((!traffic->_is_CodedBulk_traffic) && (traffic->_priority == CodedBulkTraffic::Interactive)) {
    sender->SetStartTime (MilliSeconds(traffic->_interactive_workload.front()._time_ms));
  } else {
    sender->SetStartTime (m_app_start_time);
  }
  sender->SetStopTime  (m_app_stop_time);
  sender->SetPriority  (traffic->_priority);

  std::list<Ipv4Address> remote_addresses;

  if(through_proxy) {
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
  uint32_t h = 0;
  std::map<int, Ptr<CodedBulkMultipathSet> >& path_sets = traffic->_path_sets;
  if (path_sets.size() > 0)
    h = std::numeric_limits<uint32_t>::max();
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

    if((traffic->_is_unicast) || (traffic->_priority == CodedBulkTraffic::Interactive)) {
      sender->AddMulticastPath(path_ids, 6);
    } else if(traffic->_is_codedbulk) {
      sender->AddMulticastPath(path_ids, 4);
    } else if (traffic->_is_Steiner_tree) {
      sender->AddMulticastPath(path_ids, 3);
    } else if (traffic->_is_CodedBulk_traffic) {
      sender->AddMulticastPath(path_ids, 2);
    } else {
      // default
      sender->AddMulticastPath(path_ids, 6);
    }

  }

  Ptr<Node>& host_src = m_graph->_all_nodes[traffic->_src_id]._host;
  if(host_src != NULL) {
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
    bool through_proxy = false;
    if(traffic->_is_codedbulk) {
      through_proxy = (traffic->_priority == CodedBulkTraffic::Bulk);
    } else {
      through_proxy = (traffic->_is_CodedBulk_traffic);
    }
    receiver->SetThroughProxy (through_proxy);
    receiver->SetProtocol  (m_app_protocol);
    receiver->SetStartTime (m_app_start_time);
    receiver->SetStopTime  (m_app_stop_time);
    receiver->SetPriority  (traffic->_priority);

    receiver->SetLocal(m_controller->GetHostAddress(it->first));

    traffic->_receivers.push_back(receiver);
    Ptr<Node>& host_dst = m_graph->_all_nodes[it->first]._host;
    if(host_dst != NULL) {
      host_dst->AddApplication(receiver);
    }
  }
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

uint64_t
CodedBulkController::GetTotalProxyOccupation (void) const
{
  return m_controller->GetTotalProxyOccupation ();
}

} // Namespace ns3
