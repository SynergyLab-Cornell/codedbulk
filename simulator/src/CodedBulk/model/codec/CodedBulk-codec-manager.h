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

#ifndef CodedBulk_CODEC_MANAGER_H
#define CodedBulk_CODEC_MANAGER_H

#include "CodedBulk-codec.h"
#include "ns3/CodedBulk-system-parameters.h"

#include <ostream>
#include <list>
#include <queue>
#include <mutex>

// thread pool libraries
#include <thread>
#include <condition_variable>

#define WITHOUT_WORKER_THREAD
#define MAX_NUM_WORKER_THREADS 100
#define WORKER_THREAD_BATCH_SIZE 200

namespace ns3 {

class CodedBulkProxy;

class CodedBulkReadyTask : public MemoryElement {
public:
    CodedBulkCodec* _codec;
    CodedBulkTask*  _task;
    CodedBulkReadyTask* _next_task;

    CodedBulkReadyTask () {
        initialize ();
    }
    CodedBulkReadyTask (CodedBulkCodec* codec, CodedBulkTask* task) {
        initialize (codec,task);
    }
    
    virtual void initialize () {
        _codec = NULL;
        _task = NULL;
        _next_task = NULL;
    }
    inline void initialize (CodedBulkCodec* codec, CodedBulkTask* task) {
        _codec = codec;
        _task = task;
        _next_task = NULL;
    }

    inline void run(
        uint8_t** received_packet,
        MemoryAllocator<CodedBulkOutputNotifier>* alloc_notifier,
        MemoryAllocator<CodedBulkOutput>*         alloc_output
    ) {
        _codec->coding(
            _task,
            received_packet,
            alloc_notifier,
            alloc_output
        );
    }
};

class CodedBulkCodecManager : public SimpleRefCount<CodedBulkCodecManager> {
public:
    static void SetMaxNumWorker (uint32_t num_worker) {  __max_num_worker_threads = num_worker;  }
    static void SetThreadBatchSize (uint32_t batch_size) {  __worker_thread_batch_size = batch_size;  }

    CodedBulkCodecManager ();
    ~CodedBulkCodecManager ();
    void     clearCodedBulkCodecs ();

    CodedBulkCodec* addCodedBulkCodec();
    CodedBulkCodec* addCodedBulkCodec(VirtualLink* virtual_link);

    /*
     * The input packet is a raw data
     * Return codes:
     * 1: packet received
     * 0: packet not received
     */
    int  receiveInput(
        uint32_t path_id,
        uint32_t serial_number,
        CodedBulkInput* input,
        MemoryAllocator<CodedBulkTask>*      alloc_task,
        MemoryAllocator<CodedBulkReadyTask>* alloc_ready_task
    );
    void listVirtualLinks (std::ostream& os) const;
    void receiveOutput (CodedBulkOutput* output) const;

    // for store and forward
    void storeOutput (CodedBulkOutput* output) const;

    // for faster access
    void InsertSendPeer(uint32_t path_id, void* send_peer);
    void InsertRecvPeer(uint32_t path_id, void* recv_peer);

    void scheduleTasks (void);
    void enqueueTask (CodedBulkReadyTask* ready_task);

    void spawnWorkerThreads (uint32_t num_worker_threads);

    inline bool workersIdle (void) {
        #ifdef WITHOUT_WORKER_THREAD
            return true;
        #else
            return (_num_waiting_threads != 0);
        #endif
    }
    inline bool workersFinished (void) {
        // all workers are idle
        #ifdef WITHOUT_WORKER_THREAD
            return true;
        #else
            return (_num_waiting_threads == _num_worker_threads) && (_CodedBulkReadyTasks == NULL);
        #endif
    }

    CodedBulkProxy*             _CodedBulkProxy;
    std::list<CodedBulkCodec*>  _CodedBulkCodecs;

protected:
    // coding thread pool

    // using single link list to improve the performance
    std::mutex    _CodedBulkReadyTasks_lock;
    CodedBulkReadyTask*  _CodedBulkReadyTasks;
    CodedBulkReadyTask*  _CodedBulkReadyTasks_last;
    uint32_t      _CodedBulkReadyTasks_size;
    // record one pointer per WORKER_THREAD_BATCH_SIZE tasks for faster batch transfer
    std::list<CodedBulkReadyTask*> _CodedBulkReadyTasks_cached_pointers;

    inline bool schedule_condition() {
        if(_CodedBulkReadyTasks == NULL)  return false;
        if((_num_waiting_threads + _num_uninitialized_threads < _CodedBulkReadyTasks_size) &&
           (_num_worker_threads < __max_num_worker_threads))  return true;
        if(_num_waiting_threads > 0)  return true;
        return false;
    }

    void scheduling ();
    void working ();

    static uint32_t __max_num_worker_threads;
    static uint32_t __worker_thread_batch_size;

    std::deque<std::thread>   _worker_threads;
    std::condition_variable   _worker_condition;
    bool _stop;
    uint32_t  _num_uninitialized_threads;  // the threads that are just spawned
    uint32_t  _num_waiting_threads;        // the threads that can be waken up
    uint32_t  _num_worker_threads;         // total number of worker threads

#ifdef WITHOUT_WORKER_THREAD
    uint8_t  _buf[MAX_CODEC_INPUT_SIZE][DATASEG_SIZE];
    uint8_t* _received_packet[MAX_CODEC_INPUT_SIZE];

    MemoryAllocator<CodedBulkOutputNotifier> _alloc_notifier;
    MemoryAllocator<CodedBulkOutput>         _alloc_output;
#endif
};

} // namespace ns3

#endif /* CodedBulk_CODEC_MANAGER_H */