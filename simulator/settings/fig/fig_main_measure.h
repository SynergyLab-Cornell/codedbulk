/* Author:  Shih-Hao Tseng (st688@cornell.edu) */
#include "settings/traffic/traffic_utils.h"

std::ofstream fout;
std::ofstream fout_raw;
std::ofstream fout_rawd;

//int    total = 0;
//double total_throughput = 0.0;

int    bulk_total = 0;
double bulk_total_throughput = 0.0;

int    interactive_total = 0;
double interactive_total_throughput = 0.0;

#define TOTAL_EXPERIMENTS 10

void
RecordAggregateThroughput (Ptr<CodedBulkTrafficManager> traffic_manager) {
  std::cout << Simulator::Now()
            << "\t" << traffic_manager->GetTotalThroughput()
            << "\t" << traffic_manager->GetTotalBulkThroughput()
            << "\t" << traffic_manager->GetTotalInteractiveThroughput() << std::endl;
  fout_rawd << Simulator::Now()
            << "\t" << traffic_manager->GetTotalThroughput()
            << "\t" << traffic_manager->GetTotalBulkThroughput()
            << "\t" << traffic_manager->GetTotalInteractiveThroughput() << std::endl;

  ++total;
  total_throughput += traffic_manager->GetTotalThroughput();

  ++bulk_total;
  bulk_total_throughput += traffic_manager->GetTotalBulkThroughput();

  ++interactive_total;
  interactive_total_throughput += traffic_manager->GetTotalInteractiveThroughput();

}

void
ResetFigMeasureVariables (void) {
  total = 0;
  total_throughput = 0.0;

  bulk_total = 0;
  bulk_total_throughput = 0.0;

  interactive_total = 0;
  interactive_total_throughput = 0.0;
}