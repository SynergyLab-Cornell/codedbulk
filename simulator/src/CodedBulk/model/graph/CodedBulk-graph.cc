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
#include "CodedBulk-graph.h"

#include <limits>
#include <list>
#include <algorithm>
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkGraph");

bool
CodedBulkNode::compareLess (Ptr<CodedBulkNode> n1, Ptr<CodedBulkNode> n2) {
    return n1->_dijkstra_metric < n2->_dijkstra_metric;
}

CodedBulkNode::CodedBulkNode () {
    initialize();
}

CodedBulkNode::CodedBulkNode (int id) {
    initialize();
    _id = id;
}

void
CodedBulkNode::initialize () {
    _id = -1;
    _outbound_edge_ids.clear();
    
    _marked = true;

    _dijkstra_travelled = false;
    _dijkstra_metric    = std::numeric_limits<double>::max();
    _dijkstra_upstream_edge_id = -1;

    _inbound_edge_ids.clear();
    _host = NULL;
}

void
CodedBulkNode::operator = (const CodedBulkNode& node) {
    _id                = node._id;
    _outbound_edge_ids = node._outbound_edge_ids;

    _marked = node._marked;

    _dijkstra_travelled        = node._dijkstra_travelled;
    _dijkstra_metric           = node._dijkstra_metric;
    _dijkstra_upstream_edge_id = node._dijkstra_upstream_edge_id;

    _inbound_edge_ids = node._inbound_edge_ids;
    _host = node._host;
}

CodedBulkEdge::CodedBulkEdge () {
    initialize();
}

CodedBulkEdge::CodedBulkEdge (int head, int tail) {
    initialize();
    _node_head_id = head;
    _node_tail_id = tail;
}
    
void
CodedBulkEdge::initialize (){
    _capacity = 0.0;
    _dijkstra_metric = 0.0;
    _marked = true;
}

int
CodedBulkEdge::getTheOtherEnd (int id) {
    return (id == _node_head_id) ? _node_tail_id : ((id == _node_tail_id) ? _node_head_id : -1);
}

double
CodedBulkEdge::getDijkstraMetric () {
    // add this function in case we want to change the metric
    return _dijkstra_metric;
}

void
CodedBulkEdge::operator = (const CodedBulkEdge& edge) {
    _id           = edge._id;
    _node_head_id = edge._node_head_id;
    _node_tail_id = edge._node_tail_id;

    _capacity        = edge._capacity;
    _dijkstra_metric = edge._dijkstra_metric;

    _marked = edge._marked;
    _upstream_edge_ids = edge._upstream_edge_ids;
    
    _net_device = edge._net_device;
}

CodedBulkGraph::CodedBulkGraph() {
    initialize();
}

void
CodedBulkGraph::initialize () {
    _all_nodes.clear();
    _all_edges.clear();
    _node_max_id    = 0;
    _edge_max_id    = 0;
}

void
CodedBulkGraph::addNodes (int number) {
    for (int i = 0; i < number; ++i, ++_node_max_id) {
        _all_nodes.push_back(CodedBulkNode(_node_max_id));
    }
}

void
CodedBulkGraph::setHostNodeAt (int node_id, Ptr<Node> host) {
    _all_nodes[node_id]._host = host;
}

CodedBulkEdge&
CodedBulkGraph::addEdge (int from_node_id, int to_node_id, bool directed) {
    // 1 -> and 1 <- would be 2 on the edge
    CodedBulkEdge e(from_node_id,to_node_id);
    e._id = _edge_max_id;
    _all_edges.push_back (e);

    _all_nodes[from_node_id]._outbound_edge_ids.insert (_edge_max_id);
    if ( !directed )
        _all_nodes[to_node_id]._outbound_edge_ids.insert (_edge_max_id);

    ++_edge_max_id;

    return _all_edges.back ();
}

int
CodedBulkGraph::findEdgeId (int node_head_id, int node_tail_id) {
    for (std::vector<CodedBulkEdge>::iterator it_edge = _all_edges.begin();
                                    it_edge != _all_edges.end(); ++it_edge) {
        if ( ((*it_edge)._node_head_id == node_head_id) &&
             ((*it_edge)._node_tail_id == node_tail_id) ) {
            return (*it_edge)._id;
        }
    }
    return -1;
}

void
CodedBulkGraph::DijkstraFromCodedBulkNode (int node_id) {
    for(auto& node : _all_nodes) {
        node._dijkstra_metric    = std::numeric_limits<double>::max();
        node._dijkstra_travelled = false;
        node._dijkstra_upstream_edge_id = -1;
    }

    Ptr<CodedBulkNode> current_node   = &_all_nodes[node_id];
    if( !current_node->_marked )  return;

    Ptr<CodedBulkNode> temp_node = NULL;
    current_node->_dijkstra_metric           = 0.0;
    current_node->_dijkstra_upstream_edge_id = -1;

    // the discovered nodes, must be CSPF up
    std::list<Ptr<CodedBulkNode> > pCurrent_nodes;
    pCurrent_nodes.push_back(current_node);
    
    while ( !pCurrent_nodes.empty() ) {
        current_node = pCurrent_nodes.front();
        pCurrent_nodes.pop_front();

        for(std::set<int>::iterator it_id =  current_node->_outbound_edge_ids.begin(); 
                                    it_id != current_node->_outbound_edge_ids.end(); ++it_id ){
            // find the other node
            temp_node = &_all_nodes[_all_edges[*it_id].getTheOtherEnd(current_node->_id)];

            // if the edge is connected, and both nodes are up (the current node must be up)            
            if( _all_edges[*it_id]._marked && temp_node->_marked ){
                // check if the route via this edge is shorter
                if( temp_node   ->_dijkstra_metric > 
                    current_node->_dijkstra_metric+_all_edges[*it_id].getDijkstraMetric() ){
                    // if so, update the route and set the parent node as the current node
                    temp_node->_dijkstra_metric           = current_node->_dijkstra_metric+_all_edges[*it_id].getDijkstraMetric();
                    temp_node->_dijkstra_upstream_edge_id = *it_id;
                }

                // if the other node has not yet been discovered, discover it
                if( !temp_node->_dijkstra_travelled ) {
                    temp_node->_dijkstra_travelled = true;
                    pCurrent_nodes.push_back(temp_node);
                }
            }
        }

        // sort in ascending order
        //std::sort(pCurrent_nodes.begin(), pCurrent_nodes.end(), CodedBulkNode::compareLess);
        pCurrent_nodes.sort(CodedBulkNode::compareLess);
    }
}

Ptr<CodedBulkUnicastPath>
CodedBulkGraph::GetDijkstraPath (int dst_id)
{
  // after DijkstraFromCodedBulkNode, get a path from the source to dst
  if (_all_nodes[dst_id]._dijkstra_upstream_edge_id == -1) {
    return nullptr;
  }

  Ptr<CodedBulkUnicastPath> path = Create<CodedBulkUnicastPath> (0);
  int current_node_id = dst_id;
  int current_edge_id = 0;
  while(current_edge_id != -1) {
    path->_nodes.push_front(current_node_id);

    current_edge_id = _all_nodes[current_node_id]._dijkstra_upstream_edge_id;

    current_node_id = _all_edges[current_edge_id].getTheOtherEnd(current_node_id);
  }

  return path;
}

void
CodedBulkGraph::operator = (const CodedBulkGraph& graph) {
    _all_nodes   = graph._all_nodes;
    _node_max_id = graph._node_max_id;

    _all_edges   = graph._all_edges;
    _edge_max_id = graph._edge_max_id;
}

}