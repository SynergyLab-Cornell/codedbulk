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
#ifndef CodedBulk_GRAPH_H
#define CodedBulk_GRAPH_H

#include "ns3/simple-ref-count.h"
#include "ns3/ptr.h"
#include "ns3/node.h"

#include "ns3/CodedBulk-simple-net-device.h"
#include "CodedBulk-path.h"

#include <vector>
#include <set>

namespace ns3{

class CodedBulkNode : public SimpleRefCount<CodedBulkNode> {
public:
    static bool compareLess (Ptr<CodedBulkNode> n1, Ptr<CodedBulkNode> n2);

    CodedBulkNode ();
    CodedBulkNode (int id);

    void initialize ();

    void operator = (const CodedBulkNode& CodedBulkNode);

    int              _id;
    std::set<int>    _outbound_edge_ids;

    bool             _marked;        // only the up CodedBulkNode in CSPF will be computed in Dijkstra algorithm

    bool             _dijkstra_travelled;
    double           _dijkstra_metric;
    int              _dijkstra_upstream_edge_id;

    // for code generator
    // the codes at this CodedBulkNode
    std::set<int>    _inbound_edge_ids;
    Ptr<Node>        _host; // the host at this node

    // for Watel 2014 algorithm
    std::set<int>      _FLAC_k_a_nodes;
};

class CodedBulkEdge : public SimpleRefCount<CodedBulkEdge> {
public:
    CodedBulkEdge ();
    CodedBulkEdge (int head, int tail);
    
    void   initialize ();
    int    getTheOtherEnd (int id);
    double getDijkstraMetric ();
    
    void operator = (const CodedBulkEdge& CodedBulkEdge);
    
    int     _id;
    int     _node_head_id;
    int     _node_tail_id;

    double  _capacity;
    double  _dijkstra_metric;
    bool    _marked;
    
    // for code generator
    std::set<int> _upstream_edge_ids;

    Ptr<CodedBulkSimpleNetDevice> _net_device;

    // for Watel 2014 algorithm
    Ptr<CodedBulkUnicastPath> _Watel_path;
    bool               _Watel_selected;
    //bool               _FLAC_in_G_SAT = _marked
    bool               _FLAC_in_M;
    double             _FLAC_f_a;
    int                _FLAC_k_a;
};

class CodedBulkGraph : public SimpleRefCount<CodedBulkGraph> {
public:
    CodedBulkGraph();

    void initialize ();

    void     addNodes (int number);
    void     setHostNodeAt (int node_id, Ptr<Node> host);
    CodedBulkEdge&  addEdge (int from_node_id, int to_node_id, bool directed = false);

    int  getNumTotalNodes () {  return _all_nodes.size();  }
    int  getNumTotalEdges () {  return _all_edges.size();  }

    int  findEdgeId (int node_head_id, int node_tail_id);

    void DijkstraFromCodedBulkNode (int node_id);    // compute the shortest path tree
    Ptr<CodedBulkUnicastPath> GetDijkstraPath (int dst_id);

    void operator = (const CodedBulkGraph& graph);

    int _node_max_id;
    int _edge_max_id;

    std::vector<CodedBulkNode> _all_nodes;
    std::vector<CodedBulkEdge> _all_edges;
};

}

#endif // CodedBulk_GRAPH_H
