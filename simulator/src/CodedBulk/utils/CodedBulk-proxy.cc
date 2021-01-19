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
#include "ns3/socket.h"
#include "ns3/simulator.h"

#include "CodedBulk-proxy.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CodedBulkProxy");

NS_OBJECT_ENSURE_REGISTERED (CodedBulkProxy);

uint32_t CodedBulkProxy::__input_buffer_size = DEFAULT_PROXY_INPUT_BUFFER_SIZE;
Time     CodedBulkProxy::__resend_timeout    = MilliSeconds(1.0);

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
    delete it->second;
  }
  m_recv.clear();

  for(std::unordered_map<uint32_t,CodedBulk_send_info*>::iterator
    it  = m_send.begin();
    it != m_send.end();
    ++it
  ) {
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
  TcpSendPeer* send_peer = NULL;
  if(output->_to_send_peer != NULL) {
    send_peer = (TcpSendPeer*)output->_to_send_peer;
  } else {
    send_peer = GetSendPeer (output->_path_id);
  }
  CodedBulk_send_info* send = (CodedBulk_send_info*)send_peer->m_send_info;

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
    Simulator::Schedule(Time(0), &CodedBulkProxy::DoSend, this, send_peer);
  }
}

void
CodedBulkProxy::ReleaseRecv (bool simple_forward, void* temp_pointer)
{
  TcpReceivePeer* recv_peer = NULL;
  CodedBulk_recv_info* recv = NULL;
  if(simple_forward) {
    recv_peer = (TcpReceivePeer*)temp_pointer;
    recv = (CodedBulk_recv_info*)recv_peer->m_recv_info;
    --recv->_counter;
    Simulator::Schedule(Time(0), &CodedBulkProxy::DoReceive, this, recv_peer);
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
        --recv->_counter;
        Simulator::Schedule(Time(0), &CodedBulkProxy::DoReceive, this, recv_peer);
      } else {
        received_inputs[i]->_check_lock.unlock();
      }
    }
  }
}

#define STORE_FILE_NAME() \
"tmp_store_and_forward/"; \
GetBaseAddr(0).Print(ss); \
ss << "_" << output->_path_id << "_" << send->_send_store_output_size

//"tmp_store_and_forward/" << GetBaseAddr(0).Get() << "_" << output->_path_id << "_" << send->_send_store_output_size

// for store and forward
void
CodedBulkProxy::StoreOutput(CodedBulkOutput* output)
{
  TcpSendPeer* send_peer = NULL;
  if(output->_to_send_peer != NULL) {
    send_peer = (TcpSendPeer*)output->_to_send_peer;
  } else {
    send_peer = GetSendPeer (output->_path_id);
  }
  CodedBulk_send_info* send = (CodedBulk_send_info*)send_peer->m_send_info;

  uint32_t packet_size = output->_output_packet->GetSize();

  // save the output to file
  if (send->_send_store_file_name.empty()) {
    std::stringstream ss;
    ss << STORE_FILE_NAME();
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
    ss << STORE_FILE_NAME();
    send->_send_store_file_name = ss.str();
    send->_send_store_file.open(ss.str().c_str());

    if (!send->_send_store_file) {
      std::cerr << "Failed to store data in a new chunk: " << strerror(errno) << std::endl;
    }

    // trigger to send
    Simulator::Schedule(Time(0), &CodedBulkProxy::DoSend, this, send_peer);
  }

  // ReleaseRecv
  bool delete_notifier = false;
  void* temp_pointer = NULL;

  CodedBulkOutputNotifier* notifier = output->_notifier;
  output->_notifier = NULL;
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
  NS_LOG_FUNCTION(send_peer->m_path_id);
  if(send_peer == NULL) {
    return 0;
  }
  if( !send_peer->m_connected ) {
    return 0;
  }

  CodedBulk_send_info* send = (CodedBulk_send_info*)send_peer->m_send_info;

  int ret;
  if(send->_send_store_packet.GetSize() > 0) {
    // send the packet
    Ptr<Packet> packet = send->_send_store_packet.Copy();
    ret = send_peer->m_socket->Send(packet);
    if ( ret > 0 ) {
      NS_LOG_DEBUG("send out via " << send_peer->m_path_id << " -- " << send->_serial_number);
      send->_tx_totBytes += send->_send_store_packet.GetSize();
      send->_send_store_packet = Packet();
    } else {
      return 0;
    }
  }

  if(send->_send_output != NULL) {
    // send the packet
    Ptr<Packet> packet = send->_send_output->_output_packet;

    ret = send_peer->m_socket->Send(packet);

    bool simple_forward = false;
    bool delete_notifier = false;
    void* temp_pointer = NULL;

    if ( ret > 0 ) {
      NS_LOG_DEBUG("send out via " << send_peer->m_path_id << " -- " << send->_serial_number);

      send->_tx_totBytes += packet->GetSize(); // packet->GetSize should be equal to ret in this socket interface

      simple_forward = send->_send_output->_simple_forward;
      if(simple_forward) {
        temp_pointer = send->_send_output->_from_recv_peer;
        delete_notifier = true;
      } else {
        // now m_send_buffer[path_id] must be non empty
        CodedBulkOutputNotifier* notifier = send->_send_output->_notifier;
        send->_send_output->_notifier = NULL;
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
      send->_send_output = NULL;
    } else {
      return 0;
    }
  }

  // both _send_store_packet and _send_output are empty
  // get the packet and call this function again to send it out
  // for store and forward
  uint8_t output_buffer[DATASEG_SIZE];
  uint32_t packet_size = 0;

  if(!send->_send_forward_files.empty()) {
    send->_send_state = send_state__store_file_non_empty;
  } else if (!send->_send_local_buffer.empty()) {
    send->_send_state = send_state__local_buffer_check;
    if(send->_send_local_buffer.front()->_serial_number == send->_serial_number) {
      ++send->_serial_number;
      send->_send_state = send_state__local_buffer_move;
    }
  } else {
    send->_send_state = send_state__default;
  }

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

        if(packet_size > 0) {
          send->_send_store_packet = Packet(output_buffer,packet_size);
        }
      }
      break;
    case send_state__local_buffer_move:
      {
        // move local buffered output to send_output
        send->_send_output = std::move(send->_send_local_buffer.front());
        send->_send_local_buffer.pop_front();
      }
      break;
    case send_state__local_buffer_check:
      {
        send->_send_state = send_state__default;
      }
    default:
      {
        if(send->_send_buffer.empty()) {
          return 0;
        }
        if(send->_serial_number != send->_send_buffer.front()->_serial_number) {
          return 0;
        }

        // move send buffer to local send buffer
        send->_send_local_buffer.splice(send->_send_local_buffer.begin(), send->_send_buffer);
        ++send->_serial_number;
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

  Simulator::Schedule(Time(0), &CodedBulkProxy::DoSend, this, send_peer);
  return 0;
  // this may cause the recursive call too large
  //return DoSend(send_peer);
}

int
CodedBulkProxy::DoReceive (TcpReceivePeer* recv_peer)
{
  NS_LOG_FUNCTION(recv_peer->m_path_id);
  CodedBulk_recv_info* recv = (CodedBulk_recv_info*)recv_peer->m_recv_info;

  if ( recv->_counter >= __input_buffer_size ) {
    // queue full
    if (recv_peer->m_receive_from_network_link) {
      m_recv_buffer_full = true;
    }
    return 0;
  }

  Address from;
  recv->_recv_packet = recv_peer->m_socket->RecvFrom (TcpProxy::GetDataSegSize (),0,from);
  if (recv->_recv_packet == NULL) {
    return 0;
  }
  int ret_size = recv->_recv_packet->GetSize ();
  if (ret_size == 0)
  { //EOF
    return 0;
  }
  NS_LOG_DEBUG("receive " << recv_peer->m_path_id << " -- " << recv->_serial_number);

  recv->_rx_totBytes += ret_size;
  ++recv->_serial_number;

  // check if we can do simple forward
  if(recv_peer->m_forward) {
    ++recv->_counter;

    CodedBulkOutput* output = recv->_alloc_output.New();
    output->_output_packet  = recv->_recv_packet;
    recv->_recv_packet      = NULL;
    output->_serial_number  = recv->_serial_number;
    output->_notifier       = NULL;
    output->_simple_forward = true;
    output->_from_recv_peer = recv_peer;
    output->_to_send_peer   = recv_peer->m_forward_peer;

    ReceiveOutput(output);

    return ret_size;
  }

  ++recv->_counter;

  CodedBulkInput* input = recv->_alloc_input.New();
  input->_input_packet = recv->_recv_packet;
  recv->_recv_packet = NULL;
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
    --recv->_counter;
  } else {
    input->_check_lock.unlock();
  }

  if(count == 0) {
    // should not happen
    return 0;
  }// else {
  //  NS_LOG_DEBUG ("packet received on path "<< path_id);
  //}

  m_codec_manager->scheduleTasks ();
  return ret_size;
}

void
CodedBulkProxy::ProxySendLogic (TcpSendPeer*    send_peer)
{
  DoSend (send_peer);
}

void
CodedBulkProxy::ProxyRecvLogic (TcpReceivePeer* recv_peer)
{
  DoReceive (recv_peer);
}

void
CodedBulkProxy::ListMeasuredBytes (std::ostream& os)
{
  os << "proxy at node " << GetNode()->GetId() << ":" << std::endl;
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
  Simulator::Schedule (Seconds(1.0), &CodedBulkProxy::Measuring, this);
}

uint64_t
CodedBulkProxy::GetBufferedBytes (void) const
{
  uint64_t total_bytes = 0;
  // receive buffer packets
  /* we can no longer keep track of the receive buffers
  for(std::unordered_map<uint32_t,CodedBulk_recv_info*>::const_iterator
    it  = m_recv.begin();
    it != m_recv.end();
    ++it
  ) {
    for(std::unordered_map<uint32_t,Ptr<Packet> >::const_iterator
      it_pkt  = it->second->_buffer.begin();
      it_pkt != it->second->_buffer.end();
      ++it_pkt
    ) {
      total_bytes += it_pkt->second->GetSize();
    }
  }
  */
  // send buffer packets
  for(std::unordered_map<uint32_t,CodedBulk_send_info*>::const_iterator
    it  = m_send.begin();
    it != m_send.end();
    ++it
  ) {
    for(std::list<CodedBulkOutput*>::const_iterator
      it_o  = it->second->_send_buffer.begin();
      it_o != it->second->_send_buffer.end();
      ++it_o
    ) {
      total_bytes += (*it_o)->_output_packet->GetSize();
    }
    for(std::list<CodedBulkOutput*>::const_iterator
      it_o  = it->second->_send_local_buffer.begin();
      it_o != it->second->_send_local_buffer.end();
      ++it_o
    ) {
      total_bytes += (*it_o)->_output_packet->GetSize();
    }
    if(it->second->_send_output != NULL) {
      total_bytes += it->second->_send_output->_output_packet->GetSize();
    }
  }
  return total_bytes;
}

void*
CodedBulkProxy::GetSendInfo (uint32_t path_id)
{
  CodedBulk_send_info* send = NULL;
/*
  std::unordered_map<uint32_t,CodedBulk_send_info*>::iterator it = m_send.find(path_id);
  if(it == m_send.end()) {
    send = new CodedBulk_send_info ();
    m_send[path_id] = send;
  } else {
    send = it->second;
  }
*/
  std::pair<std::unordered_map<uint32_t,CodedBulk_send_info*>::iterator, bool> result = m_send.insert(std::pair<uint32_t,CodedBulk_send_info*>(path_id,NULL));
  if(result.second) {
    send = new CodedBulk_send_info ();
    result.first->second = send;
  } else {
    send = result.first->second;
  }

  return send;
}

void*
CodedBulkProxy::GetRecvInfo (uint32_t path_id)
{
  CodedBulk_recv_info* recv = NULL;
/*
  std::unordered_map<uint32_t,CodedBulk_recv_info*>::iterator it = m_recv.find(path_id);
  if(it == m_recv.end()) {
    recv = new CodedBulk_recv_info ();
    m_recv[path_id] = recv;
  } else {
    recv = it->second;
  }
*/
  std::pair<std::unordered_map<uint32_t,CodedBulk_recv_info*>::iterator, bool> result = m_recv.insert(std::pair<uint32_t,CodedBulk_recv_info*>(path_id,NULL));
  if(result.second) {
    recv = new CodedBulk_recv_info ();
    result.first->second = recv;
  } else {
    recv = result.first->second;
  }
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

}