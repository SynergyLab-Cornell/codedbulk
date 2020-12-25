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
#include "packet.h"

Packet::Packet (uint8_t* buf, size_t len)
{
  Deserialize (buf, len);
}

void
Packet::AddHeader (CodedBulkFlowIdentifier& header)
{
  // suppose the data is filled after _buf[3]
  for (int i = 0; i < 4; ++i){
    _buf[i] = header.m_bytes[i];
  }
  _size += 4;
  _write_head += 4;
}

uint32_t
Packet::RemoveHeader (CodedBulkFlowIdentifier& header)
{
  if(_size < 4) {
    return 0;
  }
  for (int i = 0; i < 4; ++i){
    header.m_bytes[i] = _buf[i];
    _buf[i] = 0;
  }
  _size -= 4;
  _write_head -= 4;
  return 4;
}

uint32_t
Packet::Deserialize (const uint8_t* buffer, size_t size)
{
  for(size_t pos = 0; pos < size; ++pos) {
    _buf[pos] = buffer[pos];
  }
  _size = size;
  _write_head = _buf + size;
  _read_head = _buf;

  return size;
}

void
Packet::CopyData (std::ostream *os, uint32_t size) const
{
  if (size > 0) {
    os->write((char*)_buf,size);
  }
}

void
Packet::Print (std::ostream& os)
{
  for (size_t i = 0; i < _size; ++i) {
    os << _buf[i];
  }
}