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
#include "ns3/simulator.h"
#include "CodedBulk-codec.h"
#include "CodedBulk-codec-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkCodec");

void
CodedBulkInput::initialize () {
    _input_packet = NULL;
    _counter = 0;
}

void
CodedBulkInput::release () {
}

void
CodedBulkInput::decrementCounter () {
    if(_counter > 0)  --_counter;
    if((_counter == 0)&&(_input_packet != NULL)) {
        _input_packet = NULL;
    }
}

CodedBulkTask::CodedBulkTask (uint32_t serial_number, int dim) {
    initialize (serial_number,dim);
}

CodedBulkTask::CodedBulkTask (CodedBulkTask& task) {
    _num_received_inputs = task._num_received_inputs;
    _max_packet_length    = task._max_packet_length;
    _serial_number        = task._serial_number;
    for(int i = 0; i < MAX_CODEC_INPUT_SIZE; ++i) {
        _received_inputs[i] = task._received_inputs[i];
    }
}

CodedBulkTask::~CodedBulkTask () {
    cancelTimeoutEvent();
}

void
CodedBulkTask::cancelTimeoutEvent (void)
{
    if (_timeout_event.IsRunning () ) {
        Simulator::Cancel (_timeout_event);
    }
}

void
CodedBulkOutput::initialize () {
    _output_packet = NULL;
    _path_id = 0;
    _serial_number = 0;
    _notifier = NULL;
    _simple_forward = false;
}

void
CodedBulkOutput::release () {
    if(_output_packet != NULL) {
        _output_packet = NULL;
    }
    if(_notifier != NULL) {
        _notifier->_notifier_lock.lock();
        --_notifier->_num_not_yet_output;
        if(_notifier->_num_not_yet_output == 0) {
            _notifier->_notifier_lock.unlock();
            _notifier->Delete();
        } else {
            _notifier->_notifier_lock.unlock();
        }
    }
}

bool CodedBulkCodec::__store_and_foward = false;

void
CodedBulkCodec::setStoreAndForward (bool store_and_forward) {
    __store_and_foward = store_and_forward;
}

CodedBulkCodec::CodedBulkCodec(CodedBulkCodecManager* codec_manager) :
  _codec_manager(codec_manager),
  _virtual_link(NULL)
{
    resetCodec();
}

CodedBulkCodec::CodedBulkCodec(CodedBulkCodecManager* codec_manager, VirtualLink* virtual_link) :
  _codec_manager(codec_manager),
  _virtual_link(NULL)
{
    resetCodec();
    assignVirtualLink(virtual_link);
    initCodec();
}

CodedBulkCodec::CodedBulkCodec(CodedBulkCodecManager* codec_manager, CodedBulkCodec& codec) :
  _codec_manager(codec_manager),
  _virtual_link(NULL)
{
    resetCodec();
    assignVirtualLink(codec._virtual_link);
    codec._virtual_link = NULL;
    initCodec();
}

CodedBulkCodec::~CodedBulkCodec() {
    resetCodec();
}

void
CodedBulkCodec::setTimeoutPeriod(Time timeout_period) {
    _timeout_period = timeout_period;
}

CodeVector
CodedBulkCodec::coding (const CodeVector& input) const {
    return (*_virtual_link) * input;
}

void
CodedBulkCodec::coding (
    CodedBulkTask* task,
    uint8_t** received_packet,
    MemoryAllocator<CodedBulkOutputNotifier>* alloc_notifier,
    MemoryAllocator<CodedBulkOutput>*         alloc_output
) {
    // expand received packets
    for(int i = 0; i < task->_num_received_inputs; ++i) {
        Ptr<Packet>& packet = task->_received_inputs[i]->_input_packet;
        packet->CopyData(received_packet[i],packet->GetSize());
    }

    uint8_t output_buffer[DATASEG_SIZE]; // 1000 >= task->_max_packet_length...?
    // to be multithread safe -> do memory handling ourselves
    CodedBulkOutput* output = NULL;
    CodedBulkOutputNotifier* notifier = alloc_notifier->New();
    notifier->_num_not_yet_output = (uint32_t)_virtual_link->getRowDimension();
    notifier->_task = task;
    for(int pkt = 0; pkt < _virtual_link->getRowDimension(); ++pkt) {
        output = alloc_output->New();
        for(size_t pos = 0; pos < task->_max_packet_length; ++pos) {
            GF256 encode_unit = 0;
            for(int i = 0; i < _virtual_link->getColDimension(); ++i) {
                encode_unit += (*_virtual_link)[pkt][i]*(received_packet[i][pos]);
            }
            output_buffer[pos] = *encode_unit;
        }
        output->_output_packet = Create<Packet> (output_buffer, task->_max_packet_length);
        output->_to_send_peer = NULL;
        if(_virtual_link->_send_peers != NULL) {
            output->_to_send_peer = _virtual_link->_send_peers[pkt];
        }
        // notice that _virtual_link->_send_peers[pkt] is not necessarily non NULL
        output->_path_id = _virtual_link->_output_paths[pkt];
        output->_serial_number = task->_serial_number;
        output->_notifier = notifier;
        if (__store_and_foward) {
            _codec_manager->storeOutput(output);
        } else {
            _codec_manager->receiveOutput(output);
        }
    }

    // don't erase the task until its packets are all out

    // release the input packet if we no longer need it
    // this will disturb the recv counter control...
    /*
    for(int i = 0; i < task->_num_received_inputs; ++i) {
        task->_received_inputs[i]->_check_lock.lock();
        if ( --task->_received_inputs[i]->_wait_to_code_counter == 0 ) {
            task->_received_inputs[i]->_input_packet = nullptr;
        }
        task->_received_inputs[i]->_check_lock.unlock();
    }
    */
}

int
CodedBulkCodec::receiveInput(
    uint32_t path_id,
    uint32_t serial_number,
    CodedBulkInput* input,
    MemoryAllocator<CodedBulkTask>*      alloc_task,
    MemoryAllocator<CodedBulkReadyTask>* alloc_ready_task
) {
    int input_id = _virtual_link->_input_paths[path_id];
    if( input_id == -1 ) {
        // no mapping exists
        return -1;
    }

    input->incrementCounter();

    CodedBulkTask* task = NULL;

    _codec_lock.lock();
    std::pair<std::unordered_map<uint32_t, CodedBulkTask*>::iterator, bool> result = _tasks.insert(std::pair<uint32_t, CodedBulkTask*>(serial_number,NULL));

    if( result.second ) {
        task = alloc_task->New();
        task->initialize(serial_number, _virtual_link->getColDimension());
        task->_virtual_link = _virtual_link;
        result.first->second = task;
    } else {
        task = result.first->second;
    }

    if( task->_received_inputs[input_id] == NULL ) {
        if (task->_num_received_inputs == _virtual_link->getColDimension() - 1)
        {
            // the task is ready
            _tasks.erase (result.first);
            _codec_lock.unlock();

            // to improve the performance, do it after removal
            task->_received_inputs[input_id] = input;
            if(task->_max_packet_length < input->_input_packet->GetSize()){
                task->_max_packet_length = input->_input_packet->GetSize();
            }
            ++task->_num_received_inputs;

            CodedBulkReadyTask* ready_task = alloc_ready_task->New();
            ready_task->initialize(this,task);
            _codec_manager->enqueueTask (ready_task);
            return 1;
        }
        task->_received_inputs[input_id] = input;
        if(task->_max_packet_length < input->_input_packet->GetSize()){
            task->_max_packet_length = input->_input_packet->GetSize();
        }
        ++task->_num_received_inputs;
        // reset timeout -> never timeout?
        //task->cancelTimeoutEvent ();
        //task->_timeout_event = Simulator::Schedule (_timeout_period, &CodedBulkCodec::timeout, this, serial_number);
        // receive duplicated packet
        _codec_lock.unlock();
    } else {
        // discard duplicated packet
        _codec_lock.unlock();
        input->decrementCounter ();
    }

    return 0;
}

CodedBulkCodec&
CodedBulkCodec::operator=(const CodedBulkCodec& codec) {
    _virtual_link = codec._virtual_link;
    return *this;
}

void
CodedBulkCodec::timeout (uint32_t serial_number)
{
    if(_tasks.find(serial_number) != _tasks.end()) {
        _tasks[serial_number]->Delete();
    }
    _tasks.erase(serial_number);
}

void
CodedBulkCodec::listCodeMatrix (std::ostream& os) const
{
    _virtual_link->listMatrix(os);
}

void
CodedBulkCodec::listVirtualLink    (std::ostream& os) const
{
    _virtual_link->listMap(os);
}

void
CodedBulkCodec::resetCodec(void) {
    _codec_lock.lock();
    if(_virtual_link != NULL) {
        delete _virtual_link;
    }
    _virtual_link = NULL;
    _tasks.clear();
    _codec_lock.unlock();
}

void
CodedBulkCodec::assignVirtualLink (VirtualLink* virtual_link) {
    if(_virtual_link != NULL) {
        delete _virtual_link;
    }
    _virtual_link = virtual_link;
}

void
CodedBulkCodec::initCodec(void) {
    // timeout is 10 ms by default
    _timeout_period = MilliSeconds(1000.0);
}

// for faster access
void
CodedBulkCodec::InsertSendPeer(uint32_t path_id, void* send_peer)
{
    _codec_lock.lock();
    if(_virtual_link != NULL) {
        _virtual_link->InsertSendPeer(path_id,send_peer);
    }
    _codec_lock.unlock();
}

void
CodedBulkCodec::InsertRecvPeer(uint32_t path_id, void* recv_peer)
{
    _codec_lock.lock();
    if(_virtual_link != NULL) {
        _virtual_link->InsertRecvPeer(path_id,recv_peer);
    }
    _codec_lock.unlock();
}

} // Namespace ns3