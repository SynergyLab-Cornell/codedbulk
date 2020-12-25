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

#include "simple-ref-count.h"
#include "ptr.h"
#include "address.h"
#include "packet.h"
#include "socket.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#include <thread>
#include <list>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <set>

class TcpProxy;

class TcpSendPeer : public SimpleRefCount<TcpSendPeer>
{
public:
  TcpSendPeer (TcpProxy* proxy, uint32_t path_id, bool address_is_ipc = false);
  virtual ~TcpSendPeer ();

  void ConnectPeer (void);
  void ConnectPeerThread (void);
  void DisconnectPeer (void);

  void SetLocal (const Address& local);
  void SetRemote (const Address& peer);
  void SetPriority (const uint8_t& priority);

  void PeerEstablished (void);

  void PeerReady (void);

  TcpProxy*  m_proxy;
  uint32_t   m_path_id;
  Address    m_local;        //!< Local addresses
  Address    m_peer;         //!< Peer addresses
  uint8_t    m_priority;
  bool       m_assigned_local;
  bool       m_address_is_ipc;     // is the address local?
  Socket*    m_socket;       //!< Associated socket
  bool       m_connected;    //!< True if connected
 
  // for derived proxy
  void*      m_send_info;

  std::list<Packet*>  m_buffer;
  std::mutex m_send_peer_lock;
  std::mutex m_buffer_lock;

  std::thread m_connect_peer_thread;
};

class TcpReceivePeer : public SimpleRefCount<TcpReceivePeer>
{
public:
  TcpReceivePeer (TcpProxy* proxy, uint32_t path_id);
  virtual ~TcpReceivePeer ();

  void DisconnectPeer (void);

  void HandleRead (void);

  TcpProxy*  m_proxy;
  uint32_t   m_path_id;
  Socket*    m_socket;       //!< receiving socket
  bool       m_receive_from_network_link {true};  // does it receive from a network link?

  // for forwarding
  bool         m_forward;
  TcpSendPeer* m_forward_peer;

  // for derived proxy
  void*      m_recv_info;
};

class TcpListenPeer : public SimpleRefCount<TcpListenPeer>
{
public:
  TcpListenPeer (TcpProxy* proxy, bool address_is_ipc = false);
  TcpListenPeer (TcpProxy* proxy, const Address& local, bool address_is_ipc = false);
  // copy constructor for emplace 
  TcpListenPeer (const TcpListenPeer& copy_peer);

  virtual ~TcpListenPeer ();

  void ListenLocal (void);
  void StopListening (void);

  void SetLocal (const Address& local);

  void HandleAccept (Socket* socket);

  void WaitForFlowIdentifier (Socket* socket);

  TcpProxy*  m_proxy;
  Address    m_local;        //!< Local addresses
  Socket*    m_listening_socket;  //!< Listening socket
  bool       m_address_is_ipc;     // is the address local?

  std::mutex m_waiting_for_flow_identifier_lock;
  std::unordered_map<Socket*, std::thread> m_waiting_for_flow_identifier_threads;
};

class TcpProxy : public SimpleRefCount<TcpProxy>
{
public:
  friend class TcpSendPeer;
  friend class TcpReceivePeer;
  friend class TcpListenPeer;

  TcpProxy ();
  ~TcpProxy ();

  static void SetDataSegSize (size_t size);
  static size_t GetDataSegSize (void);

  void SetForwardRule (uint32_t from_path_id, uint32_t to_path_id);

  void SetBaseAddr (const Address base_addr);
  Address GetBaseAddr (uint32_t index = 0);

  TcpSendPeer*    AddPeer (uint32_t path_id);
  TcpReceivePeer* AddLocal (uint32_t path_id);

  void RegisterProxyAddr (uint32_t path_id, Address addr);
  void RegisterProxyAddr (uint32_t path_id, Address src_addr, Address dst_addr);
  void RegisterSender   (uint32_t path_id);
  void RegisterReceiver (uint32_t path_id, Address addr, uint16_t port);
  void RegisterPriority (uint32_t path_id, uint8_t priority);

  void StartProxy (void);
  void StopProxy (void);

protected:
  static size_t __data_segment_size;

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  virtual int DoSend (TcpSendPeer* send_peer);
  virtual int DoReceive (TcpReceivePeer* recv_peer);

  // the core of the proxy
  virtual void ProxySendLogic (TcpSendPeer*    send_peer);
  virtual void ProxyRecvLogic (TcpReceivePeer* recv_peer);

  virtual void* GetSendInfo (uint32_t path_id);
  virtual void* GetRecvInfo (uint32_t path_id, bool use_large_buffer = false);

  virtual void NewSendPeer (uint32_t path_id, void* send_peer);
  virtual void NewRecvPeer (uint32_t path_id, void* recv_peer);

  std::unordered_map<uint32_t, Address>   m_proxy_addr_map;
  std::unordered_map<uint32_t, Address>   m_proxy_base_addr_map;
  std::unordered_map<uint32_t, Address>   m_receiver_addresses;

  std::set<uint32_t>                      m_sender_path_ids;
  std::unordered_map<uint32_t, uint16_t>  m_receiver_port;
  std::unordered_map<uint32_t, uint8_t>   m_priority;

  std::unordered_map<uint32_t, TcpSendPeer*>    m_send_peers;
  std::unordered_map<uint32_t, TcpReceivePeer*> m_receive_peers;

  std::vector<TcpListenPeer> m_listen_peer;
  std::vector<TcpListenPeer> m_local_listen_peer;

  std::vector<std::thread>   m_listen_peer_thread;
  std::vector<std::thread>   m_local_listen_peer_thread;

  std::unordered_map<uint32_t, uint32_t>  m_forward_paths;

  std::vector<Address> m_base_addr;
  
  std::mutex  m_socket_creation;

  std::mutex  m_send_peer_lock;
  std::mutex  m_receive_peer_lock;

  TcpSendPeer*    GetSendPeer    (uint32_t path_id);
  TcpReceivePeer* GetReceivePeer (uint32_t path_id);
};

#endif /* TCP_PROXY_H */