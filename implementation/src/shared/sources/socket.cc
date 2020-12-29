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
#include "socket.h"
#include <unistd.h>
#include <sstream>

#define CREATE_SOCKET(Class,Type) \
  int socket_fd = 0; \
  socket_fd = socket(AF_INET, Type, 0); \
  if (socket_fd < 0) { \
    return nullptr; \
  } \
  struct timeval tv; \
  tv.tv_sec = 5; \
  tv.tv_usec = 0; \
  if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) { \
    perror("Recv timeout setting error."); \
  } \
  if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv))) { \
    perror("Send timeout setting error."); \
  } \
  Class* socket = new Class (); \
  socket->m_fd = socket_fd; \
  socket->m_sockaddr.in.sin_family = AF_INET; \
  socket->m_sockaddr.in.sin_port   = htons(port); \
  socket->m_sockaddr.in.sin_addr   = address; \
  socket->m_sockaddr_len = sizeof(struct sockaddr_in); \
  return socket;

Socket*
Socket::CreateSocket (const Address& address, const uint16_t port)
{
  CREATE_SOCKET(Socket,SOCK_STREAM)
}

Socket*
Socket::CreateIPCSocket (const IPCAddress& IPC_address, const uint16_t port)
{
  int socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    return nullptr;
  }
  Socket* socket = new Socket ();
  socket->m_fd = socket_fd;
  socket->m_sockaddr.un.sun_family = AF_LOCAL;
  std::stringstream convert;
  convert << "../shared/local_socket/s" << IPC_address << ":" << port;
  strcpy(socket->m_sockaddr.un.sun_path, convert.str().c_str());
  socket->m_sockaddr_len = sizeof(socket->m_sockaddr.un.sun_family) + strlen(socket->m_sockaddr.un.sun_path);

  return socket;
}

Socket::Socket () :
  m_fd (0),
  m_ipTos (0),
  m_revents (0)
{}

Socket*
Socket::Accept (void)
{
  Socket* accepted_socket = new Socket ();
  accepted_socket->m_fd = 
    accept(
      m_fd,
      (struct sockaddr *)&accepted_socket->m_sockaddr,
      &m_sockaddr_len
    );
  if ( accepted_socket->m_fd < 0 ) {
    delete accepted_socket;
    return nullptr;
  } else {
    return accepted_socket;
  }
}

int
Socket::Bind (void)
{
  return bind   (m_fd, (struct sockaddr *)&m_sockaddr, m_sockaddr_len);
}

int
Socket::BindLocal (const Address& local_address, const uint16_t port)
{
  struct sockaddr_in in;
  in.sin_family = AF_INET;
  in.sin_port   = htons(port);
  in.sin_addr   = local_address;
  return bind   (m_fd, (struct sockaddr *)&in, sizeof(struct sockaddr_in));
}

int
Socket::Close (void)
{
  return close (m_fd);
}

int
Socket::Connect (void)
{
  return connect(m_fd, (struct sockaddr *)&m_sockaddr, m_sockaddr_len);
}

int
Socket::Listen (int backlog)
{
  return listen(m_fd, backlog);
}

int
Socket::Poll (struct pollfd* fds, nfds_t nfds)
{
  for(int i = 0; i < nfds; ++i) {
    // poll this socket
    fds[i].fd = m_fd;
  }
  // never timeout
  //return poll(fds, nfds, -1);
  // timeout after 500 ms
  return poll(fds, nfds, 500);
}

int
Socket::PollEvent (short event)
{
  m_fds[0].events = event;
  int ret = Poll(m_fds);
  if ( ret > 0 ) {
    m_revents_lock.lock();
    m_revents = m_fds[0].revents;
    m_revents_lock.unlock();
  }
  return ret;
}

int
Socket::UnlinkIPCSocket (void)
{
  if(m_sockaddr.un.sun_family == AF_LOCAL) {
    unlink(m_sockaddr.un.sun_path);
    return 0;
  } else {
    return -1;
  }
}

int
Socket::Send (Packet* packet)
{
  /*
  int ret = send(m_fd, packet->GetBuf(), packet->GetSize(), 0);
  if (ret > 0) {
    m_revents_lock.lock();
    m_revents = 0;
    m_revents_lock.unlock();
  }
  return ret;
  */
  int pkt_size = packet->GetSize();
  int remaining_len = pkt_size - (size_t) (packet->GetReadHead() - packet->GetBuf());
  if(remaining_len == 0) {
    return pkt_size;
  }
  int send_len = -1;
  try {
    // handle SIGPIPE exception
    send_len = send(m_fd, packet->GetReadHead(), remaining_len, MSG_NOSIGNAL);
  } catch (const std::exception& e) {
    std::cerr << e.what();
    send_len = -1;
  }
  if (send_len < 0) {
    return -1;
  }
  if (send_len > 0) {
    packet->MoveReadHead ((size_t)send_len);
    m_revents_lock.lock();
    m_revents = 0;
    m_revents_lock.unlock();
  }
  if (remaining_len == send_len) {
    return pkt_size;
  }
  return -1;
}

int
Socket::Send (PacketSendAlias* packet_ref)
{
  int pkt_size = packet_ref->_ref_packet->GetSize();
  int remaining_len = pkt_size - (size_t) (packet_ref->GetReadHead() - packet_ref->_ref_packet->GetBuf());
  if(remaining_len == 0) {
    return pkt_size;
  }
  int send_len = -1;
  try {
    // handle SIGPIPE exception
    send_len = send(m_fd, packet_ref->GetReadHead(), remaining_len, MSG_NOSIGNAL);
  } catch (const std::exception& e) {
    std::cerr << e.what();
    send_len = -1;
  }
  m_revents_lock.lock();
  m_revents = 0;
  m_revents_lock.unlock();
  if (send_len < 0) {
    return -1;
  }
  if (send_len > 0) {
    packet_ref->MoveReadHead ((size_t)send_len);
  }
  if (remaining_len == send_len) {
    return pkt_size;
  }
  return -1;
}

int
Socket::Recv (Packet* output_packet, size_t max_len)
{
  int recv_len = recv(m_fd, output_packet->GetWriteHead (), max_len, 0);
  m_revents_lock.lock();
  m_revents = 0;
  m_revents_lock.unlock();
  if ( recv_len < 0 ) {
    return -1;
  }
  if( recv_len > 0 ) {
    output_packet->SetSize ((size_t)recv_len + output_packet->GetSize());
    output_packet->MoveWriteHead ((size_t)recv_len);
  }
  return recv_len;
}

int
Socket::Shutdown (void)
{
  return shutdown(m_fd, SHUT_RDWR);
}

int
Socket::ShutdownRD (void)
{
  return shutdown(m_fd, SHUT_RD);
}

void
Socket::SetIpTos (uint8_t tos)
{
  switch(tos) {
    case 0x20:  m_ipTos = 0x20;  break;
    case 0x40:  m_ipTos = 0x40;  break;
    case 0x60:  m_ipTos = 0x60;  break;
    default:    m_ipTos = 0x00;  break;
  }
  setsockopt(m_fd, IPPROTO_IP, IP_TOS, &m_ipTos, sizeof(m_ipTos));
}

uint8_t
Socket::GetIpTos (void) const
{
  if(m_ipTos < 0) {
    return 0;
  } else {
    return m_ipTos;
  }
}