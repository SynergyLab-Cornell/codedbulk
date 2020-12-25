/* Author:  Shih-Hao Tseng (st688@cornell.edu) */
#ifndef __TRAFFIC_GENERATION_UTILITIES__
#define __TRAFFIC_GENERATION_UTILITIES__
#include <set>
#include "ptr.h"
#include "simple-ref-count.h"
#include "unif-random-variable.h"

// generate num_total_generated_nodes node indices within [0, num_total_nodes-1]
std::set<int>
UniformlyGenerateNodesWithin (
  int num_total_nodes,
  int num_total_generated_nodes,
  int forbidden_node = -1,
  uint64_t random_seed = 0
) {
  Ptr<UniformRandomVariable> unif_rv = Create<UniformRandomVariable> (random_seed);
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