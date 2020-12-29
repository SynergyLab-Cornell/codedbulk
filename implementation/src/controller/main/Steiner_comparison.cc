/***********
 * Comparing the routing decisions of Steiner tree algorithms
 ***********/
#define TOPOLOGY_NAME B4

#include "../settings/topology/topology_macros.h"
  #define STR(X) XSTR(X)
  #define XSTR(X) #X
  #define TOPOLOGY_ADDRESS(X) STR(../settings/topology/X.cc_part)
#include TOPOLOGY_ADDRESS(TOPOLOGY_NAME)
#include "../settings/traffic/traffic_utils.h"
#include "unif-random-variable.h"
#include "CodedBulk-controller.h"

#include "CodedBulk-Watel2014-Steiner-routing.h"
#include "CodedBulk-optimal-Steiner-routing.h"

#include <iostream>
#include <string>
#include <stdlib.h>
#include <sstream>
using namespace std;

#define TOTAL_EXPERIMENTS 5

int main(int argc, char* argv[]){
  //if(argc < 4) {
  if(argc < 3) {
    return 0;
  }

  Ptr<CodedBulkWatel2014SteinerRouting> watel2014 = Create<CodedBulkWatel2014SteinerRouting> ();
  Ptr<CodedBulkOptimalSteinerRouting> optimal = Create<CodedBulkOptimalSteinerRouting> ();

  int total_num_sources      = atoi(argv[1]);
  int total_num_destinations = atoi(argv[2]);
  int total_loads            = atoi(argv[3]);

  //double workload = 0.1;
  //if (total_loads > 1) {
  //  workload = 0.05;
  //}

  cout << "number of sources = " << total_num_sources << endl;
  cout << "number of destinations = " << total_num_destinations << endl;

  uint64_t RANDOM_SEED = 0;
  //for(int load = 0; load < total_loads; ++load) {
  //  cout << "workload = " << workload << endl;
    for(int experiment = 0; experiment < TOTAL_EXPERIMENTS; ++experiment) {
      std::stringstream sim_name;
      //sim_name << STR(TOPOLOGY_NAME) << "-" << total_num_sources << "-" << total_num_destinations << "-interactive-" << workload << "-" << experiment;
      sim_name << "dummy";

      CodedBulkController::__simualtion_name=sim_name.str();

      #include "../settings/controller_setup.cc_part"

      GENERATE_TOPOLOGY()

      traffic_manager->clearAllCodedBulkTraffic();

      Ptr<CodedBulkTraffic> traffic_at_source[TOTAL_HOSTS];

      if( traffic_manager->GetTrafficNotGenerated() ) {
        std::set<int> sources;
        std::set<int> non_sources;

        Ptr<CodedBulkTraffic> traffic;

        // geberate total_num_sources multicast flows
        sources = UniformlyGenerateNodesWithin (TOTAL_HOSTS, total_num_sources, -1, experiment);
        for(std::set<int>::iterator
          it_src  = sources.begin();
          it_src != sources.end();
          ++it_src
        ) {
          traffic = traffic_manager->addCodedBulkTraffic(*it_src);
          traffic->ApplyCodedBulk(true);
          traffic->SetUnicast(false);
          traffic->SetPriority(CodedBulkTraffic::Bulk);

          std::set<int> destinations = UniformlyGenerateNodesWithin (TOTAL_HOSTS, total_num_destinations, *it_src, RANDOM_SEED++);
          for(std::set<int>::iterator
            it_dst  = destinations.begin();
            it_dst != destinations.end();
            ++it_dst
          ) {
            traffic->addDst(*it_dst);
          }
          traffic_at_source[*it_src] = traffic;
        }
        traffic_manager->SetTrafficNotGenerated(false);
      }

      // TODO: make comparison
      std::cout << "Optimal ==== " << std::endl;
      CodedBulk_controller->SetRoutingAlgorithm(optimal);
      CodedBulk_controller->InitializeAlgorithms();
      CodedBulk_controller->ComputeAllPaths ();
      traffic_manager->listAllCodedBulkTraffic(std::cout);

      std::cout << "Watel 2014 ==== " << std::endl;
      CodedBulk_controller->SetRoutingAlgorithm(watel2014);
      CodedBulk_controller->InitializeAlgorithms();
      CodedBulk_controller->ComputeAllPaths ();
      traffic_manager->listAllCodedBulkTraffic(std::cout);

    }

  //  workload += 0.05;
  //}

  return 0;
}
