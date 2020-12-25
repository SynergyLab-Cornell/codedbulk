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
#include "CodedBulk-flow-identifier.h"
#include "system_parameters.h"
#include "CodedBulk-receiver.h"

extern SystemParameters system_parameters;

CodedBulkReceiverPath::CodedBulkReceiverPath (CodedBulkReceiver* application, uint32_t path_id) :
  m_application(application),
  m_path_id(path_id)
{
}

CodedBulkReceiverPath::~CodedBulkReceiverPath ()
{
  if(m_socket != nullptr) {
    delete m_socket;
    m_socket = nullptr;
  }
}

CodedBulkReceiverListener::CodedBulkReceiverListener (CodedBulkReceiver* application) :
  m_application(application)
{
  m_waiting_for_flow_identifier_threads.clear();
}

CodedBulkReceiverListener::~CodedBulkReceiverListener ()
{
  if (m_listening_thread.joinable())
    m_listening_thread.join();
  StopListening ();
  if(m_listening_socket != nullptr) {
    if(m_application->GetThroughProxy()) {
      m_listening_socket->UnlinkIPCSocket();
    }
    delete m_listening_socket;
    m_listening_socket = nullptr;
  }

  m_waiting_for_flow_identifier_lock.lock();
  for(auto& item : m_waiting_for_flow_identifier_threads) {
    if (item.second.joinable())
      item.second.join();
  }
  m_waiting_for_flow_identifier_threads.clear();
  m_waiting_for_flow_identifier_lock.unlock();
}

void
CodedBulkReceiverListener::ListenLocal (void) {
  m_listening_thread = std::thread(&CodedBulkReceiverListener::ListenLocalThread, this);
}

void
CodedBulkReceiverListener::ListenLocalThread (void)
{
  if (m_listening_socket == nullptr)
    {
      if(m_application->GetThroughProxy()) {
        m_listening_socket =
          Socket::CreateIPCSocket (
            AddressToIPCAddress(m_local),
            m_application->GetApplicationID ()
          );
      } else {
        while ( system_parameters.isRunning() && (m_listening_socket == nullptr) ) {
          m_listening_socket =
            Socket::CreateSocket (
              m_local,
              INTERACTIVE_PORT+m_application->GetApplicationID ()
            );
        }
      }
      m_listening_socket->SetIpTos (6 << 4);
      if (m_listening_socket->Bind () < 0)
        {
          perror ("Failed to bind socket");
          return;
        }
      if (m_listening_socket->Listen ())
        {
          perror ("Failed to listen");
          return;
        }
    }
  system_parameters.registerListeningSocket(m_listening_socket);
  Socket* accepted_socket = nullptr;
  while(system_parameters.isRunning()){
    accepted_socket = m_listening_socket->Accept ();
    if (accepted_socket != nullptr)
      {
        HandleAccept (accepted_socket);
      }
    }
}

void
CodedBulkReceiverListener::StopListening (void)
{
  if (m_listening_socket != nullptr) 
    {
      m_listening_socket->Close ();
    }
}

void
CodedBulkReceiverListener::SetLocal (const Address& local)
{
  m_local = local;
}

Socket*
CodedBulkReceiverListener::GetListeningSocket (void) const
{
  return m_listening_socket;
}

void
CodedBulkReceiverListener::HandleAccept (Socket* socket)
{
  // let ack have high priority
  socket->SetIpTos (6 << 4);
  m_waiting_for_flow_identifier_lock.lock();
  m_waiting_for_flow_identifier_threads[socket] = std::thread (&CodedBulkReceiverListener::WaitForFlowIdentifier, this, socket);
  m_waiting_for_flow_identifier_lock.unlock();
}

void
CodedBulkReceiverListener::WaitForFlowIdentifier (Socket* socket)
{
  // receive 4 bytes
  Packet packet;
  int recv_len = 0;
  bool flow_identifier_received = false;
  while (system_parameters.isRunning() && !flow_identifier_received) {
    recv_len = socket->Recv(&packet, 4);
    if(recv_len >= 4) {
      flow_identifier_received = true;
    }
  }
  CodedBulkFlowIdentifier mpls_flow_identifier;
  packet.RemoveHeader(mpls_flow_identifier);
  Ptr<CodedBulkReceiverPath> path = m_application->AddPath(mpls_flow_identifier.GetValue());
  path->m_socket = socket;

  //m_waiting_for_flow_identifier_lock.lock();
  //m_waiting_for_flow_identifier_threads.erase(socket);
  //m_waiting_for_flow_identifier_lock.unlock();

  m_application->HandleRead(path->m_socket);
}

CodedBulkReceiver::CodedBulkReceiver () :
  m_listener(this)
{
  m_paths.clear();
}

CodedBulkReceiver::~CodedBulkReceiver()
{}

void
CodedBulkReceiver::SetLocal(const Address& local)
{
  m_local = local;
  m_listener.SetLocal(m_local);
}

Ptr<CodedBulkReceiverPath>
CodedBulkReceiver::AddPath (const uint32_t path_id)
{
  m_paths_lock.lock();
  Ptr<CodedBulkReceiverPath> path = Create<CodedBulkReceiverPath> (this, path_id);
  m_paths.push_back(path);
  m_paths_lock.unlock();
  return path;
}

void
CodedBulkReceiver::HandleRead (Socket* socket)
{
  Packet packet;
  while(system_parameters.isRunning()){
    if (socket->CheckPollEvent(POLLIN)) {
      // rx buffer ready
      //int recv_len = socket->Recv(&packet, 1004);
      int recv_len = socket->Recv(&packet, DATASEG_SIZE - packet.GetSize());
      if( recv_len < 0 ) {
        // read fail
        continue;
      }
      int pkt_size = packet.GetSize ();
      if (pkt_size == 0) {
        //EOF
        packet.initialize();
        continue;
      }
      if (pkt_size < DATASEG_SIZE)
      {
        // not yet finish receiving
        continue;
      }

      TEST_REGION(
        packet->Print(std::cout);
      );
      m_totBytes_lock.lock();
      m_totBytes += pkt_size;
      m_totBytes_lock.unlock();
      packet.initialize();
    } else {
      socket->PollEvent(POLLIN);
    }
  }
}

void
CodedBulkReceiver::StartApplication (void)
{
  m_listener.ListenLocal();
}

void
CodedBulkReceiver::StopApplication (void)
{
  m_listener.StopListening();
  for(std::vector<Ptr<CodedBulkReceiverPath> >::iterator
    it  = m_paths.begin();
    it != m_paths.end();
    ++it)
  {
    (*it)->m_socket->Close ();
  }
}

