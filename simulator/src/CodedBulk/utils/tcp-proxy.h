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
#ifndef TCP_PROXY_H
#define TCP_PROXY_H

#include "ns3/application.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/packet.h"

#include <list>
#include <unordered_map>
#include <vector>
#include <set>

#define PROXY_PORT 1000

namespace ns3 {

class Address;
class Socket;
class Packet;

class TcpProxy;

class TcpSendPeer : public SimpleRefCount<TcpSendPeer>
{
public:
  TcpSendPeer (TcpProxy* proxy, uint32_t path_id);
  ~TcpSendPeer ();

  void ConnectPeer (const Ptr<Node> node);
  void DisconnectPeer (void);

  void SetLocal (const Address& local);
  void SetRemote (const Address& peer);
  void SetPriority (const uint8_t& priority);

  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
  void PeerEstablished (Ptr<Socket>, uint32_t);
  void PeerReady (Ptr<Socket>, uint32_t);

  TcpProxy*    m_proxy;
  uint32_t     m_path_id;
  Address      m_local;        //!< Local addresses
  Address      m_peer;         //!< Peer addresses
  uint8_t      m_priority;
  bool         m_assigned_local;
  Ptr<Socket>  m_socket;       //!< Associated socket
  bool         m_connected;    //!< True if connected

  // for derived proxy
  void*      m_send_info;

  std::list<Ptr<Packet> > m_buffer;
};

class TcpReceivePeer : public SimpleRefCount<TcpReceivePeer>
{
public:
  TcpReceivePeer (TcpProxy* proxy, uint32_t path_id);
  ~TcpReceivePeer ();

  void DisconnectPeer (void);

  void HandleRead (Ptr<Socket> socket);

  TcpProxy*    m_proxy;
  uint32_t     m_path_id;
  Ptr<Socket>  m_socket;       //!< receiving socket
  bool         m_receive_from_network_link {true};  // does it receive from a network link?

  // for forwarding
  bool         m_forward;
  TcpSendPeer* m_forward_peer;

  // for derived proxy
  void*      m_recv_info;
};

class TcpListenPeer : public SimpleRefCount<TcpListenPeer>
{
public:
  TcpListenPeer (TcpProxy* proxy);
  TcpListenPeer (TcpProxy* proxy, const Address& local);
  ~TcpListenPeer ();

  void ListenLocal (const Ptr<Node> node);
  void StopListening (void);

  void SetLocal (const Address& local);

  void HandleAccept (Ptr<Socket> socket, const Address& from);
  void HandlePeerClose (Ptr<Socket> socket);
  void HandlePeerError (Ptr<Socket> socket);

  void WaitForFlowIdentifier (Ptr<Socket> socket) const;

  TcpProxy*    m_proxy;
  Address      m_local;        //!< Local addresses
  Ptr<Socket>  m_listening_socket;  //!< Listening socket
};

class TcpProxy : public Application
{
public:
  friend class TcpSendPeer;
  friend class TcpReceivePeer;
  friend class TcpListenPeer;

  TcpProxy ();
  ~TcpProxy ();

  static TypeId GetTypeId (void);
  static void SetDataSegSize (uint32_t size);
  static uint32_t GetDataSegSize (void);
  static void SetMaxRto (Time max_rto) {  __max_rto = max_rto;  }

  //void SetupMatch (uint32_t in_path_id, int out_port);
  void SetForwardRule (uint32_t from_path_id, uint32_t to_path_id);
  void ListForwardRule (std::ostream& os) const;

  void SetBaseAddr (const Ipv4Address base_addr);
  Ipv4Address GetBaseAddr (uint32_t index = 0);

  TcpSendPeer*    AddPeer (uint32_t path_id);
  TcpReceivePeer* AddLocal (uint32_t path_id);

  void RegisterProxyAddr (uint32_t path_id, Ipv4Address addr);
  void RegisterProxyAddr (uint32_t path_id, Ipv4Address src_addr, Ipv4Address dst_addr);
  void RegisterSender   (uint32_t path_id);
  void RegisterReceiver (uint32_t path_id, Ipv4Address addr, uint16_t port);
  void RegisterPriority (uint32_t path_id, uint8_t priority);

  virtual uint64_t GetBufferedBytes (void) const;

protected:
  static uint32_t __data_segment_size;
  static Time     __max_rto;

  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  virtual int DoSend (TcpSendPeer* send_peer);
  virtual int DoReceive (TcpReceivePeer* recv_peer);

  // the core of the proxy
  virtual void ProxySendLogic (TcpSendPeer*    send_peer);
  virtual void ProxyRecvLogic (TcpReceivePeer* recv_peer);

  virtual void* GetSendInfo (uint32_t path_id);
  virtual void* GetRecvInfo (uint32_t path_id);

  virtual void NewSendPeer (uint32_t path_id, void* send_peer);
  virtual void NewRecvPeer (uint32_t path_id, void* recv_peer);

  std::unordered_map<uint32_t, Ipv4Address>   m_proxy_addr_map;
  std::unordered_map<uint32_t, Ipv4Address>   m_proxy_base_addr_map;
  std::unordered_map<uint32_t, Ipv4Address>   m_receiver_addresses;

  std::set<uint32_t>                          m_sender_path_ids;
  std::unordered_map<uint32_t, uint16_t>      m_receiver_port;
  std::unordered_map<uint32_t, uint8_t>       m_priority;

  std::unordered_map<uint32_t, TcpSendPeer* >    m_send_peers;
  std::unordered_map<uint32_t, TcpReceivePeer* > m_receive_peers;

  std::vector<TcpListenPeer> m_listen_peer;

  std::unordered_map<uint32_t, uint32_t>  m_forward_paths;

  std::vector<Ipv4Address> m_base_addr;

  TcpSendPeer*    GetSendPeer    (uint32_t path_id);
  TcpReceivePeer* GetReceivePeer (uint32_t path_id); 
};

}

#endif /* TCP_PROXY_H */