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
#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include <stdio.h>
#include <atomic>

//4294967295
#define MEMORY_ALLOCATOR_SIZE 1000//1045000

class MemoryElement {
public:
  MemoryElement() {
    _allocated = false;
  }

  virtual void initialize () {};
  virtual void release () {};

  void Delete () {
    release ();
    _allocated = false;
  }

  std::atomic<bool>  _allocated;
};

template <class T>
class MemoryAllocator {
public:
  MemoryAllocator () {
    _size = MEMORY_ALLOCATOR_SIZE;
    _data = new T[_size];
    _head = 0;
  }

  MemoryAllocator (uint32_t size) {
    _size = size;
    _data = new T[_size];
    _head = 0;
  }
  
  ~MemoryAllocator () {
    delete [] _data;
  }

  T* New() {
    uint32_t _prev_head = (_head == 0) ? _size : _head;
    do{
      if(_head == _size) {
        _head = 0;
      }
      if(_data[_head]._allocated) {
        ++_head;
      } else {
        _data[_head]._allocated = true;
        _data[_head].initialize ();
        return &_data[_head++];
      }
    } while (_head != _prev_head);
    return nullptr;
  }

  uint32_t _size;
  uint32_t _head;
  T*       _data;
};

#endif /* MEMORY_ALLOCATOR_H */