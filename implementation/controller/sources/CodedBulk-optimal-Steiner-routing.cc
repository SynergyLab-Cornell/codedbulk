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
#include "CodedBulk-optimal-Steiner-routing.h"
#include <set>
#include <limits>
#include <vector>

// CBC MILP solver
#include <coin/CbcModel.hpp>
#include <coin/OsiClpSolverInterface.hpp>
//#include <coin/OsiCbcSolverInterface.hpp>
#include <coin/CbcHeuristicFPump.hpp>
#include <coin/CbcHeuristicDiveGuided.hpp>
#include <coin/CbcHeuristicDivePseudoCost.hpp>
#include <coin/CbcHeuristicPivotAndFix.hpp>
#include <coin/CbcHeuristicDW.hpp>
#include <coin/CbcHeuristicGreedy.hpp>
using namespace std;

void
CodedBulkOptimalSteinerRouting::GetPaths (Ptr<CodedBulkTraffic> traffic)
{
  /* optimization */
  // solver
  OsiClpSolverInterface CLP_solver;
  CLP_solver.setObjSense (1.0); // min

  // it is column major format!
  vector<CoinBigIndex> prob_start;
  // and the first one should be 0
  prob_start.push_back(0);

  vector<int>    prob_index;
  vector<double> prob_value;

  vector<double> prob_collb;
  vector<double> prob_colub;

  vector<double> prob_obj  ;

  vector<double> prob_rowlb;
  vector<double> prob_rowub;

  vector<int>    integers;

  Ptr<CodedBulkGraph> dijkstra = Create<CodedBulkGraph> (*m_graph);

  // constriants: from src to each dst, we should be able to send one unit traffic
  //   at each edge: the sending rate is bounded by the link capacity
  //   at each node: i/o balance
  int num_of_links = dijkstra->_all_edges.size();
  int num_of_nodes = dijkstra->_all_nodes.size();
  int rows_per_step = num_of_links + num_of_nodes;

  // variables: 
  //   capacity of links/whether the links are chosen (1, 0)
  int step_row_base = 0;
  for (auto& edge : dijkstra->_all_edges) {
    integers.push_back(prob_collb.size());
    prob_collb.push_back(0.0);
    prob_colub.push_back(1.0);
    prob_obj  .push_back(1.0);

    // constraint on each edge
    // c_l - x_l >= 0
    step_row_base = 0;
    for(auto& dst_id : traffic->_dst_id) {
      prob_index.push_back(step_row_base + edge._id);
      prob_value.push_back(1.0);
      step_row_base += rows_per_step;
    }

    prob_start.push_back (prob_index.size());
  }
  //   for each dst: the rate on each link corresponding to the rate from src to dst 
  step_row_base = 0;
  for(auto& dst_id : traffic->_dst_id) {
    for (auto& edge : dijkstra->_all_edges) {
      prob_collb.push_back(0.0);
      prob_colub.push_back(1.0);
      prob_obj  .push_back(0.0);

      // constraint on each edge
      // c_l - x_l >= 0
      prob_index.push_back(step_row_base + edge._id);
      prob_value.push_back(-1.0);

      // constraint at each node
      // input sum x_l - output sum x_l = ?
      // the head node: output
      prob_index.push_back(step_row_base + num_of_links + edge._node_head_id);
      prob_value.push_back(-1.0);

      // the tail node: input
      prob_index.push_back(step_row_base + num_of_links + edge._node_tail_id);
      prob_value.push_back(1.0);

      prob_start.push_back (prob_index.size());

      // constraints
      // 1 >= c_l - x_l >= 0
      prob_rowlb.push_back(0.0);
      prob_rowub.push_back(1.0);
    }

    // input sum x_l - output sum x_l = ?
    for (auto& node : dijkstra->_all_nodes) {
      if (node._id == traffic->_src_id) {
        prob_rowlb.push_back(-1.0);
        prob_rowub.push_back(-1.0);
      } else if (node._id == dst_id) {
        prob_rowlb.push_back(1.0);
        prob_rowub.push_back(1.0);
      } else {
        prob_rowlb.push_back(0.0);
        prob_rowub.push_back(0.0);
      }
    }
    step_row_base += rows_per_step;
  }

  // finish setting, load the problem
  CLP_solver.loadProblem(
      prob_collb.size(),  // # of columns
      prob_rowlb.size(),  // # of rows
      prob_start.data(),
      prob_index.data(),
      prob_value.data(),
      prob_collb.data(),
      prob_colub.data(),
      prob_obj  .data(),
      prob_rowlb.data(),
      prob_rowub.data()
  );

  for(auto& integer_parameter_index : integers) {
      CLP_solver.setInteger(integer_parameter_index);
  }

  CbcModel milp_model (CLP_solver);

  milp_model.setLogLevel (0);
  milp_model.initialSolve ();
  milp_model.branchAndBound ();

  // get the optimal solution
  if(milp_model.isProvenOptimal()) {
    const double * solution = milp_model.getColSolution();

    // load the solutions
    for (auto& edge : dijkstra->_all_edges) {
      edge._dijkstra_metric = 1.0;
      edge._marked = (*solution > 0.5);
      ++solution;
    }

    // try the paths
    for (auto& node : dijkstra->_all_nodes) {
      node._dijkstra_upstream_edge_id = -1;
    }
    dijkstra->DijkstraFromCodedBulkNode(traffic->_src_id);

    Ptr<CodedBulkMultipathSet> path_set;
    for (auto& dst_id : traffic->_dst_id) {
      // ensure the existence of CodedBulkMultipathSet
      traffic->addDst (dst_id);
      path_set = traffic->getCodedBulkMultipathSet (dst_id);
      path_set->clearAllPaths ();
    }

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
}
