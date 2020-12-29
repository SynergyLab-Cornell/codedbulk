/***********
 * Multithread Coding Experiment
 ***********/
#include "CodedBulk-proxy.h"
#include "measure_tools.h"
#include <type_traits>
#include <iostream>
#include <fstream>
using namespace std;

#define TOTAL_DATA 100000000
#define NUM_WORKER_THREAD 20

using clock_type = typename std::conditional<
  std::chrono::high_resolution_clock::is_steady,
  std::chrono::high_resolution_clock,
  std::chrono::steady_clock>::type;

int main(int argc, char* argv[]){
  CodedBulkInput* pkt0 = new CodedBulkInput ();
  pkt0->_input_packet = new Packet ();
  pkt0->_input_packet->SetSize(1000);

  CodedBulkInput* pkt1 = new CodedBulkInput ();
  pkt1->_input_packet = new Packet ();
  pkt0->_input_packet->SetSize(1000);

  ofstream fout("results/batch_to_time.dat");

  MemoryAllocator<CodedBulkTask>      alloc_task(TOTAL_DATA);
  MemoryAllocator<CodedBulkReadyTask> alloc_ready_task(TOTAL_DATA);

  for(uint32_t batch_size = 1; batch_size < 100000; batch_size *= 10) {
    uint32_t serial_number = 0;
    std::chrono::high_resolution_clock::time_point begin_time;
    std::chrono::high_resolution_clock::time_point end_time;

    Ptr<CodedBulkCodecManager> codec_manager = Create<CodedBulkCodecManager> ();
    codec_manager->SetMaxNumWorker(NUM_WORKER_THREAD);
    codec_manager->spawnWorkerThreads(NUM_WORKER_THREAD);
    codec_manager->SetThreadBatchSize(batch_size);
    // wait until the threads are all spawned.
    while(!codec_manager->workersFinished()) {
      SleepForOneSec();
    }

    VirtualLink* code_map;
    code_map = new VirtualLink (1,2);
    (*code_map)[0][0] = 5;
    (*code_map)[0][1] = 3;
    code_map->_input_paths  << 1000 << 1001;
    code_map->_output_paths << 1000;
    codec_manager->addCodedBulkCodec (code_map);

    // we can't simply use clock_t for multithreaded program
    //clock_t begin_time = clock();
    while(serial_number < TOTAL_DATA) {
      // insert packet to code
      codec_manager->receiveInput(
        1000,
        serial_number,
        pkt0,
        &alloc_task,
        &alloc_ready_task
      );
      codec_manager->receiveInput(
        1001,
        serial_number++,
        pkt1,
        &alloc_task,
        &alloc_ready_task
      );
    }
    begin_time = std::chrono::high_resolution_clock::now();
    codec_manager->scheduleTasks ();
    while(!codec_manager->workersFinished()) {
      SleepForOneMilliSec();
    }
    end_time = std::chrono::high_resolution_clock::now();

    double elapsed_millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - begin_time).count();
    //double elapsed_millisecs = double(end_time - begin_time) / CLOCKS_PER_SEC * 1000;
    double throughput = double(TOTAL_DATA)/elapsed_millisecs*8/1000.0;
    cout << "with batch " << batch_size
         << " in " << elapsed_millisecs << " milliseconds. throughput = " << throughput << " Gbps." << endl;
    // # of workers, elapsed milliseconds, throughput Gbps (10^9 bps)
    fout << batch_size << "\t" << elapsed_millisecs << "\t" << throughput << endl;
  }

  delete pkt0->_input_packet;
  delete pkt0;
  delete pkt1->_input_packet;
  delete pkt1;

  fout.close();
  return 0;
}