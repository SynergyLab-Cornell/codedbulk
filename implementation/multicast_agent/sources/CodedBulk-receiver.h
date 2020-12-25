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

#ifndef CodedBulk_RECEIVER_H
#define CodedBulk_RECEIVER_H

#include "CodedBulk-application.h"

#include <vector>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>

class CodedBulkReceiver;

class CodedBulkReceiverPath : public SimpleRefCount<CodedBulkReceiverPath>
{
public:
  CodedBulkReceiverPath (CodedBulkReceiver* application, uint32_t path_id);
  virtual ~CodedBulkReceiverPath ();

  CodedBulkReceiver*  m_application;
  uint32_t     m_path_id;
  Socket*      m_socket;
};

class CodedBulkReceiverListener : public SimpleRefCount<CodedBulkReceiverListener>
{
public:
  CodedBulkReceiverListener (CodedBulkReceiver* application);
  virtual ~CodedBulkReceiverListener ();

  void ListenLocal (void);
  void ListenLocalThread (void);
  void StopListening (void);

  void SetLocal (const Address& local);

  Socket*  GetListeningSocket (void) const;

  void HandleAccept (Socket* socket);
  void WaitForFlowIdentifier (Socket* socket);

  CodedBulkReceiver*  m_application {nullptr};
  Address      m_local;             //!< Peer addresses
  Socket*      m_listening_socket {nullptr};  //!< Listening socket

  std::thread  m_listening_thread;

  std::mutex m_waiting_for_flow_identifier_lock;
  std::unordered_map<Socket*, std::thread> m_waiting_for_flow_identifier_threads;
};

class CodedBulkReceiver : public CodedBulkApplication
{
public:
  friend class CodedBulkReceiverPath;
  friend class CodedBulkReceiverListener;
  CodedBulkReceiver ();

  virtual ~CodedBulkReceiver ();

  void SetLocal(const Address& local);
  Ptr<CodedBulkReceiverPath> AddPath (const uint32_t path_id);

  Socket* GetListeningSocket (unsigned int path_index) const;

  virtual void StartApplication (void);
  virtual void StopApplication (void);

protected:
  /**
   * \brief Handle a packet received by the application
   * \param socket the receiving socket
   */
  void HandleRead (Socket* socket);

  // In the case of TCP, each socket accept returns a new socket, so the 
  // listening socket is stored separately from the accepted sockets

  Address     m_local; //!< Local Address to bind to

  CodedBulkReceiverListener m_listener;
  std::vector<Ptr<CodedBulkReceiverPath> > m_paths;       //!< Associated socket
  std::mutex m_paths_lock;
};

#endif /* CodedBulk_RECEIVER_H */

