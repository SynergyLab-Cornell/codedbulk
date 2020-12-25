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
#include "CodedBulk-codec-manager.h"
#include "ns3/CodedBulk-proxy.h"
#include "ns3/CodedBulk-system-parameters.h"
#include <utility>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkCodecManager");

uint32_t CodedBulkCodecManager::__max_num_worker_threads   = MAX_NUM_WORKER_THREADS;
uint32_t CodedBulkCodecManager::__worker_thread_batch_size = WORKER_THREAD_BATCH_SIZE;

CodedBulkCodecManager::CodedBulkCodecManager () :
  _CodedBulkProxy(NULL),
  _CodedBulkReadyTasks(NULL),
  _stop(false),
  _num_uninitialized_threads(0),
  _num_waiting_threads(0),
  _num_worker_threads(0)
#ifdef WITHOUT_WORKER_THREAD
  , _alloc_notifier(2*MAX_CODEC_INPUT_SIZE*MAX_PROXY_INPUT_BUFFER_SIZE)
  , _alloc_output(2*MAX_CODEC_INPUT_SIZE*MAX_PROXY_INPUT_BUFFER_SIZE)
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

    while(_CodedBulkReadyTasks != NULL) {
        CodedBulkReadyTask* task = _CodedBulkReadyTasks;
        _CodedBulkReadyTasks = _CodedBulkReadyTasks->_next_task;
        task->Delete ();
    }
    _CodedBulkReadyTasks_last = NULL;
    _CodedBulkReadyTasks_size = 0;
    _CodedBulkReadyTasks_cached_pointers.clear();
}

CodedBulkCodec*
CodedBulkCodecManager::addCodedBulkCodec() {
    return addCodedBulkCodec (NULL);
}

CodedBulkCodec*
CodedBulkCodecManager::addCodedBulkCodec(VirtualLink* code_map) {
    CodedBulkCodec* codec = new CodedBulkCodec (this,code_map);
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
    //NS_LOG_DEBUG("receive serial number " << serial_number << " at path " << path_id);
    int ret = 0;
    /*
    input->_check_lock.lock();
    input->_wait_to_code_counter = _CodedBulkCodecs.size();
    input->_check_lock.unlock();
    */
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
    /*
    input->_check_lock.lock();
    input->_wait_to_code_counter += ret;
    input->_wait_to_code_counter -= _CodedBulkCodecs.size();
    if (input->_wait_to_code_counter == 0) {
        input->_input_packet = nullptr;
    }
    input->_check_lock.unlock();
    */
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
    if(_CodedBulkProxy != NULL) {
        _CodedBulkProxy->ReceiveOutput(output);
    } else {
        output->Delete();
    }
}

void
CodedBulkCodecManager::storeOutput (CodedBulkOutput* output) const
{
    if(_CodedBulkProxy != NULL) {
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

    _CodedBulkReadyTasks_lock.lock();
#ifdef WITHOUT_WORKER_THREAD
    while(_CodedBulkReadyTasks != NULL) {
        CodedBulkReadyTask* task = _CodedBulkReadyTasks;
        _CodedBulkReadyTasks = _CodedBulkReadyTasks->_next_task;
        --_CodedBulkReadyTasks_size;
        if(_CodedBulkReadyTasks_size == 0) {
            _CodedBulkReadyTasks_last = NULL;
        }
        _CodedBulkReadyTasks_lock.unlock();
        task->run(
            _received_packet,
            &_alloc_notifier,
            &_alloc_output
        );
        task->Delete ();
        _CodedBulkReadyTasks_lock.lock();
    }
#else
    while(schedule_condition()) {
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
    if(_CodedBulkReadyTasks_last == NULL) {
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
    uint8_t  buf[MAX_CODEC_INPUT_SIZE][DATASEG_SIZE];
    uint8_t* received_packet[MAX_CODEC_INPUT_SIZE];
    for(int i = 0; i < MAX_CODEC_INPUT_SIZE; ++i) {
        received_packet[i] = buf[i];
    }
    MemoryAllocator<CodedBulkOutputNotifier> alloc_notifier;
    MemoryAllocator<CodedBulkOutput>         alloc_output;

    bool initialized = false;

    uint32_t num_local_tasks = 0;
    // coding working thread
    CodedBulkReadyTask* local_tasks = NULL;
    CodedBulkReadyTask* local_task_now = NULL;
    for(;;) {
        {
            std::unique_lock<std::mutex> lock(_CodedBulkReadyTasks_lock);
            if(!initialized) {
                --_num_uninitialized_threads;
                initialized = true;
            }
            ++_num_waiting_threads;
            _worker_condition.wait(lock, [this] {  return _stop || (_CodedBulkReadyTasks != NULL);  });

            // being waken up
            --_num_waiting_threads;

            if(_CodedBulkReadyTasks == NULL) {
                if(_stop)
                    return;
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
                _CodedBulkReadyTasks = NULL;
                _CodedBulkReadyTasks_last = NULL;
            }
        }

        for(uint32_t i = 0; i < num_local_tasks; ++i) {
            local_task_now = local_tasks;
            local_tasks = local_tasks->_next_task;
            local_task_now->run(
                received_packet,
                &alloc_notifier,
                &alloc_output
            );
            local_task_now->Delete ();
        }
        num_local_tasks = 0;
        local_tasks = NULL;
        local_task_now = NULL;
    }
}

} // Namespace ns3