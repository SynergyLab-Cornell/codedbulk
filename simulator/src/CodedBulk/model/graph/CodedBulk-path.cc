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
#include "CodedBulk-path.h"

namespace ns3 {

CodedBulkUnicastPath&
operator << (CodedBulkUnicastPath& path, const int& node)
{
  path._nodes.push_back(node);
  return path;
}

CodedBulkUnicastPath::CodedBulkUnicastPath () {
  _rate   = 0.0;
  _path_id = 0;
  _nodes.clear();
}

CodedBulkUnicastPath::CodedBulkUnicastPath (uint32_t path_id)
{
  _rate   = 0.0;
  _path_id = path_id;
  _nodes.clear();
}

CodedBulkUnicastPath::CodedBulkUnicastPath (const CodedBulkUnicastPath& path)
{
  _rate   = path._rate;
  _path_id = path._path_id;
  _nodes  = path._nodes;
}

CodedBulkUnicastPath::~CodedBulkUnicastPath()
{
  _rate = 0.0;
}

void
CodedBulkUnicastPath::ListPath (std::ostream& os) {
  for(std::list<int>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
    os << *it << " ";
  }
  os << std::endl;
}

CodedBulkUnicastPath
CodedBulkUnicastPath::GetReversedPath (uint32_t new_path_id)
{
  CodedBulkUnicastPath p(new_path_id);
  p._rate  = _rate;
  p._nodes = _nodes;
  p._nodes.reverse();
  return p;
}

CodedBulkMultipathSet::CodedBulkMultipathSet() {
  _paths.clear();
  _backward_path = NULL;
}

CodedBulkMultipathSet::~CodedBulkMultipathSet() {
  _paths.clear();
  _backward_path = NULL;
}

void
CodedBulkMultipathSet::addCodedBulkUnicastPath(Ptr<CodedBulkUnicastPath> path) {
  _paths.push_back(path);
}

void
CodedBulkMultipathSet::setBackwardPath (Ptr<CodedBulkUnicastPath> backward_path) {
  _backward_path = backward_path;
}

void
CodedBulkMultipathSet::listPath (std::ostream& os) {
    os << std::dec;
    for(std::vector<Ptr<CodedBulkUnicastPath> >::iterator it = _paths.begin(); it != _paths.end(); ++it) {
        os << "path id: " << (*it)->_path_id <<  ", rate: " << (*it)->_rate << ", path: ";
        (*it)->ListPath(os);
    }
    if( _backward_path != NULL ){
      os << "back rate: " << _backward_path->_rate << ", path: ";
      _backward_path->ListPath(os);
    }
}

void
CodedBulkMultipathSet::clearAllPaths (void) {
  _paths.clear();
}

}