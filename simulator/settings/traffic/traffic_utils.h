/* Author:  Shih-Hao Tseng (st688@cornell.edu) */
#ifndef __TRAFFIC_GENERATION_UTILITIES__
#define __TRAFFIC_GENERATION_UTILITIES__
#include <set>

// generate num_total_generated_nodes node indices within [0, num_total_nodes-1]
std::set<int>
UniformlyGenerateNodesWithin (
  int num_total_nodes,
  int num_total_generated_nodes,
  int forbidden_node = -1
) {
  Ptr<UniformRandomVariable> unif_rv = CreateObject<UniformRandomVariable> ();
  std::set<int> nodes;
  nodes.clear();
  while( (int)nodes.size() < num_total_generated_nodes ) {
    int generated_node = unif_rv->GetValue(0,num_total_nodes);
    if( generated_node != forbidden_node ) {
      nodes.insert(generated_node);
    }
  }
  return nodes;
}

#endif  // __TRAFFIC_GENERATION_UTILITIES__