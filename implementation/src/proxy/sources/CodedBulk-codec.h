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

#ifndef CodedBulk_CODEC_H
#define CodedBulk_CODEC_H

#include "CodedBulk-virtual-link.h"
#include "packet.h"
#include "memory-allocator.h"
#include "system_parameters.h"
#include <ostream>
#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>
#include <atomic>

class CodedBulkReadyTask;
class CodedBulkCodecManager;

class CodedBulkInput : public SimpleRefCount<CodedBulkInput>, public MemoryElement {
public:
    CodedBulkInput () {  initialize ();  }
    
    virtual void initialize ();
    virtual void release ();

    inline void incrementCounter () {
      _check_lock.lock();
      ++_counter;
      _check_lock.unlock();
    }
    void decrementCounter ();
    bool hasFinished () {  return (_counter == 0);  }

    Packet*     _input_packet;
    uint32_t    _counter;  // number of outputs depending on this input
    std::mutex  _check_lock;
};

class CodedBulkTask : public SimpleRefCount<CodedBulkTask>, public MemoryElement {
public:
    CodedBulkTask() {}
    CodedBulkTask (uint32_t serial_number, int dim);
    CodedBulkTask (CodedBulkTask& task);

    virtual void initialize () {}
    inline void initialize (uint32_t serial_number, int dim) {
        // dim must < MAX_CODEC_INPUT_SIZE, but we don't check to save computational power
        _num_received_inputs = 0;
        _max_packet_length    = 0;
        _serial_number        = serial_number;
        for(int i = 0; i < dim; ++i) {
            _received_inputs[i] = nullptr;
        }
    }

    VirtualLink*  _code_map;
    uint32_t  _serial_number;
    CodedBulkInput*  _received_inputs[MAX_CODEC_INPUT_SIZE];
    int       _num_received_inputs;   // currently received packets
    size_t    _max_packet_length;
};

class CodedBulkOutputNotifier : public SimpleRefCount<CodedBulkOutputNotifier>, public MemoryElement {
public:
    CodedBulkOutputNotifier() {  initialize ();  }

    virtual void initialize () {
        _num_not_yet_output = 0;
        _task = nullptr;
    }

    // the number of output packets that have not yet been sent
    uint32_t     _num_not_yet_output;
    // the codemap to release the inports
    CodedBulkTask*      _task;

    std::mutex   _notifier_lock;
};

class CodedBulkOutput : public SimpleRefCount<CodedBulkOutput>, public MemoryElement {
public:
    CodedBulkOutput() {  initialize ();  }
    ~CodedBulkOutput() {
        if(_allocated) {
            Delete();
        }
    }

    virtual void initialize ();
    virtual void release ();

    Packet*     _output_packet;
    uint32_t    _path_id;
    uint32_t    _serial_number;
    CodedBulkOutputNotifier* _notifier;

    // to skip header handling
    bool        _simple_forward;
    void*       _from_recv_peer;
    void*       _to_send_peer;
};

class CodedBulkCodec : public SimpleRefCount<CodedBulkCodec> {
public:
    static void setStoreAndForward (bool store_and_forward);
    CodedBulkCodec(CodedBulkCodecManager* codec_manager);
    CodedBulkCodec(CodedBulkCodecManager* codec_manager, VirtualLink* code_map);
    CodedBulkCodec(CodedBulkCodecManager* codec_manager, CodedBulkCodec& codec);
    virtual ~CodedBulkCodec();

    CodeVector coding(const CodeVector& input) const;
    bool       coding (
        CodedBulkTask* task,
        uint8_t** received_packet,
        MemoryAllocator<CodedBulkOutputNotifier>* alloc_notifier,
        MemoryAllocator<CodedBulkOutput>*         alloc_output,
        MemoryAllocator<Packet>*           alloc_packet
    );

    /* 
     * Return codes:
     * 1: task is ready
     * 0: packet received, but task is not yet ready
     * -1: packet is not received
     */
    int        receiveInput(
        uint32_t path_id,
        uint32_t serial_number,
        CodedBulkInput* input,
        MemoryAllocator<CodedBulkTask>*      alloc_task,
        MemoryAllocator<CodedBulkReadyTask>* alloc_ready_task
    );

    CodedBulkCodec&   operator=(const CodedBulkCodec& codec);

    void       timeout (uint32_t serial_number);

    void       listCodeMatrix (std::ostream& os) const;
    void       listVirtualLink    (std::ostream& os) const;

    // for faster access
    void       InsertSendPeer(uint32_t path_id, void* send_peer);
    void       InsertRecvPeer(uint32_t path_id, void* recv_peer);

protected:
    static bool __store_and_foward;

    CodedBulkCodecManager* _codec_manager;
    std::unordered_map<uint32_t, CodedBulkTask* > _tasks; // map serial number to the task

    VirtualLink* _code_map;

    void resetCodec(void);
    void assignVirtualLink (VirtualLink* code_map);
    void initCodec(void);

    std::mutex   _codec_lock;
};

#endif /* CodedBulk_CODEC_H */