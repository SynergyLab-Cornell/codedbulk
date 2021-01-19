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
#include "test_tools.h"
#include "system_parameters.h"

#include "CodedBulk-proxy.h"
#include <sstream>

extern SystemParameters system_parameters;

uint32_t CodedBulkProxy::__input_buffer_size = DEFAULT_PROXY_INPUT_BUFFER_SIZE;

void
CodedBulkProxy::setInputBufferSize (uint32_t size)
{
  if (size < MAX_PROXY_INPUT_BUFFER_SIZE) {
    __input_buffer_size = size;
  } else {
    __input_buffer_size = MAX_PROXY_INPUT_BUFFER_SIZE;
  }
}

void
CodedBulkProxy::setDefaultInputBufferSize ()
{
  __input_buffer_size = DEFAULT_PROXY_INPUT_BUFFER_SIZE;
}

CodedBulkProxy::CodedBulkProxy ()
{
  // for codecs to save their tmp files
  system("mkdir -p tmp_store_and_forward");

  m_recv.clear();
  m_send.clear();
}

CodedBulkProxy::~CodedBulkProxy ()
{
  for(std::unordered_map<uint32_t,CodedBulk_recv_info*>::iterator
    it  = m_recv.begin();
    it != m_recv.end();
    ++it
  ) {
    it->second->_recv_buffer_room_cv.notify_all();
    delete it->second;
  }
  m_recv.clear();

  for(std::unordered_map<uint32_t,CodedBulk_send_info*>::iterator
    it  = m_send.begin();
    it != m_send.end();
    ++it
  ) {
    it->second->_send_buffer_non_empty_cv.notify_all();
    delete it->second;
  }
  m_send.clear();
}

void
CodedBulkProxy::SetCodecManager (Ptr<CodedBulkCodecManager> codec_manager)
{
  m_codec_manager = PeekPointer(codec_manager);
  m_codec_manager->_CodedBulkProxy = this;
} // CodedBulkProxy::SetupCodecManager

CodedBulkCodecManager*
CodedBulkProxy::GetCodecManager (void)
{
  return m_codec_manager;
}

void
CodedBulkProxy::ReceiveOutput(CodedBulkOutput* output)
{
  TcpSendPeer* send_peer = nullptr;
  if(output->_to_send_peer != nullptr) {
    send_peer = (TcpSendPeer*)output->_to_send_peer;
  } else {
    send_peer = GetSendPeer (output->_path_id);
  }
  CodedBulk_send_info* send = (CodedBulk_send_info*)send_peer->m_send_info;

  //bool inserted = false;

  send->_send_buffer_lock.lock();
  if(send->_send_buffer.empty()) {
    send->_send_buffer.push_front(output);
  } else {
    if (output->_serial_number < send->_send_buffer.front()->_serial_number) {
      send->_send_buffer.push_front(output);
    } else {
      send->_send_buffer.push_back(output);
    }
  }
  if(send->_serial_number == send->_send_buffer.front()->_serial_number) {
    send->_send_buffer_non_empty_cv.notify_all();
  }

/*
  // sort when insertion
  for (std::list<CodedBulkOutput*>::reverse_iterator
    rit  = send->_send_buffer.rbegin();
    rit != send->_send_buffer.rend(); ++rit) {
    if((*rit)->_serial_number < output->_serial_number) {
      send->_send_buffer.insert(rit.base(),output);
      inserted = true;
      break;
    }
  }
  if(!inserted) {
    send->_send_buffer.push_front(output);
  }
*/
  send->_send_buffer_lock.unlock();
}

void
CodedBulkProxy::ReleaseRecv (bool simple_forward, void* temp_pointer)
{
  TcpReceivePeer* recv_peer = nullptr;
  CodedBulk_recv_info* recv = nullptr;
  if(simple_forward) {
    recv_peer = (TcpReceivePeer*)temp_pointer;
    recv = (CodedBulk_recv_info*)recv_peer->m_recv_info;
    recv->_counter_lock.lock();
    --recv->_counter;
    recv->_recv_buffer_room_cv.notify_all();
    recv->_counter_lock.unlock();
  } else {
    CodedBulkTask* task = (CodedBulkTask*)temp_pointer;
    VirtualLink* virtual_link = std::move(task->_virtual_link);
    CodedBulkInput* received_inputs[MAX_CODEC_INPUT_SIZE];
    for(int i = 0; i < MAX_CODEC_INPUT_SIZE; ++i) {
      received_inputs[i] = std::move(task->_received_inputs[i]);
    }
    // delete now
    task->Delete();
    // the output is finished
    for(int i = 0; i < virtual_link->getColDimension(); ++i) {
      received_inputs[i]->_check_lock.lock();
      received_inputs[i]->decrementCounter();
      if(received_inputs[i]->hasFinished()) {
        received_inputs[i]->_check_lock.unlock();
        received_inputs[i]->Delete();
        recv_peer = (TcpReceivePeer*)virtual_link->_recv_peers[i];
        recv = (CodedBulk_recv_info*)recv_peer->m_recv_info;
        recv->_counter_lock.lock();
        --recv->_counter;
        recv->_recv_buffer_room_cv.notify_all();
        recv->_counter_lock.unlock();
      } else {
        received_inputs[i]->_check_lock.unlock();
      }
    }
  }
}

#define STORE_FILE_NAME() \
Address tmp_addr = GetBaseAddr(0); \
ss << "tmp_store_and_forward/" << AddressToIPCAddress(tmp_addr) << "_" << output->_path_id << "_" << send->_send_store_output_size

// for store and forward
void
CodedBulkProxy::StoreOutput(CodedBulkOutput* output)
{
  TcpSendPeer* send_peer = nullptr;
  if(output->_to_send_peer != nullptr) {
    send_peer = (TcpSendPeer*)output->_to_send_peer;
  } else {
    send_peer = GetSendPeer (output->_path_id);
  }
  CodedBulk_send_info* send = (CodedBulk_send_info*)send_peer->m_send_info;

  uint32_t packet_size = output->_output_packet->GetSize();

  // save the output to file
  send->_send_buffer_lock.lock();
  if (send->_send_store_file_name.empty()) {
    std::stringstream ss;
    STORE_FILE_NAME();
    send->_send_store_file_name = ss.str();
    send->_send_store_file.open(ss.str().c_str());
  }

  // save output->_output_packet to file
  output->_output_packet->CopyData(&send->_send_store_file,packet_size);

  send->_send_file_partition_counter += packet_size;
  send->_send_store_output_size += packet_size;

  // if we have gather enough data, forward it
  if (send->_send_file_partition_counter >= STORE_AND_FORWARD_SIZE) {
    send->_send_file_partition_counter -= STORE_AND_FORWARD_SIZE;
    send->_send_store_file.close();
    // forward data
    send->_send_forward_files.emplace_back(send->_send_store_file_name);
 
    // start a new file
    std::stringstream ss;
    STORE_FILE_NAME();
    send->_send_store_file_name = ss.str();
    send->_send_store_file.close();
    send->_send_store_file.open(ss.str().c_str());

    send->_send_buffer_non_empty_cv.notify_all();
  }
  send->_send_buffer_lock.unlock();

  // ReleaseRecv
  bool delete_notifier = false;
  void* temp_pointer = nullptr;

  CodedBulkOutputNotifier* notifier = output->_notifier;
  output->_notifier = nullptr;
  notifier->_notifier_lock.lock();
  --notifier->_num_not_yet_output;
  delete_notifier = (notifier->_num_not_yet_output == 0);
  notifier->_notifier_lock.unlock();
  if (delete_notifier) {
    temp_pointer = notifier->_task;
    notifier->Delete();
    // this function is always called by codecs
    ReleaseRecv(false, temp_pointer);
  }
  // release CodedBulkOutput
  output->Delete();
}

int
CodedBulkProxy::DoSend (TcpSendPeer* send_peer)
{
  if(send_peer == nullptr) {
    return 0;
  }
  if( !send_peer->m_connected ) {
    return 0;
  }

  CodedBulk_send_info* send = (CodedBulk_send_info*)send_peer->m_send_info;

  int ret;
  if(send->_send_store_packet.GetSize() > 0) {
    // send the packet
    ret = -1;
    if( send_peer->m_socket->CheckPollEvent(POLLOUT) ) {
      ret = send_peer->m_socket->Send(&send->_send_store_packet);
    }
    if ( ret > 0 ) {
      send->_tx_totBytes += send->_send_store_packet.GetSize();
      send->_send_store_packet.SetSize(0);
    } else {
      // send fail
      return 0;
    }
  }

  if(send->_send_output != nullptr) {
    // send the packet
    Packet* packet = send->_send_output->_output_packet;

    ret = -1;
    if( send_peer->m_socket->CheckPollEvent(POLLOUT) ) {
      ret = send_peer->m_socket->Send(packet);
    }

    bool simple_forward = false;
    bool delete_notifier = false;
    void* temp_pointer = nullptr;

    if ( ret > 0 ) {
      send->_tx_totBytes += packet->GetSize(); // packet->GetSize should be equal to ret in this socket interface

      simple_forward = send->_send_output->_simple_forward;
      if(simple_forward) {
        temp_pointer = send->_send_output->_from_recv_peer;
        delete_notifier = true;
      } else {
        // now m_send_buffer[path_id] must be non empty
        CodedBulkOutputNotifier* notifier = send->_send_output->_notifier;
        send->_send_output->_notifier = nullptr;
        notifier->_notifier_lock.lock();
        --notifier->_num_not_yet_output;
        delete_notifier = (notifier->_num_not_yet_output == 0);
        notifier->_notifier_lock.unlock();
        if (delete_notifier) {
          temp_pointer = notifier->_task;
          notifier->Delete();
        }
      }
      if (delete_notifier) {
        ReleaseRecv(simple_forward, temp_pointer);
      }
      // release CodedBulkOutput
      send->_send_output->Delete();
      send->_send_output = nullptr;
    } else {
      // send fail
      return 0;
    }
  }

  // both _send_store_packet and _send_output are empty
  // get the packet and call this function again to send it out
  // for store and forward
  uint8_t output_buffer[DATASEG_SIZE];
  uint32_t packet_size = 0;
  {
    // only grab the send_buffer_lock here for efficiency
    std::unique_lock<std::mutex> non_empty(send->_send_buffer_lock);
    send->_send_buffer_non_empty_cv.wait(non_empty, [send]{
      // conditions
      if(!system_parameters.isRunning()) {
        send->_send_state = send_state__default;
        return true;
      }
      if(!send->_send_forward_files.empty()) {
        send->_send_state = send_state__store_file_non_empty;
        return true;
      }
      if(!send->_send_local_buffer.empty()) {
        send->_send_state = send_state__local_buffer_check;
        return true;
      }
      send->_send_state = send_state__default;
      if(send->_send_buffer.empty())  return false;
      // in-order delivery
      return (send->_serial_number == send->_send_buffer.front()->_serial_number);
    });

    switch (send->_send_state) {
      case send_state__store_file_non_empty:
        {
          std::ifstream& fin = send->_send_forward_files.front().GetFile();
    
          // extract packet
          fin.read((char*)output_buffer, DATASEG_SIZE);
          packet_size = fin.gcount();
    
          if (fin.eof()) {
            send->_send_forward_files.pop_front();
          }

          if ((packet_size == 0) && (send->_send_forward_files.empty())) {
            // nothing to send
            return 0;
          }
        }
        break;
      case send_state__local_buffer_check:
        {
          // local buffer has the next to send
          if(send->_send_local_buffer.front()->_serial_number == send->_serial_number) {
            ++send->_serial_number;
            send->_send_state = send_state__local_buffer_move;
            break;
          }
          if(send->_send_buffer.empty())  return 0;
          if(send->_serial_number == send->_send_buffer.front()->_serial_number) {
            send->_send_state = send_state__default;
          } else {
            break;
          }
        }
      default:
        {
          // otherwise, try to move from the send buffer
          if(!system_parameters.isRunning()) {
            // quit
            return 0;
          }

          // move send buffer to local send buffer
          send->_send_local_buffer.splice(send->_send_local_buffer.begin(), send->_send_buffer);
          ++send->_serial_number;
        }
        break;
    }
  }


    switch (send->_send_state) {
      case send_state__store_file_non_empty:
        {
          // create send store packet
          if(packet_size > 0) {
            send->_send_store_packet.Deserialize(output_buffer,packet_size);
          }
        }
        break;
      case send_state__local_buffer_move:
        {
          // move local buffered output to send_output
          send->_send_output = std::move(send->_send_local_buffer.front());
          send->_send_local_buffer.pop_front();
        }
      case send_state__local_buffer_check:
        break;
      default:
        {
          // sort local buffer
          send->_send_local_buffer.sort(
            [](const CodedBulkOutput* out1, const CodedBulkOutput* out2) {
              return out1->_serial_number < out2->_serial_number;
            }
          );
          // create send output
          send->_send_output = std::move(send->_send_local_buffer.front());
          send->_send_local_buffer.pop_front();
        }
        break;
    }

  // repeat
  return 1;//DoSend(send_peer);
}

int
CodedBulkProxy::DoReceive (TcpReceivePeer* recv_peer)
{
  CodedBulk_recv_info* recv = (CodedBulk_recv_info*)recv_peer->m_recv_info;

  {
  std::unique_lock<std::mutex> has_room(recv->_counter_lock);
  recv->_recv_buffer_room_cv.wait(has_room,
    [this,recv_peer,recv]{
      if(!system_parameters.isRunning()) {
        // quit
        return true;
      }
      if (recv_peer->m_receive_from_network_link) {
        if ( recv->_counter >= __input_buffer_size ) {
          // queue full
          this->m_recv_buffer_full = true;
          return false;
        }
      } else {
        // from host (or interactive traffic), use smaller buffer
        if ( recv->_counter >= DEFAULT_PROXY_INPUT_BUFFER_SIZE ) {
          // queue full
          return false;
        }
      }
      return true;
    }
  );
  }
  if(!system_parameters.isRunning()) {
    // quit
    return 0;
  }

  if( !recv_peer->m_socket->CheckPollEvent(POLLIN) ) {
    return 0;
  }

  if (recv->_recv_packet == nullptr) {
    recv->_recv_packet = recv->_alloc_packet.New();
  }
  int recv_len = recv_peer->m_socket->Recv(recv->_recv_packet, TcpProxy::GetDataSegSize () - recv->_recv_packet->GetSize() );
  if( recv_len < 0 ) {
    // keep reading then...
    //recv->_recv_packet->Delete();
    //recv->_recv_packet = nullptr;
    return 0;
  }

  int ret_size = recv->_recv_packet->GetSize ();
  if (ret_size == 0)
  { //EOF
    // reuse the packet
    recv->_recv_packet->initialize();
    return 0;
  }
  if (ret_size < TcpProxy::GetDataSegSize () )
  {
    // not yet finish receiving
    return ret_size;
  }

  recv->_counter_lock.lock();

  recv->_rx_totBytes += ret_size;
  ++recv->_serial_number;

  // check if we can do simple forward
  if(recv_peer->m_forward) {
    ++recv->_counter;

    CodedBulkOutput* output = recv->_alloc_output.New();
    output->_output_packet  = recv->_recv_packet;
    recv->_recv_packet      = nullptr;
    output->_serial_number  = recv->_serial_number;
    recv->_counter_lock.unlock();

    output->_notifier       = nullptr;
    output->_simple_forward = true;
    output->_from_recv_peer = recv_peer;
    output->_to_send_peer   = recv_peer->m_forward_peer;

    ReceiveOutput(output);

    return ret_size;
  }

  ++recv->_counter;
  recv->_counter_lock.unlock();

  TEST_REGION(
    std::cout << "receive packet ";
    recv->_recv_packet->Print(std::cout);
  );

  CodedBulkInput* input = recv->_alloc_input.New();
  input->_input_packet = recv->_recv_packet;
  recv->_recv_packet = nullptr;
  input->incrementCounter();

  int count = m_codec_manager->receiveInput (
    recv_peer->m_path_id,
    recv->_serial_number,
    input,
    &recv->_alloc_task,
    &recv->_alloc_ready_task
  );

  input->_check_lock.lock();
  input->decrementCounter();
  if(input->hasFinished()) {
    input->_check_lock.unlock();
    // finished before scheduled
    input->Delete();
    recv->_counter_lock.lock();
    --recv->_counter;
    recv->_counter_lock.unlock();
  } else {
    input->_check_lock.unlock();
  }

  if(count == 0) {
    // should not happen
    return 0;
  }

  m_codec_manager->scheduleTasks ();
  return ret_size;
}

void
CodedBulkProxy::ProxySendLogic (TcpSendPeer*    send_peer)
{
  while( CodedBulkProxy::DoSend (send_peer) > 0);
}

void
CodedBulkProxy::ProxyRecvLogic (TcpReceivePeer* recv_peer)
{
  CodedBulkProxy::DoReceive (recv_peer);
}

void
CodedBulkProxy::ListMeasuredBytes (std::ostream& os)
{
  os << "proxy:" << std::endl;
  for(std::unordered_map<uint32_t,CodedBulk_recv_info*>::iterator
    it  = m_recv.begin();
    it != m_recv.end();
    ++it
  ) {
    os << "receives " << it->second->_rx_bytes_interval << " Bps into proxy rx buffer of path " << it->first << std::endl;
  }
  for(std::unordered_map<uint32_t,CodedBulk_send_info*>::iterator
    it  = m_send.begin();
    it != m_send.end();
    ++it
  ) {
    os << "sends " << it->second->_tx_bytes_interval << " Bps out of proxy tx buffer of path " << it->first << std::endl;
  }
}

bool
CodedBulkProxy::IsRecvBufferFull (void) const
{
  return m_recv_buffer_full;
}

void
CodedBulkProxy::InitializeMeasureParameters (void)
{
  for(std::unordered_map<uint32_t,CodedBulk_recv_info*>::iterator
    it  = m_recv.begin();
    it != m_recv.end();
    ++it
  ) {
    it->second->_rx_totBytes       = 0;
    it->second->_rx_prevTotBytes   = 0;
    it->second->_rx_bytes_interval = 0;
  }
  for(std::unordered_map<uint32_t,CodedBulk_send_info*>::iterator
    it  = m_send.begin();
    it != m_send.end();
    ++it
  ) {
    it->second->_tx_totBytes       = 0;
    it->second->_tx_prevTotBytes   = 0;
    it->second->_tx_bytes_interval = 0;
  }
}

void
CodedBulkProxy::Measuring(void)
{
  for(std::unordered_map<uint32_t,CodedBulk_recv_info*>::iterator
    it  = m_recv.begin();
    it != m_recv.end();
    ++it
  ) {
    it->second->_rx_bytes_interval = it->second->_rx_totBytes - it->second->_rx_prevTotBytes;
    it->second->_rx_prevTotBytes   = it->second->_rx_totBytes;
  }
  for(std::unordered_map<uint32_t,CodedBulk_send_info*>::iterator
    it  = m_send.begin();
    it != m_send.end();
    ++it
  ) {
    it->second->_tx_bytes_interval = it->second->_tx_totBytes - it->second->_tx_prevTotBytes;
    it->second->_tx_prevTotBytes   = it->second->_tx_totBytes;
  }
}

void
CodedBulkProxy::WakeAllThreads(void)
{
  for(std::unordered_map<uint32_t,CodedBulk_recv_info*>::iterator
    it  = m_recv.begin();
    it != m_recv.end();
    ++it
  ) {
    it->second->_recv_buffer_room_cv.notify_all();
  }
  for(std::unordered_map<uint32_t,CodedBulk_send_info*>::iterator
    it  = m_send.begin();
    it != m_send.end();
    ++it
  ) {
    it->second->_send_buffer_non_empty_cv.notify_all();
  }
}

void*
CodedBulkProxy::GetSendInfo (uint32_t path_id)
{
  CodedBulk_send_info* send = nullptr;
  m_send_access_lock.lock();
/*
  std::unordered_map<uint32_t,CodedBulk_send_info*>::iterator it = m_send.find(path_id);
  if(it == m_send.end()) {
    send = new CodedBulk_send_info ();
    m_send[path_id] = send;
  } else {
    send = it->second;
  }
*/
  std::pair<std::unordered_map<uint32_t,CodedBulk_send_info*>::iterator, bool> result = m_send.insert(std::pair<uint32_t,CodedBulk_send_info*>(path_id,nullptr));
  if(result.second) {
    send = new CodedBulk_send_info ();
    result.first->second = send;
  } else {
    send = result.first->second;
  }

  m_send_access_lock.unlock();
  return send;
}

void*
CodedBulkProxy::GetRecvInfo (uint32_t path_id, bool use_large_buffer)
{
  CodedBulk_recv_info* recv = nullptr;
  m_recv_access_lock.lock();
/*
  std::unordered_map<uint32_t,CodedBulk_recv_info*>::iterator it = m_recv.find(path_id);
  if(it == m_recv.end()) {
    recv = new CodedBulk_recv_info ();
    m_recv[path_id] = recv;
  } else {
    recv = it->second;
  }
*/
  std::pair<std::unordered_map<uint32_t,CodedBulk_recv_info*>::iterator, bool> result = m_recv.insert(std::pair<uint32_t,CodedBulk_recv_info*>(path_id,nullptr));
  if(result.second) {
    if (use_large_buffer) {
      recv = new CodedBulk_recv_info (__input_buffer_size);
    } else {
      recv = new CodedBulk_recv_info (DEFAULT_PROXY_INPUT_BUFFER_SIZE);
    }
    result.first->second = recv;
  } else {
    recv = result.first->second;
  }

  m_recv_access_lock.unlock();
  return recv;
}

void
CodedBulkProxy::NewSendPeer (uint32_t path_id, void* send_peer)
{
  m_codec_manager->InsertSendPeer(path_id, send_peer);
}

void
CodedBulkProxy::NewRecvPeer (uint32_t path_id, void* recv_peer)
{
  m_codec_manager->InsertRecvPeer(path_id, recv_peer);
}