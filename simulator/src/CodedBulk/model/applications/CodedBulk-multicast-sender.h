/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Shih-Hao Tseng <st688@cornell.edu>
//

#ifndef CodedBulk_MULTICAST_SENDER_H
#define CodedBulk_MULTICAST_SENDER_H

#include "ns3/address.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable-stream.h"

#include "ns3/CodedBulk-application.h"

#include <vector>
#include <list>

namespace ns3 {

class Address;
//class RandomVariableStream;
class Socket;

class CodedBulkMulticastSenderMulticastPath;
class CodedBulkMulticastSender;

class CodedBulkMulticastSenderPath : public SimpleRefCount<CodedBulkMulticastSenderPath>
{
public:
  static Time __resend_period;

  CodedBulkMulticastSenderPath (Ptr<CodedBulkMulticastSenderMulticastPath> root); 
  CodedBulkMulticastSenderPath (const CodedBulkMulticastSenderPath& path);
  ~CodedBulkMulticastSenderPath ();

  void ConnectPeer (const Ptr<Node> node, const TypeId& tid);
  void DisconnectPeer (void);

  void SetRemote (const Address& peer);
  void SetPathID (const uint32_t path_id);
  uint32_t GetPathID (void) const;

  Ptr<Socket> GetSocket (void) const;
  /**
   * \brief Handle a Connection Succeed event
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Handle a Connection Failed event
   * \param socket the not connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);

  int  Send (Ptr<Packet> packet);
  void Resend (void);

  void PathEstablished (Ptr<Socket>, uint32_t);
  void PathReady (Ptr<Socket>, uint32_t);

  Ptr<CodedBulkMulticastSenderMulticastPath> m_root;
  Ptr<Packet>    m_current_packet;
  uint32_t       m_path_id;
  Address        m_peer;         //!< Peer addresses
  Ptr<Socket>    m_socket;       //!< Associated socket
  bool           m_connected;    //!< True if connected
  bool           m_ready;        //!< Ready to send new packet?

  EventId        m_resend_event;
};

class CodedBulkMulticastSenderMulticastPath : public SimpleRefCount<CodedBulkMulticastSenderMulticastPath>
{
public:
  CodedBulkMulticastSenderMulticastPath(Ptr<CodedBulkMulticastSender> application);
  CodedBulkMulticastSenderMulticastPath(const CodedBulkMulticastSenderMulticastPath& path);

  void ConnectPeers(const Ptr<Node> node, const TypeId& tid);
  void DisconnectPeers(void);

  void AddRemote(const Address& peer, uint32_t path_id);

  Ptr<Socket> GetSocket (unsigned int index) const;

  int  Send (Ptr<Packet> packet);

  void DataSent (Ptr<CodedBulkMulticastSenderPath> path);

  void CheckConnection (void);
  bool CheckReady (void);

  void    SetPriority (uint8_t priority) {  m_priority = priority;  }
  uint8_t GetPriority (void) {  return m_priority;  }

  bool                    m_connected;
  bool                    m_ready;
  uint8_t                 m_priority;
  Ptr<CodedBulkMulticastSender>  m_application;
  std::vector<Ptr<CodedBulkMulticastSenderPath> >    m_paths;       //!< Associated socket
};


class CodedBulkMulticastSender : public CodedBulkApplication 
{
public:
  friend class CodedBulkMulticastSenderMulticastPath;
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  CodedBulkMulticastSender (void);

  virtual ~CodedBulkMulticastSender(void);

  void SetRemoteAddresses(const std::list<Ipv4Address>& addresses);

  Ptr<CodedBulkMulticastSenderMulticastPath> AddMulticastPath (const std::list<uint32_t>& path_ids, uint8_t path_type = 6);
  Ptr<CodedBulkMulticastSenderMulticastPath> GetMulticastPath (unsigned int path_index);

  /**
   * \brief Set the total number of bytes to send.
   *
   * Once these bytes are sent, no packet is sent again, even in on state.
   * The value zero means that there is no limit.
   *
   * \param maxBytes the total number of bytes to send
   */
  void SetMaxBytes (uint64_t maxBytes);

  /**
   * \brief Return a pointer to associated socket.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (unsigned int multicast_path_index, unsigned int path_index) const;

protected:
  //virtual void DoDispose (void);
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  //void CheckMarker (void);
  virtual void AllReady (Ptr<CodedBulkMulticastSenderMulticastPath> path);

  virtual void PathReady(Ptr<CodedBulkMulticastSenderMulticastPath> path);
  int SendPacket(Ptr<CodedBulkMulticastSenderMulticastPath>& path, Ptr<Packet>& packet);

  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
  std::list<Ipv4Address>  m_remote_addresses;
  std::vector<Ptr<CodedBulkMulticastSenderMulticastPath> > m_multicast_paths;

  // make the codedbulk paths send at roughtly the same rate. Helps, but not a determining factor
  // number of multicast paths
  //std::size_t     m_num_coded_multicast_paths {0}; 
  //std::list<Ptr<CodedBulkMulticastSenderMulticastPath> > m_ready_paths;
  
  //static Ptr<UniformRandomVariable> m_unif_rv;
};

} // namespace ns3

#endif /* CodedBulk_MULTICAST_SENDER_H */
