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
#include "CodedBulk-Watel2014-Steiner-routing.h"
#include <set>
#include <limits>
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkWatel2014SteinerRouting");

void
CodedBulkWatel2014SteinerRouting::GetPaths (Ptr<CodedBulkTraffic> traffic)
{
  Algorithm1(traffic);
}

void
CodedBulkWatel2014SteinerRouting::Algorithm1 (Ptr<CodedBulkTraffic> traffic)
{
  // Build I^{|>}
  Ptr<CodedBulkGraph> dijkstra = Create<CodedBulkGraph> (*m_graph);
  Ptr<CodedBulkGraph> I_triangle = Create<CodedBulkGraph> (*m_graph);
  I_triangle->_all_edges.clear();
  I_triangle->_edge_max_id = 0;
  for (auto& node : I_triangle->_all_nodes) {
    node._outbound_edge_ids.clear();
    node._inbound_edge_ids.clear();
  }

  int src_id, dst_id;
  for (auto& src_node : dijkstra->_all_nodes) {
    src_id = src_node._id;
    for (auto& edge : dijkstra->_all_edges) {
      edge._dijkstra_metric = 1.0;
      edge._marked = true;
    }
    dijkstra->DijkstraFromCodedBulkNode (src_id);

    for (auto& dst_node : dijkstra->_all_nodes) {
      dst_id = dst_node._id;
      if (src_id == dst_id) {
        continue;
      }

      CodedBulkEdge& edge = I_triangle->addEdge (src_id, dst_id, true);
      edge._dijkstra_metric = dijkstra->_all_nodes[dst_id]._dijkstra_metric;

      // associate edge with path
      edge._Watel_path = dijkstra->GetDijkstraPath (dst_id);
      edge._Watel_selected = false;
    }
  }
  Ptr<CodedBulkMultipathSet> path_set;
  std::set<int> X;
  for (auto& dst_id : traffic->_dst_id) {
    X.insert(dst_id);
    // ensure the existence of CodedBulkMultipathSet
    traffic->addDst (dst_id);
    path_set = traffic->getCodedBulkMultipathSet (dst_id);
    path_set->clearAllPaths ();
  }
  // T^{|>} <- Greedy_FLAC (I^{|>})
  Greedy_FLAC (I_triangle, traffic->_src_id, X);

  // T contains all corresponding paths
  // translate the tree to paths for merging by coding algorithm
  for (auto& node : dijkstra->_all_nodes) {
    node._dijkstra_upstream_edge_id = -1;
  }
  std::list<int> reachable_nodes;

  // source node
  dijkstra->_all_nodes[traffic->_src_id]._dijkstra_upstream_edge_id = -2;
  reachable_nodes.push_back(traffic->_src_id);

  while (!reachable_nodes.empty()) {
    src_id = reachable_nodes.front();
    reachable_nodes.pop_front();

    for (auto& edge_id : I_triangle->_all_nodes[src_id]._outbound_edge_ids) {
      CodedBulkEdge& edge = I_triangle->_all_edges[edge_id];
      if (edge._Watel_selected) {
        int prev_node_id = -1;
        for (auto& node_id : edge._Watel_path->_nodes) {
          if (prev_node_id == -1) {
            prev_node_id = node_id;
            continue;
          }
          // if the node has not been reached
          if (dijkstra->_all_nodes[node_id]._dijkstra_upstream_edge_id == -1) {
            // use the edge
            dijkstra->_all_nodes[node_id]._dijkstra_upstream_edge_id = dijkstra->findEdgeId(prev_node_id,node_id);

            reachable_nodes.push_back(node_id);
          }
          prev_node_id = node_id;
        }
      }
    }
  }
  dijkstra->_all_nodes[traffic->_src_id]._dijkstra_upstream_edge_id = -1;

  // generate paths from terminals to the source
  for (auto& dst_id : traffic->_dst_id) {
    Ptr<CodedBulkUnicastPath> path = Create<CodedBulkUnicastPath> (0);
    int current_node_id = dst_id;
    int current_edge_id = 0;
    while(current_edge_id != -1) {
      path->_nodes.push_front(current_node_id);

      current_edge_id = dijkstra->_all_nodes[current_node_id]._dijkstra_upstream_edge_id;

      current_node_id = dijkstra->_all_edges[current_edge_id].getTheOtherEnd(current_node_id);
    }

    path_set = traffic->getCodedBulkMultipathSet (dst_id);
    path->_path_id = NewPathID();
    path->_application_port = (uint16_t) traffic->_id + 1000;
      
    path_set->addCodedBulkUnicastPath(path);
  }
}

void
CodedBulkWatel2014SteinerRouting::Greedy_FLAC (Ptr<CodedBulkGraph> G, int r, std::set<int>& X)
{
  /*
    T = \emptyset
    while X (destinations) is not emptyset do
      T0 <- FLAC (graph, X)
      T = T cup T0
      X = X \ (X cap T0) : remove the destinations connected by T0
    return T
  */
  while (!X.empty()) {
    FLAC (G, r, X);
  }
}

void
CodedBulkWatel2014SteinerRouting::FLAC (Ptr<CodedBulkGraph> G, int r, std::set<int>& X)
{
  for (auto& node : G->_all_nodes) {
    node._FLAC_k_a_nodes.clear();

    if (X.find(node._id) != X.end()) {
      // the node is a terminal
      node._FLAC_k_a_nodes.insert(node._id);
    }
  }
  for (auto& edge : G->_all_edges) {
    edge._FLAC_f_a = 0.0;
    edge._marked = false;
    edge._FLAC_in_M = false;
  }

  double t_a;
  double min_t_a;
  double t = 0.0;
  int min_edge_id = -1;
  while (true) {
    min_edge_id = -1;
    min_t_a = std::numeric_limits<double>::max();

    for (auto& edge : G->_all_edges) {
      if ( !(edge._marked | edge._FLAC_in_M) ) {
        edge._FLAC_k_a = G->_all_nodes[edge._node_tail_id]._FLAC_k_a_nodes.size();

        if (edge._FLAC_k_a > 0) {
          t_a = (edge._dijkstra_metric - edge._FLAC_f_a) / edge._FLAC_k_a;
          if (t_a < min_t_a) {
            min_t_a = t_a;
            min_edge_id = edge._id;
          }
        }
      }
    }
    for (auto& edge : G->_all_edges) {
      if ( !(edge._marked | edge._FLAC_in_M) ) {
        edge._FLAC_f_a += edge._FLAC_k_a * min_t_a;
      }
    }

    // t <- t + t(u,v)
    t += min_t_a;
    // G_SAT <- G_SAT \cup (u,v)
    CodedBulkEdge& current_edge = G->_all_edges[min_edge_id];
    current_edge._marked = true;
    
    // if (u = r)
    CodedBulkNode& head_node = G->_all_nodes[current_edge._node_head_id];
    CodedBulkNode& tail_node = G->_all_nodes[current_edge._node_tail_id];
    if (current_edge._node_head_id == r) {
      head_node._FLAC_k_a_nodes.insert(tail_node._FLAC_k_a_nodes.begin(),tail_node._FLAC_k_a_nodes.end());
      // return the tree T0 in G_SAT linking r to the terminals
      G->DijkstraFromCodedBulkNode(r);
      for (auto& terminal : head_node._FLAC_k_a_nodes) {
        // from each terminal, trace back to r
        int current_node_id = terminal;
        while (current_node_id != r) {
          int current_edge_id = G->_all_nodes[current_node_id]._dijkstra_upstream_edge_id;
          CodedBulkEdge& trace_back_edge = G->_all_edges[current_edge_id];
          trace_back_edge._Watel_selected = true;
          current_node_id = trace_back_edge._node_head_id;
        }
        X.erase(terminal);
      }
      return;
    }

    // if the flows is degenerate
    bool degenerate = false;
    // check degeneracy (the edges in G_SAT form a cycle)
    for (auto& terminal : tail_node._FLAC_k_a_nodes) {
      if (head_node._FLAC_k_a_nodes.find(terminal) != head_node._FLAC_k_a_nodes.end()) {
        // degenerate
        degenerate = true;
        break;
      }
    }

    if (degenerate) {
      current_edge._marked = false;
      current_edge._FLAC_in_M = true;
    } else {
      head_node._FLAC_k_a_nodes.insert(tail_node._FLAC_k_a_nodes.begin(),tail_node._FLAC_k_a_nodes.end());
    }
  }
}

} // Namespace ns3
