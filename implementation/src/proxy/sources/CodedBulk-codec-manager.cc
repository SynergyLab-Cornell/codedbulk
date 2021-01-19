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
#include "CodedBulk-codec-manager.h"
#include "CodedBulk-proxy.h"
#include <utility>
#include "system_parameters.h"

//#define WITHOUT_WORKER_THREAD

extern SystemParameters system_parameters;

uint32_t CodedBulkCodecManager::__max_num_worker_threads   = MAX_NUM_WORKER_THREADS;
uint32_t CodedBulkCodecManager::__worker_thread_batch_size = WORKER_THREAD_BATCH_SIZE;

CodedBulkCodecManager::CodedBulkCodecManager () :
  _CodedBulkProxy(nullptr),
  _CodedBulkReadyTasks(nullptr),
  _stop(false),
  _num_uninitialized_threads(0),
  _num_waiting_threads(0),
  _num_worker_threads(0)
#ifdef WITHOUT_WORKER_THREAD
  , _alloc_notifier(2*MAX_CODEC_INPUT_SIZE*MAX_PROXY_INPUT_BUFFER_SIZE)
  , _alloc_output(2*MAX_CODEC_INPUT_SIZE*MAX_PROXY_INPUT_BUFFER_SIZE)
  , _alloc_packet(2*MAX_CODEC_INPUT_SIZE*MAX_PROXY_INPUT_BUFFER_SIZE)
#endif
{
#ifdef WITHOUT_WORKER_THREAD
    for(int i = 0; i < MAX_CODEC_INPUT_SIZE; ++i) {
        _received_packet[i] = _buf[i];
    }
#endif
    clearCodedBulkCodecs();
}

CodedBulkCodecManager::~CodedBulkCodecManager ()
{
#ifndef WITHOUT_WORKER_THREAD
    {
        std::unique_lock<std::mutex> lock(_CodedBulkReadyTasks_lock);
        _stop = true;
    }
    _worker_condition.notify_all();
    for(auto& worker: _worker_threads)
        worker.join();
#endif
    clearCodedBulkCodecs ();
}

void
CodedBulkCodecManager::clearCodedBulkCodecs ()
{
    for(std::list<CodedBulkCodec*>::iterator
        it  = _CodedBulkCodecs.begin();
        it != _CodedBulkCodecs.end();
        ++it
    ) {
        delete (*it);
    }
    _CodedBulkCodecs.clear();

    while(_CodedBulkReadyTasks != nullptr) {
        CodedBulkReadyTask* task = _CodedBulkReadyTasks;
        _CodedBulkReadyTasks = _CodedBulkReadyTasks->_next_task;
        task->Delete ();
    }
    _CodedBulkReadyTasks_last = nullptr;
    _CodedBulkReadyTasks_size = 0;
    _CodedBulkReadyTasks_cached_pointers.clear();
}

CodedBulkCodec*
CodedBulkCodecManager::addCodedBulkCodec() {
    return addCodedBulkCodec (nullptr);
}

CodedBulkCodec*
CodedBulkCodecManager::addCodedBulkCodec(VirtualLink* virtual_link) {
    CodedBulkCodec* codec = new CodedBulkCodec (this,virtual_link);
    _CodedBulkCodecs.push_back(codec);
    return codec;
}

int
CodedBulkCodecManager::receiveInput(
    uint32_t path_id,
    uint32_t serial_number,
    CodedBulkInput* input,
    MemoryAllocator<CodedBulkTask>*      alloc_task,
    MemoryAllocator<CodedBulkReadyTask>* alloc_ready_task
) {
    int ret = 0;
    for(std::list<CodedBulkCodec*>::iterator
        it  = _CodedBulkCodecs.begin();
        it != _CodedBulkCodecs.end();
        ++it
    ) {
        // feed the pacekt to all the encoders
        if( (*it)->receiveInput(
                path_id,
                serial_number,
                input,
                alloc_task,
                alloc_ready_task
            ) != -1 
        ) {
            // the packet is received
            ++ret;
        }
    }
    return ret;
}

void
CodedBulkCodecManager::listVirtualLinks (std::ostream& os) const
{
    for(std::list<CodedBulkCodec*>::const_iterator
        it  = _CodedBulkCodecs.begin();
        it != _CodedBulkCodecs.end();
        ++it) {
        (*it)->listVirtualLink(os);
    }
}

// for faster access
void
CodedBulkCodecManager::InsertRecvPeer(uint32_t path_id, void* recv_peer)
{
    for(std::list<CodedBulkCodec*>::const_iterator
        it  = _CodedBulkCodecs.begin();
        it != _CodedBulkCodecs.end();
        ++it) {
        (*it)->InsertRecvPeer(path_id,recv_peer);
    }
}

void
CodedBulkCodecManager::InsertSendPeer(uint32_t path_id, void* send_peer)
{
    for(std::list<CodedBulkCodec*>::const_iterator
        it  = _CodedBulkCodecs.begin();
        it != _CodedBulkCodecs.end();
        ++it) {
        (*it)->InsertSendPeer(path_id,send_peer);
    }
}

void
CodedBulkCodecManager::receiveOutput(CodedBulkOutput* output) const
{
    if(_CodedBulkProxy != nullptr) {
        _CodedBulkProxy->ReceiveOutput(output);
    } else {
        output->Delete();
    }
}

void
CodedBulkCodecManager::storeOutput (CodedBulkOutput* output) const
{
    if(_CodedBulkProxy != nullptr) {
        _CodedBulkProxy->StoreOutput(output);
    } else {
        output->Delete();
    }
}

void
CodedBulkCodecManager::scheduleTasks (void)
{
    if(_stop) {
        return;
    }
    bool coding_success = false;

    _CodedBulkReadyTasks_lock.lock();
#ifdef WITHOUT_WORKER_THREAD
    while(_CodedBulkReadyTasks != nullptr) {
        CodedBulkReadyTask* task = _CodedBulkReadyTasks;
        _CodedBulkReadyTasks = _CodedBulkReadyTasks->_next_task;
        --_CodedBulkReadyTasks_size;
        if(_CodedBulkReadyTasks_size == 0) {
            _CodedBulkReadyTasks_last = nullptr;
        }
        _CodedBulkReadyTasks_lock.unlock();
        coding_success = task->run(
            _received_packet,
            &_alloc_notifier,
            &_alloc_output,
            &_alloc_packet
        );
        task->Delete ();
        _CodedBulkReadyTasks_lock.lock();
    }
#else
    while(system_parameters.isRunning() && schedule_condition()) {
        scheduling ();
        _CodedBulkReadyTasks_lock.unlock();
        _CodedBulkReadyTasks_lock.lock();
    }
#endif
    _CodedBulkReadyTasks_lock.unlock();
}

void
CodedBulkCodecManager::enqueueTask (CodedBulkReadyTask* ready_task)
{
    _CodedBulkReadyTasks_lock.lock();
    if(_CodedBulkReadyTasks_last == nullptr) {
        _CodedBulkReadyTasks_last = ready_task;
        _CodedBulkReadyTasks = _CodedBulkReadyTasks_last;
    } else {
        _CodedBulkReadyTasks_last->_next_task = ready_task;
        _CodedBulkReadyTasks_last = _CodedBulkReadyTasks_last->_next_task;
    }
#ifndef WITHOUT_WORKER_THREAD
    if((_CodedBulkReadyTasks_size > 0) &&
       (_CodedBulkReadyTasks_size % __worker_thread_batch_size == 0)) {
        _CodedBulkReadyTasks_cached_pointers.push_back(_CodedBulkReadyTasks_last);
    }
#endif
    ++_CodedBulkReadyTasks_size;
    _CodedBulkReadyTasks_lock.unlock();
}

void
CodedBulkCodecManager::spawnWorkerThreads (uint32_t num_worker_threads)
{
    uint32_t current_num_worker_threads = _num_worker_threads;
    _num_worker_threads += num_worker_threads;
    if(_num_worker_threads > __max_num_worker_threads) {
        _num_worker_threads = __max_num_worker_threads;
    }
    for(;current_num_worker_threads < _num_worker_threads; ++current_num_worker_threads) {
        ++_num_uninitialized_threads;
        _worker_threads.emplace_back(&CodedBulkCodecManager::working,this);
    }
}

void
CodedBulkCodecManager::scheduling ()
{
    if((_num_waiting_threads + _num_uninitialized_threads < _CodedBulkReadyTasks_size) && 
       (_num_worker_threads < __max_num_worker_threads)) {
        //spawnWorkerThreads(1);
        ++_num_uninitialized_threads;
        ++_num_worker_threads;
        _worker_threads.emplace_back(&CodedBulkCodecManager::working,this);
    }
    _worker_condition.notify_one();
}

void
CodedBulkCodecManager::working ()
{
    uint8_t* received_packet[MAX_CODEC_INPUT_SIZE];

    MemoryAllocator<CodedBulkOutputNotifier> alloc_notifier;
    MemoryAllocator<CodedBulkOutput>         alloc_output;
    MemoryAllocator<Packet>           alloc_packet;

    bool initialized = false;
    bool coding_success = false;

    uint32_t num_local_tasks = 0;
    uint32_t num_failed_tasks = 0;
    // coding working thread
    CodedBulkReadyTask* local_tasks = nullptr;
    CodedBulkReadyTask* local_task_now = nullptr;
    CodedBulkReadyTask* local_failed_tasks = nullptr;
    while(system_parameters.isRunning()){
        if (local_tasks == nullptr) {
            std::unique_lock<std::mutex> lock(_CodedBulkReadyTasks_lock);
            if(!initialized) {
                --_num_uninitialized_threads;
                initialized = true;
            }
            ++_num_waiting_threads;
            _worker_condition.wait(lock, [this] {  return _stop || (_CodedBulkReadyTasks != nullptr);  });

            // being waken up
            --_num_waiting_threads;

            if(_CodedBulkReadyTasks == nullptr) {
                //if(_stop)
                //    return;
                continue;
            }

            local_tasks = _CodedBulkReadyTasks;

            if(_CodedBulkReadyTasks_size > __worker_thread_batch_size) {
                num_local_tasks = __worker_thread_batch_size;
                _CodedBulkReadyTasks_size -= __worker_thread_batch_size;
                _CodedBulkReadyTasks = _CodedBulkReadyTasks_cached_pointers.front();
                _CodedBulkReadyTasks_cached_pointers.pop_front();
            } else {
                num_local_tasks = _CodedBulkReadyTasks_size;
                _CodedBulkReadyTasks_size = 0;
                _CodedBulkReadyTasks = nullptr;
                _CodedBulkReadyTasks_last = nullptr;
            }
        }

        local_failed_tasks = nullptr;
        num_failed_tasks = 0;
        for(uint32_t i = 0; i < num_local_tasks; ++i) {
            local_task_now = local_tasks;
            local_tasks = local_tasks->_next_task;
            coding_success = local_task_now->run(
                received_packet,
                &alloc_notifier,
                &alloc_output,
                &alloc_packet
            );
            if (coding_success) {
                local_task_now->Delete ();
            } else {
                local_task_now->_next_task = local_failed_tasks;
                local_failed_tasks = local_task_now;
                ++num_failed_tasks;
            }
        }
        //num_local_tasks = 0;
        local_task_now = nullptr;
        local_tasks = local_failed_tasks;
        num_local_tasks = num_failed_tasks;
    }

    // we need to release allocs in order
    // output depends on packet and notifier
    // manually free memory allocators:
    for (int i = 0; i < alloc_output._size; ++i) {
        alloc_output._data[i].Delete();
    }
    for (int i = 0; i < alloc_notifier._size; ++i) {
        alloc_notifier._data[i].Delete();
    }
}
