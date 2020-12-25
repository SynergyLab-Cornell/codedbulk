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
#ifndef SOCKET_H
#define SOCKET_H

#include "simple-ref-count.h"
#include "ptr.h"
#include "address.h"
#include "packet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <stdio.h>
#include <mutex>
#include <atomic>

class Socket : public SimpleRefCount<Socket>
{
public:
  //static Ptr<Socket> CreateSocket (const Address& address, const uint16_t port);
  //static Ptr<Socket> CreateIPCSocket (const IPCAddress& IPC_address, const uint16_t port);
  static Socket* CreateSocket (const Address& address, const uint16_t port);
  static Socket* CreateIPCSocket (const IPCAddress& IPC_address, const uint16_t port);

  Socket ();

  //Ptr<Socket> Accept (void);
  Socket* Accept (void);

  int  Bind (void);
  int  BindLocal (const Address& local_address, const uint16_t port);
  int  Close (void);
  int  Connect (void);
  int  Listen (int backlog = 1);
  int  Poll (struct pollfd* fds, nfds_t nfds = 1);
  int  PollEvent (short event);
  inline bool CheckPollEvent (short event) {
    m_revents_lock.lock();
    bool ret = m_revents & event;
    m_revents_lock.unlock();
    return ret;
  }
  int  UnlinkIPCSocket(void); // should be called by the server when finished

  int  Send (Packet* packet);
  int  Send (PacketSendAlias* packet_ref);
  int  Recv (Packet* output_packet, size_t max_len);  

  int Shutdown (void);
  int ShutdownRD (void);  // this will stop ongoing accept as well

  void SetIpTos (uint8_t ipTos);
  uint8_t GetIpTos (void) const;

  int                m_fd;
  union {
    struct sockaddr_in in; // for AF_INET
    struct sockaddr_un un; // for AF_UNIX
  } m_sockaddr;
  socklen_t m_sockaddr_len;

  int m_ipTos; //!< the socket IPv4 TOS

protected:
  struct pollfd m_fds[1];
  short         m_revents;
  std::mutex    m_revents_lock;
};

#endif /* SOCKET_H */