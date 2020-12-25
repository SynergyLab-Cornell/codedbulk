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
#include "CodedBulk-greedy-routing.h"
#include <limits>
#include <assert.h>
#define ERROR_BOUND 0.000000001

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkGreedyRouting");

void
CodedBulkGreedyRouting::GetPaths (Ptr<CodedBulkTraffic> traffic)
{
  for(std::list<int>::iterator
    it_dst_id  = traffic->_dst_id.begin();
    it_dst_id != traffic->_dst_id.end();
    ++it_dst_id
  ) {
    GetPaths (traffic, *it_dst_id);
  }
}

void
CodedBulkGreedyRouting::GetPaths (Ptr<CodedBulkTraffic> traffic, int dst_id)
{
  if(traffic->_src_id == dst_id) {
    return;
  }
  Ptr<CodedBulkGraph> dijkstra_graph = Create<CodedBulkGraph> (*m_graph);
  for (auto& edge : dijkstra_graph->_all_edges) {
    edge._dijkstra_metric = 1.0;
    edge._marked = true;
  }

  // find the CodedBulkMultipathSet and reset its paths
  traffic->addDst (dst_id);
  Ptr<CodedBulkMultipathSet> path_set = traffic->getCodedBulkMultipathSet (dst_id);
  path_set->clearAllPaths ();

  // find the edge-disjoint paths and their rates to achieve max s-t rates
  bool connected = true;
  while (connected) {
    double bandwidth = std::numeric_limits<double>::max();

    dijkstra_graph->DijkstraFromCodedBulkNode(traffic->_src_id);
    Ptr<CodedBulkUnicastPath> path = dijkstra_graph->GetDijkstraPath (dst_id);
    connected = (path != nullptr);

    // once we have a path, take the max bandwidth
    if (connected) {
      int edge_id = 0;
      // find the bottleneck bandwidth
      for (auto& node_id : path->_nodes) {
        edge_id = dijkstra_graph->_all_nodes[node_id]._dijkstra_upstream_edge_id;
        if (edge_id != -1) {
          if (bandwidth > dijkstra_graph->_all_edges[edge_id]._capacity) {
            bandwidth = dijkstra_graph->_all_edges[edge_id]._capacity;
          }
        }
      }
      // remove the bottleneck bandwidth
      for (auto& node_id : path->_nodes) {
        edge_id = dijkstra_graph->_all_nodes[node_id]._dijkstra_upstream_edge_id;
        if (edge_id != -1) {
          dijkstra_graph->_all_edges[edge_id]._capacity -= bandwidth;
          if ( dijkstra_graph->_all_edges[edge_id]._capacity < ERROR_BOUND ) {
            dijkstra_graph->_all_edges[edge_id]._marked = false;
          }
        }
      }

      path->_rate = bandwidth;
      
      path->_path_id = NewPathID();
      path->_application_port = (uint16_t) traffic->_id + 1000;

      path_set->addCodedBulkUnicastPath(path);
    }
  }
/*
  if((path_set->_paths.size() > 0) && (!traffic->_is_CodedBulk_traffic)) {
    // setup the backward path
    Ptr<CodedBulkUnicastPath> path_r = Create<CodedBulkUnicastPath>(path_set->_paths[0]->GetReversedPath (CodedBulkUnicastPath::GetAnyPathID()));
    path_set->setBackwardPath(path_r);
  }
*/
}

} // Namespace ns3
