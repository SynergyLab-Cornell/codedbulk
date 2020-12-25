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

#include "CodedBulk-application.h"

#include <vector>
#include <list>
#include <thread>

class CodedBulkMulticastSenderMulticastPath;
class CodedBulkMulticastSender;

class CodedBulkMulticastSenderPath : public SimpleRefCount<CodedBulkMulticastSenderPath>
{
public:
  CodedBulkMulticastSenderPath (Ptr<CodedBulkMulticastSenderMulticastPath> root); 
  CodedBulkMulticastSenderPath (const CodedBulkMulticastSenderPath& path);
  virtual ~CodedBulkMulticastSenderPath ();

  void ConnectPeer (void);
  void ConnectPeerThread (void);
  void DisconnectPeer (void);

  void SetRemote (const Address& peer);
  void SetPathID (const uint32_t path_id);
  uint32_t GetPathID (void) const;

  Socket* GetSocket (void) const;

  int  Send (Packet* packet);
  void Resend (void);

  void PathEstablished (void);
  void PathReady (void);

  Ptr<CodedBulkMulticastSenderMulticastPath> m_root;
  PacketSendAlias  m_current_packet_ref;
  uint32_t         m_path_id;
  Address          m_peer;         //!< Peer addresses
  Socket*          m_socket;       //!< Associated socket
  bool             m_connected;    //!< True if connected
  bool             m_ready;        //!< Ready to send new packet?

  std::mutex       m_connected_lock;
  std::mutex       m_ready_lock;

  std::thread      m_connect_peer_thread;
};

class CodedBulkMulticastSenderMulticastPath : public SimpleRefCount<CodedBulkMulticastSenderMulticastPath>
{
public:
  CodedBulkMulticastSenderMulticastPath(Ptr<CodedBulkMulticastSender> application);
  CodedBulkMulticastSenderMulticastPath(const CodedBulkMulticastSenderMulticastPath& path);

  void ConnectPeers(void);
  void DisconnectPeers(void);

  void AddRemote(const Address& peer, uint32_t path_id);

  Socket* GetSocket (unsigned int index) const;

  int  Send (Packet* packet);

  void DataSent (CodedBulkMulticastSenderPath* path);

  void CheckConnection (void);
  bool CheckReady (void);

  void    SetPriority (uint8_t priority) {  m_priority = priority;  }
  uint8_t GetPriority (void) {  return m_priority;  }

  bool                    m_connected;
  bool                    m_ready;
  uint8_t                 m_priority;
  Ptr<CodedBulkMulticastSender>  m_application;
  std::vector<Ptr<CodedBulkMulticastSenderPath> >    m_paths;       //!< Associated socket

  // for packet generation
  Packet                  m_packet;

  std::mutex              m_connected_lock;
  std::mutex              m_ready_lock;
  std::mutex              m_check_ready_lock;
  std::mutex              m_packet_buf_lock;
};


class CodedBulkMulticastSender : public CodedBulkApplication 
{
public:
  friend class CodedBulkMulticastSenderPath;
  friend class CodedBulkMulticastSenderMulticastPath;
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  CodedBulkMulticastSender (void);

  virtual ~CodedBulkMulticastSender(void);

  void SetRemoteAddresses(const std::list<Address>& addresses);

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
  Socket* GetSocket (unsigned int multicast_path_index, unsigned int path_index) const;

  virtual void StartApplication (void);
  virtual void StopApplication (void);

protected:
  void AllReady (void);

  virtual void PathReady(CodedBulkMulticastSenderMulticastPath* path);
  int SendPacket(CodedBulkMulticastSenderMulticastPath* path, Packet* packet);

  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent

  std::list<Address>  m_remote_addresses;
  std::vector<Ptr<CodedBulkMulticastSenderMulticastPath> > m_multicast_paths;

  std::mutex      m_all_ready_lock;
};

#endif /* CodedBulk_MULTICAST_SENDER_H */
