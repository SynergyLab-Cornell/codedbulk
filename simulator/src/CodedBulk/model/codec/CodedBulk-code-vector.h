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
#ifndef CodedBulk_CODE_VECTOR_H
#define CodedBulk_CODE_VECTOR_H

#include "CodedBulk-finite-field.h"
#include <ostream>

namespace ns3 {

class CodeVector {
public:
    CodeVector ();
    CodeVector (int dimension);
    CodeVector (const CodeVector& other);

    ~CodeVector ();

    // this resize does not keep data when expanding
    void forceResize (int dimension);
    void fillWith (uint8_t number);
    inline int getDimension () const {  return _dimension;  }
    void listVector (std::ostream& os) const;

    CodeVector& operator=(const CodeVector& other);
    CodeVector& operator+=(const CodeVector& other);
    CodeVector& operator-=(const CodeVector& other);
    // should not use const GF256& for *= and /=,
    // otherwise a /= a[0] will fail since a[1]/(a[0]/a[0]) = a[1]
    CodeVector& operator*=(const GF256 other);
    CodeVector& operator/=(const GF256 other);

    CodeVector  operator+(const CodeVector& other) const;
    CodeVector  operator-(const CodeVector& other) const;
    CodeVector  operator*(const GF256& other) const;
    CodeVector  operator/(const GF256& other) const;

    inline GF256& operator[] (int dimension) {  return _vector[dimension];  }
    // inner product
    GF256  operator* (const CodeVector& other) const;

    // handle manually
    GF256* _vector;
    int    _dimension;
};

} // namespace ns3

#endif // CodedBulk_CODE_VECTOR_H