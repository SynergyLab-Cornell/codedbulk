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
#ifndef CodedBulk_PROXY_H
#define CodedBulk_PROXY_H

#include "tcp-proxy.h"
#include "CodedBulk-codec-manager.h"
#include <fstream>
#include <stdio.h>  // remove file
#include "system_parameters.h"

class CodedBulkProxy : public TcpProxy {
public:
  static uint32_t __input_buffer_size;
  static void setInputBufferSize (uint32_t size);
  static void setDefaultInputBufferSize ();

  CodedBulkProxy ();
  ~CodedBulkProxy ();

  virtual void SetCodecManager (Ptr<CodedBulkCodecManager> codec_manager);
  virtual CodedBulkCodecManager* GetCodecManager (void);

  void ReceiveOutput(CodedBulkOutput* output);
  void ReleaseRecv (bool simple_forward, void* temp_pointer);

  // for store and forward
  void StoreOutput(CodedBulkOutput* output);

  void ListMeasuredBytes (std::ostream& os);

  // is the recv buffer which receives from network links full?
  bool IsRecvBufferFull (void) const;

  void InitializeMeasureParameters (void);
  void Measuring(void);

  void WakeAllThreads(void);

protected:
  virtual int DoSend    (TcpSendPeer*    send_peer);
  virtual int DoReceive (TcpReceivePeer* recv_peer);

  // the core of the proxy
  virtual void ProxySendLogic (TcpSendPeer*    send_peer);
  virtual void ProxyRecvLogic (TcpReceivePeer* recv_peer);

  typedef struct _CodedBulk_forward_file_ {
    _CodedBulk_forward_file_ (std::string name) {
      _name = name;
    }
    ~_CodedBulk_forward_file_ () {
      _file.close();
      remove(_name.c_str());
    }
    std::ifstream& GetFile() {
      if (!_file.is_open())
        _file.open(_name.c_str());
      return _file;
    }
    std::string   _name;
    std::ifstream _file;
  } CodedBulk_forward_file;

  enum _CodedBulk_send_state_ {
    send_state__store_file_non_empty,
    send_state__local_buffer_check,
    send_state__local_buffer_move,
    send_state__default
  };

  typedef struct _CodedBulk_send_info_ {
    enum _CodedBulk_send_state_       _send_state;

    std::mutex                 _send_buffer_lock;

    // to avoid exhuasting all the computing resources spinning
    std::condition_variable    _send_buffer_non_empty_cv;

    // for measuring at tx
    uint64_t                   _tx_totBytes;       //!< Total bytes sent
    uint64_t                   _tx_prevTotBytes;   // last recorded bytes
    uint64_t                   _tx_bytes_interval; // the bytes received within the interval

    CodedBulkOutput*                  _send_output;
    std::list<CodedBulkOutput*>       _send_buffer;
    std::list<CodedBulkOutput*>       _send_local_buffer;

    // for store and forward
    Packet                     _send_store_packet;
    uint32_t                   _send_file_partition_counter;
    uint32_t                   _send_store_output_size;
    std::ofstream              _send_store_file;
    std::string                _send_store_file_name;
    std::list<CodedBulk_forward_file> _send_forward_files;

    uint32_t                   _serial_number;

    _CodedBulk_send_info_ () :
      _tx_totBytes(0),
      _tx_prevTotBytes(0),
      _tx_bytes_interval(0),
      _send_output(nullptr),
      _send_file_partition_counter(0),
      _send_store_output_size(0),
      _serial_number(1)
    {
      _send_buffer.clear();
      _send_local_buffer.clear();
      _send_forward_files.clear();
    }

    ~_CodedBulk_send_info_ ()
    {
      _send_buffer.clear();
      _send_local_buffer.clear();

      if (_send_store_file.is_open()) {
        _send_store_file.close();
      }
    
      _send_forward_files.clear();
    }
  } CodedBulk_send_info;

  typedef struct _CodedBulk_recv_info_ {
    std::mutex                 _counter_lock;

    // to avoid exhuasting all the computing resources spinning
    std::condition_variable    _recv_buffer_room_cv;

    Packet*                    _recv_packet;
    uint32_t                   _counter;

    uint32_t                   _serial_number;

    // for measuring at rx
    uint64_t                   _rx_totBytes;       //!< Total bytes received
    uint64_t                   _rx_prevTotBytes;   // last recorded bytes
    uint64_t                   _rx_bytes_interval; // the bytes received within the interval

    MemoryAllocator<Packet>    _alloc_packet;
    MemoryAllocator<CodedBulkInput>   _alloc_input;
    MemoryAllocator<CodedBulkOutput>  _alloc_output;
    MemoryAllocator<CodedBulkTask>      _alloc_task;
    MemoryAllocator<CodedBulkReadyTask> _alloc_ready_task;

    _CodedBulk_recv_info_ (uint32_t input_buffer_size) :
      _recv_packet (nullptr),
      _counter(0),
      _serial_number(0),
      _rx_totBytes(0),
      _rx_prevTotBytes(0),
      _rx_bytes_interval(0),
      _alloc_packet(2*input_buffer_size),
      _alloc_input(2*input_buffer_size),
      _alloc_output(2*input_buffer_size),  // forward packet
      // a packet might be needed by several codecs (suppose a packet is at most needed by 5 codecs)
      _alloc_task(2*MAX_CODEC_INPUT_SIZE*input_buffer_size),
      _alloc_ready_task(2*MAX_CODEC_INPUT_SIZE*input_buffer_size)
    {
    }
  } CodedBulk_recv_info;

  virtual void* GetSendInfo (uint32_t path_id);
  virtual void* GetRecvInfo (uint32_t path_id, bool use_large_buffer);

  virtual void NewSendPeer (uint32_t path_id, void* send_peer);
  virtual void NewRecvPeer (uint32_t path_id, void* recv_peer);

  std::unordered_map<uint32_t,CodedBulk_recv_info*>  m_recv;
  std::unordered_map<uint32_t,CodedBulk_send_info*>  m_send;

  std::mutex m_recv_access_lock;
  std::mutex m_send_access_lock;
  
  CodedBulkCodecManager* m_codec_manager;

  // to test if the buffer is full
  bool            m_recv_buffer_full {false};
};

#endif /* CodedBulk_PROXY_H */