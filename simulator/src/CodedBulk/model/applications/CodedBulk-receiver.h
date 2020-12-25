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

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"

#include "ns3/CodedBulk-application.h"
#include <vector>

namespace ns3 {

class Address;
class Socket;
class Packet;

class CodedBulkReceiver;

class CodedBulkReceiverPath : public SimpleRefCount<CodedBulkReceiverPath>
{
public:
  CodedBulkReceiverPath (CodedBulkReceiver* application, uint32_t path_id);
  virtual ~CodedBulkReceiverPath ();

  CodedBulkReceiver*  m_application;
  uint32_t     m_path_id;
  Ptr<Socket>  m_socket;
};

class CodedBulkReceiverListener : public SimpleRefCount<CodedBulkReceiverListener>
{
public:
  CodedBulkReceiverListener (CodedBulkReceiver* application);
  virtual ~CodedBulkReceiverListener ();

  void ListenLocal (const Ptr<Node> node, const TypeId& tid);
  void StopListening (void);

  void SetLocal (const Address& local);

  Ptr<Socket> GetListeningSocket (void) const;
  /**
   * \brief Handle an incoming connection
   * \param socket the incoming connection socket
   * \param from the address the connection is from
   */
  void HandleAccept (Ptr<Socket> socket, const Address& from);
  void WaitForFlowIdentifier (Ptr<Socket> socket) const;
  /**
   * \brief Handle an connection close
   * \param socket the connected socket
   */
  void HandlePeerClose (Ptr<Socket> socket);
  /**
   * \brief Handle an connection error
   * \param socket the connected socket
   */
  void HandlePeerError (Ptr<Socket> socket);

  CodedBulkReceiver*     m_application;
  Address         m_local;                       //!< Peer addresses
  Ptr<Socket>     m_listening_socket {nullptr};  //!< Listening socket
};

class CodedBulkReceiver : public CodedBulkApplication 
{
public:
  friend class CodedBulkReceiverListener;
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  CodedBulkReceiver ();

  virtual ~CodedBulkReceiver ();

  void SetLocal(const Ipv4Address& local);
  Ptr<CodedBulkReceiverPath> AddPath (const uint32_t path_id);

  Ptr<Socket> GetListeningSocket (void);

protected:
  //virtual void DoDispose (void);
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  /**
   * \brief Handle a packet received by the application
   * \param socket the receiving socket
   */
  void HandleRead (Ptr<Socket> socket);

  // In the case of TCP, each socket accept returns a new socket, so the 
  // listening socket is stored separately from the accepted sockets

  Ipv4Address m_local; //!< Local address to bind to

  CodedBulkReceiverListener m_listener;
  std::vector<Ptr<CodedBulkReceiverPath> > m_paths;       //!< Associated socket
};

} // namespace ns3

#endif /* CodedBulk_RECEIVER_H */

