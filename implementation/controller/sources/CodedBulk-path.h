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
#ifndef CodedBulk_PATH_H
#define CodedBulk_PATH_H

#include "ptr.h"
#include "simple-ref-count.h"
#include "CodedBulk-virtual-link.h"
#include <ostream>
#include <vector>
#include <list>

#ifndef ANY_PATH_ID
#define ANY_PATH_ID 0xffffffff
#endif

class CodedBulkUnicastPath : public SimpleRefCount<CodedBulkUnicastPath> {
public:
    friend CodedBulkUnicastPath& operator << (CodedBulkUnicastPath& path, const int& node);
    static uint32_t GetAnyPathID() {  return ANY_PATH_ID;  }

    CodedBulkUnicastPath ();
    CodedBulkUnicastPath (uint32_t path_id);
    CodedBulkUnicastPath (const CodedBulkUnicastPath& path);
    virtual ~CodedBulkUnicastPath();

    virtual void ListPath (std::ostream& os);
    CodedBulkUnicastPath GetReversedPath (uint32_t new_path_id);

    double         _rate;
    uint32_t       _path_id;   // the path identifier
    uint16_t       _application_port;

    // the packet that carries the current path information
    uint32_t       _carrying_path_id;
    // for code generator
    uint32_t       _path_code_id;

    std::list<int> _nodes;

    CodeVector* _path_code;
    CodeVector* _orthogonal_tester; // a_t
    CodeMatrix* _orthogonal_matrix;

    std::list<int>::iterator _current_node; // for computation
};

class CodedBulkMultipathSet : public SimpleRefCount<CodedBulkMultipathSet> {
public:
    CodedBulkMultipathSet();
    ~CodedBulkMultipathSet();

    void addCodedBulkUnicastPath (Ptr<CodedBulkUnicastPath> path);
    // for the data to send back from the destination to the source
    void setBackwardPath (Ptr<CodedBulkUnicastPath> backward_path);

    void listPath (std::ostream& os);

    void clearAllPaths (void);

    std::vector<Ptr<CodedBulkUnicastPath> > _paths;
    Ptr<CodedBulkUnicastPath> _backward_path;
};

#endif // CodedBulk_PATH_H
