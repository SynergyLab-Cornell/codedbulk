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
#ifndef CodedBulk_VIRTUAL_LINK_H
#define CodedBulk_VIRTUAL_LINK_H

#include "ns3/simple-ref-count.h"
#include "CodedBulk-code-matrix.h"
#include <vector>
#include <mutex>
#include <unordered_map>
#include <set>

namespace ns3 {

class VirtualLink : public CodeMatrix, public SimpleRefCount<VirtualLink> {
public:
  class VirtualLinkInputPaths{
  public:
    friend VirtualLinkInputPaths& operator << (VirtualLinkInputPaths& map, const uint32_t& path_id);

    VirtualLinkInputPaths() : _path_ids(NULL), _size(0) {}
    ~VirtualLinkInputPaths() {
      if(_path_ids != NULL) {
        delete [] _path_ids;
      }
    }

    int operator[] (uint32_t path_id) const; // get index
    VirtualLinkInputPaths& operator= (const VirtualLinkInputPaths& other);

    inline void SetMaxSize (int max_size) {
      if(_path_ids != NULL) {
        delete [] _path_ids;
      }
      _size = 0;
      if(max_size > 0) {
        _path_ids = new uint32_t[max_size];
      } else {
        _path_ids = NULL;
      }
    }

    //std::unordered_map<uint32_t, int> _path_indices;

    uint32_t* _path_ids;
    int       _size;
  };

  class VirtualLinkOutputPaths{
  public:
    friend VirtualLinkOutputPaths& operator << (VirtualLinkOutputPaths& map, const uint32_t& path_id);

    VirtualLinkOutputPaths() : _path_ids(NULL), _size(0) {}
    ~VirtualLinkOutputPaths() {
      if(_path_ids != NULL) {
        delete [] _path_ids;
      }
    }

    inline void SetMaxSize (int max_size) {
      if(_path_ids != NULL) {
        delete [] _path_ids;
      }
      _size = 0;
      if(max_size > 0) {
        _path_ids = new uint32_t[max_size];
      } else {
        _path_ids = NULL;
      }
    }

    uint32_t operator[] (int index) const;  // get path id
    VirtualLinkOutputPaths& operator= (const VirtualLinkOutputPaths& other);

    uint32_t* _path_ids;
    int       _size;
  };

  VirtualLink ();
  VirtualLink (int row_dimension, int col_dimension);
  VirtualLink (const VirtualLink& map, bool copy_path_ids = true);
  VirtualLink (const CodeMatrix& matrix);
  ~VirtualLink ();

  void listMap (std::ostream& os) const;

  VirtualLink& operator= (const VirtualLink& map);

  void ClearMapPaths (void);

  VirtualLinkInputPaths  _input_paths;
  VirtualLinkOutputPaths _output_paths;

  // for faster access
  void InsertSendPeer(uint32_t path_id, void* send_peer);
  void InsertRecvPeer(uint32_t path_id, void* recv_peer);

  void** _send_peers;
  void** _recv_peers;

  std::mutex _virtual_link_lock;
};

} // namespace ns3

#endif // CodedBulk_VIRTUAL_LINK_H