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
#ifndef PACKET_H
#define PACKET_H
#include "memory-allocator.h"
#include "system_parameters.h"

#include "simple-ref-count.h"
#include "ptr.h"
#include "CodedBulk-flow-identifier.h"
#include <sys/types.h>
#include <iostream>
#include <mutex>

class PacketSendAlias;

class Packet : public SimpleRefCount<Packet>, public MemoryElement
{
public:
  friend class PacketSendAlias;
  inline static size_t GetMaxLen() {  return DATASEG_SIZE;  }

  Packet () {  initialize ();  }
  Packet (uint8_t* buf, size_t len);

  virtual void initialize () {
    _size = 0;
    _write_head = _buf;
    _read_head = _buf;
  }

  inline uint8_t* GetBuf (void) {  return _buf;  }
  inline uint8_t* GetWriteHead (void) {  return _write_head;  }
  inline void     MoveWriteHead (size_t offset) {  _write_head += offset;  }
  inline uint8_t* GetReadHead (void) {  return _read_head;  }
  inline void     MoveReadHead (size_t offset) {  _read_head += offset;  }
  inline size_t   GetSize (void) {  return _size;  }
  inline void     SetSize (size_t size) {  _size = size;  }

  void     AddHeader (CodedBulkFlowIdentifier& header);
  uint32_t RemoveHeader (CodedBulkFlowIdentifier& header);

  uint32_t Deserialize (const uint8_t* buffer, size_t size);

  void     CopyData (std::ostream *os, uint32_t size) const;

  void     Print (std::ostream& os);

private:
  uint8_t  _buf[DATASEG_SIZE];
  size_t   _size;
  uint8_t* _write_head;
  uint8_t* _read_head;
};

class PacketSendAlias {
public:
  PacketSendAlias () : _ref_packet(nullptr), _read_head(nullptr) {}
  inline void ReferToPacket (Packet* packet) {
    _ref_packet = packet;
    _read_head  = _ref_packet->_buf;
  }

  inline uint8_t* GetReadHead (void) {  return _read_head;  }
  inline void     MoveReadHead (size_t offset) {  _read_head += offset;  }

  inline void Dereference () {  _ref_packet = nullptr;  }

  Packet*     _ref_packet;
  uint8_t*    _read_head;
  std::mutex  _set_lock;
};

#endif /* PACKET_H */