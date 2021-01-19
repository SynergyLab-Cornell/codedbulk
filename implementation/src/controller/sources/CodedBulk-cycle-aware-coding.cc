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
#include "CodedBulk-cycle-aware-coding.h"
#include <limits>
#include <iostream>
#include <assert.h>

void
CodedBulkCycleAwareCoding::GenerateCodes (Ptr<CodedBulkTraffic> traffic) {
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

  std::map<int, std::list<Ptr<CodedBulkUnicastPath> > > paths_through_the_node_map;
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

      // construct the map of paths_through_the_node
      // excluding the destinations
      std::list<int>::iterator it_node = path->_current_node;
      std::list<int>::iterator it_next_node = path->_current_node;
      ++it_next_node;
      while(it_next_node != path->_nodes.end() ) {
        if( paths_through_the_node_map.find(*it_node) == paths_through_the_node_map.end() ) {
          paths_through_the_node_map[*it_node].clear();
        }
        paths_through_the_node_map[*it_node].push_back(path);

        ++it_node;
        ++it_next_node;
      }
    }
  }

  int current_node_id = -1;
  while ( !paths_through_the_node_map.empty() ) {
    // if we still have node that is not considered
    // figure out the next node to consider:
    //   1. is there a node of topological order?
    current_node_id = -1;
    for(std::map<int, std::list<Ptr<CodedBulkUnicastPath> > >::iterator
      it_path_map  = paths_through_the_node_map.begin();
      it_path_map != paths_through_the_node_map.end();
      ++it_path_map
    ) {
      bool arrive_at_the_node = true;
      for(std::list<Ptr<CodedBulkUnicastPath> >::iterator
        it_path  = it_path_map->second.begin();
        it_path != it_path_map->second.end();
        ++it_path
      ) {
        if( (*((*it_path)->_current_node)) != it_path_map->first ) {
          // some path is still not at the node
          arrive_at_the_node = false;
          break;
        }
      }
      if(arrive_at_the_node) {
        // found a ready node
        current_node_id = it_path_map->first;
        break;
      }
    }

    //   2. if not, pick the first one and move all the paths to the node (greedy approach)
    VirtualLink* encode_link = nullptr;
    if(current_node_id == -1) {
      current_node_id = paths_through_the_node_map.begin()->first;

      for(std::list<Ptr<CodedBulkUnicastPath> >::iterator
        it_path  = paths_through_the_node_map[current_node_id].begin();
        it_path != paths_through_the_node_map[current_node_id].end();
        ++it_path
      ) {
        Ptr<CodedBulkUnicastPath>& path = *it_path;
        while( (*(path->_current_node)) != current_node_id ) {
          paths_through_the_node_map[*(path->_current_node)].remove(path);

          // add an identity code map
          encode_link = new VirtualLink (1,1);
          (*encode_link)[0][0] = 1;
          encode_link->_input_paths  << path->_carrying_path_id;
          encode_link->_output_paths << path->_path_id + 1;
          path->_carrying_path_id = path->_path_id + 1;

          m_controller->AddCodedBulkEncoderAt(encode_link,*(path->_current_node));
/*
          // setup a simple forward rule
          // -> don't work unless we know path->_carrying_path_id will only be used by one path here
          uint32_t from_path_id = path->_carrying_path_id;
          uint32_t to_path_id   = path->_path_id;
          path->_carrying_path_id = path->_path_id;

          m_controller->SetForwardRuleAt (*(path->_current_node), from_path_id, to_path_id);
*/
          ++path->_current_node;
        }
      }
    }

    // now we have the node to be considered
    // walk through those paths
    std::list<Ptr<CodedBulkUnicastPath> >& paths_through_the_node = paths_through_the_node_map[current_node_id];
    for(std::list<Ptr<CodedBulkUnicastPath> >::iterator
      it_path  = paths_through_the_node.begin();
      it_path != paths_through_the_node.end();
      ++it_path
    ) {
      // move to the next hop
      ++(*it_path)->_current_node;
    }

    // handle those paths:
    int next_hop_id = -1;
    int dst_id = -1;
    int col_size = 0;
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
      encode_link = new VirtualLink (0,0);
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
          encode_link->_input_paths << current_carrying_path_id;

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
      encode_link->singleRow(coder);
      // just output the first port, as the encoded packet will go to the same next hop
      encode_link->_output_paths << carrying_path_id;

      m_controller->AddCodedBulkEncoderAt(encode_link,current_node_id);
      // std::cout << "add encoder at node " << current_node_id << std::endl;
      // encode_link->listMap(std::cout);
    }

    // finish this node
    paths_through_the_node_map.erase(current_node_id);
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
