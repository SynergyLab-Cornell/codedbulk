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
#include "CodedBulk-virtual-link.h"
#include "system_parameters.h"

VirtualLink::VirtualLinkInputPaths&
operator << (VirtualLink::VirtualLinkInputPaths& map, const uint32_t& path_id)
{
  map._path_ids[map._size++] = path_id;
  return map;
}

int
VirtualLink::VirtualLinkInputPaths::operator[] (uint32_t path_id) const
{
  // it would be fastere to do linear search when the array is small
  for (int i = 0; i < _size; ++i) {
    if(_path_ids[i] == path_id) {
      return i;
    }
  }
  return -1;
}

VirtualLink::VirtualLinkInputPaths&
VirtualLink::VirtualLinkInputPaths::operator= (const VirtualLink::VirtualLinkInputPaths& other)
{
  SetMaxSize(MAX_CODEC_INPUT_SIZE);
  for(int i = 0; i < other._size; ++i) {
    _path_ids[i] = other._path_ids[i];
  }
  return *this;
}

VirtualLink::VirtualLinkOutputPaths&
operator << (VirtualLink::VirtualLinkOutputPaths& map, const uint32_t& path_id)
{
  map._path_ids[map._size++] = path_id;
  return map;
}

uint32_t
VirtualLink::VirtualLinkOutputPaths::operator[] (int index) const
{
  if ( index < 0 ) {
    return 0;
  }
  if ( index >= _size ) {
    return 0;
  }
  return _path_ids[index];
}

VirtualLink::VirtualLinkOutputPaths&
VirtualLink::VirtualLinkOutputPaths::operator= (const VirtualLink::VirtualLinkOutputPaths& other)
{
  SetMaxSize(MAX_CODEC_INPUT_SIZE);
  for(int i = 0; i < other._size; ++i) {
    _path_ids[i] = other._path_ids[i];
  }
  return *this;
}

VirtualLink::VirtualLink ()
{
  VirtualLink(0,0);
}

VirtualLink::VirtualLink (int row_dimension, int col_dimension) : 
  CodeMatrix(row_dimension, col_dimension)
{
  _send_peers = nullptr;
  _recv_peers = nullptr;
  ClearMapPaths();
}

VirtualLink::VirtualLink (const VirtualLink& map, bool copy_path_ids)
{
  _send_peers = nullptr;
  _recv_peers = nullptr;
  _row_dimension = map._row_dimension;
  _col_dimension = map._col_dimension;
  _rows = new CodeVector[_row_dimension];
  for(int i = 0; i < _row_dimension; ++i) {
    _rows[i] = map._rows[i];
  }
  if( copy_path_ids ) {
    _input_paths    = map._input_paths   ;
    _output_paths   = map._output_paths  ;
  } else {
    ClearMapPaths();
  }
}

VirtualLink::VirtualLink (const CodeMatrix& matrix)
{
  _send_peers = nullptr;
  _recv_peers = nullptr;
  _row_dimension = matrix._row_dimension;
  _col_dimension = matrix._col_dimension;
  _rows = new CodeVector[_row_dimension];
  for(int i = 0; i < _row_dimension; ++i) {
    _rows[i] = matrix._rows[i];
  }
  ClearMapPaths();
}

VirtualLink::~VirtualLink ()
{
  if(_send_peers != nullptr) {
    delete [] _send_peers;
    _send_peers = nullptr;
  }
  if(_recv_peers != nullptr) {
    delete [] _recv_peers;
    _recv_peers = nullptr;
  }
}

void
VirtualLink::listMap (std::ostream& os) const
{
  os << "map: " << std::dec;
  for(int i = 0; i < _input_paths._size; ++i) {
    os << _input_paths[i] << " ";    
  }
  os << "-> ";
  for(int i = 0; i < _output_paths._size; ++i) {
    os << _output_paths[i] << " ";
  }
  os << std::endl;
  listMatrix(os);
  os << std::endl;
}

VirtualLink&
VirtualLink::operator= (const VirtualLink& map)
{
  forceResize(map._row_dimension, map._col_dimension);
  for(int i = 0; i < _row_dimension; ++i) {
    _rows[i] = map._rows[i];
  }
  _input_paths    = map._input_paths   ;
  _output_paths   = map._output_paths  ;
  return *this;
}

void
VirtualLink::ClearMapPaths (void)
{
  //_input_paths .SetMaxSize(_col_dimension);
  //_output_paths.SetMaxSize(_row_dimension);
  _input_paths .SetMaxSize(MAX_CODEC_INPUT_SIZE);
  _output_paths.SetMaxSize(MAX_CODEC_INPUT_SIZE);
}

// for faster access
void
VirtualLink::InsertSendPeer(uint32_t path_id, void* send_peer)
{
  _virtual_link_lock.lock();
  if(_send_peers == nullptr) {
    void** tmp = new void*[_row_dimension];
    for(int i = 0; i < _row_dimension; ++i) {
      tmp[i] = nullptr;
    }
    _send_peers = tmp;
  }
  for(int i = 0; i < _row_dimension; ++i) {
    if(_output_paths[i] == path_id) {
      _send_peers[i] = send_peer;
      break;
    }
  }
  _virtual_link_lock.unlock();
}

void
VirtualLink::InsertRecvPeer(uint32_t path_id, void* recv_peer)
{
  _virtual_link_lock.lock();
  if(_recv_peers == nullptr) {
    void** tmp = new void*[_col_dimension];
    for(int i = 0; i < _col_dimension; ++i) {
      tmp[i] = nullptr;
    }
    _recv_peers = tmp;
  }
  int index = _input_paths[path_id];
  if(index != -1) {
    _recv_peers[index] = recv_peer;
  }
  _virtual_link_lock.unlock();
}