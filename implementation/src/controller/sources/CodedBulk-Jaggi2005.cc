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
#include "CodedBulk-Jaggi2005.h"
#include <limits>
#include <iostream>
#include <assert.h>

void
CodedBulkJaggi2005::GenerateCodes (Ptr<CodedBulkTraffic> traffic) {
  // suppose we have paths
  if(traffic->_path_sets.empty()) {
    return;
  }

  // let h be the minimum number of simple paths,
  // we only code the first h paths.
  uint32_t h = std::numeric_limits<uint32_t>::max();
  for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
    it  = traffic->_path_sets.begin();
    it != traffic->_path_sets.end();
    ++it) {
    if(h > it->second->_paths.size()) {
      h = it->second->_paths.size();
    }
  }

  // initialize C_t, A_t
  std::list<CodeMatrix> C_ts;
  std::list<CodeMatrix> A_ts;
  for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
    it  = traffic->_path_sets.begin();
    it != traffic->_path_sets.end();
    ++it) {
    CodeMatrix M(h,h);
    M.diagonal(1);
    C_ts.push_back(M);
    A_ts.push_back(M);
  }

  // v in topological order
  // initialize
  for(std::vector<CodedBulkEdge>::iterator
    it  = m_graph->_all_edges.begin();
    it != m_graph->_all_edges.end();
    ++it) {
    (*it)._marked = false;
    (*it)._upstream_edge_ids.clear();
  }
  for(std::vector<CodedBulkNode>::iterator
    it  = m_graph->_all_nodes.begin();
    it != m_graph->_all_nodes.end();
    ++it) {
    (*it)._outbound_edge_ids.clear();
    (*it)._inbound_edge_ids.clear();
  }
  // construct the graph
  std::list<CodeMatrix>::iterator it_c = C_ts.begin();
  std::list<CodeMatrix>::iterator it_a = A_ts.begin();
  for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
    it  = traffic->_path_sets.begin();
    it != traffic->_path_sets.end();
    ++it, ++it_c, ++it_a) {
    for(uint32_t i = 0; i < h; ++i) {
      Ptr<CodedBulkUnicastPath>& path = it->second->_paths[i];
      // setup tp dst
      path->_carrying_path_id = path->_path_id + 1;

      // assign path id
      path->_path_code_id = i + h*it->first;

      // associate the code vector
      path->_path_code = &(*it_c)[i];

      // associate the orthogonal tester a_t
      path->_orthogonal_tester = &(*it_a)[i];

      // associate the code matrix A_t
      path->_orthogonal_matrix = &(*it_a);

      // start from the path root
      path->_current_node = path->_nodes.begin();

      // consider only the first h paths
      std::list<int>::iterator current_node = path->_current_node;
      std::list<int>::iterator next_node    = path->_current_node;
      ++next_node;

      int edge_id;
      int prev_edge_id = -1;
      int src, dst;

      while(next_node != path->_nodes.end() ) {
        src = *current_node;
        dst = *next_node;

        // associate edge information
        edge_id = m_graph->findEdgeId(src,dst);
        CodedBulkEdge& edge = m_graph->_all_edges[edge_id];
        edge._marked = true;
        if( prev_edge_id != -1 ) {
          edge._upstream_edge_ids.insert(prev_edge_id);
        }
        prev_edge_id = edge_id;

        // associate node information
        m_graph->_all_nodes[src]._outbound_edge_ids.insert(edge_id);
        m_graph->_all_nodes[dst]._inbound_edge_ids.insert(edge_id);

        // move to the next node
        ++current_node;
        ++next_node;
      }
    }
  }

  std::list<int> ordered_node_id; // in topological order
  ordered_node_id.clear();
  std::list<int> nodes_with_no_upstream;
  nodes_with_no_upstream.clear();
  nodes_with_no_upstream.push_back(traffic->_src_id);
  // Kahn's algorithm
  int current_node_id = -1;
  while (!nodes_with_no_upstream.empty()) {
    current_node_id = nodes_with_no_upstream.front();
    nodes_with_no_upstream.pop_front();

    ordered_node_id.push_back(current_node_id);
    CodedBulkNode& node = m_graph->_all_nodes[current_node_id];
    for(std::set<int>::iterator
      it  = node._outbound_edge_ids.begin();
      it != node._outbound_edge_ids.end();
      ++it) {
      CodedBulkEdge& edge = m_graph->_all_edges[(*it)];

      if (!edge._marked) {
        // weird case, should not happen
        continue;
      }
      edge._marked = false;
      CodedBulkNode& other_node = m_graph->_all_nodes[edge._node_tail_id];
      bool exist_upstream = false;
      for(std::set<int>::iterator
        it1  = other_node._inbound_edge_ids.begin();
        it1 != other_node._inbound_edge_ids.end();
        ++it1) {
        if( m_graph->_all_edges[*it1]._marked ){
          exist_upstream = true;
        }
      }
      if( !exist_upstream ) {
        nodes_with_no_upstream.push_back(other_node._id);
      }
    }
  }
  // check if topological order exists
  for(std::vector<CodedBulkEdge>::iterator
    it  = m_graph->_all_edges.begin();
    it != m_graph->_all_edges.end();
    ++it) {
    if( (*it)._marked ) {
      // there exists cycle, no topological order exists
      return;
    }
  }

  for(std::list<int>::iterator 
    it_id  = ordered_node_id.begin();
    it_id != ordered_node_id.end();
    ++it_id) {
    // in topological order
    // walk through those paths

    std::list<Ptr<CodedBulkUnicastPath> > paths_through_the_node;
    paths_through_the_node.clear();
    for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
      it  = traffic->_path_sets.begin();
      it != traffic->_path_sets.end();
      ++it) {
      // reached the destination
      if(it->first == *it_id) {
        continue;
      }

      // find those paths to be considered
      for(uint32_t i = 0; i < h; ++i) {
        Ptr<CodedBulkUnicastPath>& path = it->second->_paths[i];

        if(*(path->_current_node) == (*it_id)) {
          // move to the next node
          ++path->_current_node;

          // goes through the node
          paths_through_the_node.push_back(path);
        }
      }
    }

    // handle those paths:
    int next_hop_id = -1;
    int dst_id = -1;
    int col_size = 0;
    VirtualLink* encode_map = nullptr;
    
    while(!paths_through_the_node.empty()) {
      // consider the paths going through the same edge
      std::list<Ptr<CodedBulkUnicastPath> > considered_paths;
      considered_paths.clear();
      next_hop_id = *(paths_through_the_node.front()->_current_node);
      std::set<int> included_dst;
      included_dst.clear();

      for(std::list<Ptr<CodedBulkUnicastPath> >::iterator
        it_p  = paths_through_the_node.begin();
        it_p != paths_through_the_node.end();
        ) {
        dst_id = (*it_p)->_path_code_id / h;
        if(
          (*((*it_p)->_current_node) == next_hop_id) &&
          (included_dst.find(dst_id) == included_dst.end()) 
        ) {
          // go through the same edge and not from the same flow
          considered_paths.push_back(*it_p);
          it_p = paths_through_the_node.erase(it_p);
          included_dst.insert(dst_id);
        } else {
          ++it_p;
        }
      }

      // found the paths, calculate the new code
      col_size = considered_paths.size();
      encode_map = new VirtualLink (0,0);
      CodeVector u = *(considered_paths.front()->_path_code);
      CodeVector coder(col_size);
      coder.fillWith(0);
      int coder_index = 0;
      coder[coder_index] = 1;

      // using the first packet tp dst to carry those information
      uint32_t carrying_path_id = considered_paths.front()->_path_id + 1;
      std::set<uint32_t> considered_carrying_paths;
      considered_carrying_paths.clear();

      for(std::list<Ptr<CodedBulkUnicastPath> >::iterator
        it_i  = considered_paths.begin();
        it_i != considered_paths.end();
        ++it_i) {
        uint32_t& current_carrying_path_id = (*it_i)->_carrying_path_id;

        if(considered_carrying_paths.find(current_carrying_path_id) == considered_carrying_paths.end()) {
          // the path has not yet been considered
          considered_carrying_paths.insert(current_carrying_path_id);
          encode_map->_input_paths << current_carrying_path_id;

          CodeVector& a = *((*it_i)->_orthogonal_tester);
          CodeVector& x = *((*it_i)->_path_code);
          if( u*a == 0 ) {
            // construct alpha_to_avoid
            std::set<GF256> alpha_to_avoid;
            for(std::list<Ptr<CodedBulkUnicastPath> >::iterator
              it_j  = considered_paths.begin();
              it_j != it_i;
              ++it_j) {
              CodeVector& a_j = *((*it_j)->_orthogonal_tester);
              GF256 alpha_j = -(x*a_j)/(u*a_j);
              alpha_to_avoid.insert(alpha_j);
            }

            // find alpha, notice that GF256(x) + GF256(1) + GF256(1) = GF256(x)
            GF256 alpha = 1;

            while ( alpha_to_avoid.find(alpha) != alpha_to_avoid.end() ) {
              // move to the next element
              alpha = (+alpha + 1);
              if( alpha == 0 ) {
                alpha = 1;
              }
            }
            u = u*alpha + x;

            // construct coder
            coder *= alpha;
            coder[coder_index] = 1;
          }
          ++coder_index;
        }
        current_carrying_path_id = carrying_path_id;
      }
      // we got the coded vector u, update the orthogonal basis
      uint32_t path_subid = h + 1;
      for(std::list<Ptr<CodedBulkUnicastPath> >::iterator
        it_i  = considered_paths.begin();
        it_i != considered_paths.end();
        ++it_i) {
        path_subid = (*it_i)->_path_code_id % h;
        CodeVector& x = *((*it_i)->_path_code);
        CodeVector& a = *((*it_i)->_orthogonal_tester);
        CodeMatrix& A = *((*it_i)->_orthogonal_matrix);

        // update 
        x  = u;
        a /= (u*a);
        for(uint32_t i = 0; i < h; ++i) {
          if(i == path_subid) {
            continue;
          }
          // update orthogonal testers
          A[i] -= a*(x*A[i]); 
        }
      }

      // resize the coder
      coder.forceResize(coder_index);
      // set up the size the code map
      encode_map->singleRow(coder);
      // just output the first port, as the encoded packet will go to the same next hop
      encode_map->_output_paths << carrying_path_id;

      m_controller->AddCodedBulkEncoderAt(encode_map,*it_id);
//      std::cout << "add encoder at node " << *it_id << std::endl;
//      encode_map->listMap(std::cout);
    }
  }

  // install decoders at the destination
  VirtualLink* decode_basis_map = nullptr;
  it_c = C_ts.begin();
  for(std::map<int, Ptr<CodedBulkMultipathSet> >::iterator
    it  = traffic->_path_sets.begin();
    it != traffic->_path_sets.end();
    ++it, ++it_c) {
    decode_basis_map = new VirtualLink (*it_c);
    for(uint32_t i = 0; i < h; ++i) {
      Ptr<CodedBulkUnicastPath>& path = it->second->_paths[i];
      decode_basis_map-> _input_paths << path->_carrying_path_id;
      decode_basis_map->_output_paths << path->_path_id + 1;
    }
    // discard all other paths
    it->second->_paths.resize(h);
    m_controller->AddCodedBulkDecoderAt(decode_basis_map,it->first);
//    std::cout << "add decoder at node " << it->first << std::endl;
//    decode_basis_map.listMap(std::cout);
  }
}
