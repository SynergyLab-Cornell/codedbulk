/* Author:  Shih-Hao Tseng (st688@cornell.edu) */
#ifndef __MEASUREMENT_UTILITIES__
#define __MEASUREMENT_UTILITIES__

int total = 0;
uint64_t total_bytes = 0;
double total_throughput = 0.0;
double average = 0.0;

void
ReturnTotalReceivedBytes (Ptr<CodedBulkReceiver> receiver) {
  std::cout << "receiver " << receiver << " received " << receiver->GetTotBytes () << " bytes" << std::endl;
}

// the measured bytes within the previous second
void
ReturnMeasuredBytes (Ptr<CodedBulkReceiver> receiver) {
  ++total;
  total_bytes += receiver->GetMeasuredBytes ();
  std::cout << Simulator::Now() << "\tTotal within the second is " << receiver->GetMeasuredBytes () << " bytes/s, average is " << total_bytes/total << " bytes/s." << std::endl;
}

void
ReturnTrafficMeasuredBytes (Ptr<CodedBulkTraffic> traffic) {
  ++total;
  total_bytes += traffic->GetTotalMeasuredBytes ();
  average = (double) total_bytes/(total*traffic->GetNumDst());
  std::cout << Simulator::Now() << "\tTotal within the second is " << traffic->GetTotalMeasuredBytes () << " bytes/s, average is " << average << " bytes/s." << std::endl;
}

void
ReturnAverageThroughput (Ptr<CodedBulkTrafficManager> traffic_manager) {
  // the average per destination throughput over all traffic
  ++total;
  total_throughput += traffic_manager->GetAverageThroughput();
  average = (double) total_throughput/total;
  std::cout << Simulator::Now() << "\tAverage throughput within the second is " << traffic_manager->GetAverageThroughput() << " bytes/s, average is " << average << " bytes/s." << std::endl;
}

void
ReturnAverageThroughput (Ptr<CodedBulkTrafficManager> traffic_manager, int& _total, double& _total_throughput, double& _average) {
  // the average per destination throughput over all traffic
  ++_total;
  _total_throughput += traffic_manager->GetAverageThroughput();
  _average = (double) _total_throughput/_total;
  std::cout << Simulator::Now() << "\tAverage throughput within the second is " << traffic_manager->GetAverageThroughput() << " bytes/s, average is " << average << " bytes/s." << std::endl;
}

void
ResetMeasureVariables (void) {
  total = 0;
  total_bytes = 0;
  total_throughput = 0.0;
  average = 0.0;
}

void
ReturnPerTrafficMeasuredBytes (Ptr<CodedBulkTraffic> traffic) {
  std::cout << Simulator::Now() << "\tTotal within the second for traffic " << traffic->_id << " is " 
  << traffic->GetTotalMeasuredBytes ()/(traffic->GetNumDst()) << " bytes/s." << std::endl;
}

#ifndef LOG_EVERYTHING_OUT
#define LOG_EVERYTHING_OUT std::cout
#endif

void
LogEverything (Ptr<CodedBulkTraffic>* traffic, int number_of_traffic, bool coded, Ptr<CodedBulkController> controller, Ptr<CodedBulkProxy>* proxy) {
  std::cout << "at time " << Simulator::Now() << " --" << std::endl;
  LOG_EVERYTHING_OUT << "at time " << Simulator::Now() << " --" << std::endl;
  for(int i = 0; i < number_of_traffic; ++i) {
    LOG_EVERYTHING_OUT << "traffic " << traffic[i]->_id << " sends " << traffic[i]->_sender->GetMeasuredBytes()
         << " Bps at source " << traffic[i]->_src_id << " and receives ";
    std::list<int>::iterator it_dst_id = traffic[i]->_dst_id.begin();
    for(std::list<Ptr<CodedBulkReceiver> >::iterator it = traffic[i]->_receivers.begin(); it != traffic[i]->_receivers.end(); ++it, ++it_dst_id) {
      LOG_EVERYTHING_OUT << (*it)->GetMeasuredBytes() << " Bps at destination " << (*it_dst_id) << "; ";
    }
    LOG_EVERYTHING_OUT << std::endl;
  }
  LOG_EVERYTHING_OUT << std::endl;

  Ptr<CodedBulkGraph> graph = controller->GetCodedBulkGraph();
  for(std::vector<CodedBulkEdge>::iterator
    it  = graph->_all_edges.begin();
    it != graph->_all_edges.end();
    ++it
  ) {
    LOG_EVERYTHING_OUT << "edge (" << (*it)._node_head_id << "," << (*it)._node_tail_id << ") sends "
                       << (*it)._net_device->GetMeasuredBytes() << " Bps;" << std::endl;
  }
  LOG_EVERYTHING_OUT << std::endl;

  if (coded) {
    for(int n = 0; n < 13; ++n) {
      proxy[n]->ListMeasuredBytes (LOG_EVERYTHING_OUT);
    }
    LOG_EVERYTHING_OUT << std::endl;
  }
}

#endif  // __MEASUREMENT_UTILITIES__