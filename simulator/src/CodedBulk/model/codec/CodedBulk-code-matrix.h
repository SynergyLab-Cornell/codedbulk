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
#ifndef CodedBulk_CODE_MATRIX_H
#define CodedBulk_CODE_MATRIX_H

#include "CodedBulk-code-vector.h"
#include <vector>

namespace ns3 {

class CodeMatrix {
public:
    CodeMatrix ();
    CodeMatrix (int row_dimension, int col_dimension);
    CodeMatrix (const CodeMatrix& other);

    ~CodeMatrix();

    inline int getRowDimension () const {  return _row_dimension;  }
    inline int getColDimension () const {  return _col_dimension;  }

    void listMatrix (std::ostream& os) const;

    // this resize does not keep data when expanding
    void forceResize (int row_dimension, int col_dimension);
    void singleRow (CodeVector& vector);
    void addRow (CodeVector& vector);

    // set the diagonal elements as other
    void diagonal(const GF256 other);
    void inverse();
    CodeMatrix getInverse() const;

    CodeMatrix& operator =(const CodeMatrix& other);
    CodeMatrix& operator+=(const CodeMatrix& other);
    CodeMatrix& operator-=(const CodeMatrix& other);
    CodeMatrix& operator*=(const GF256 other);
    CodeMatrix& operator/=(const GF256 other);

    CodeMatrix  operator+(const CodeMatrix& other) const;
    CodeMatrix  operator-(const CodeMatrix& other) const;
    CodeMatrix  operator*(const GF256& other) const;
    CodeMatrix  operator/(const GF256& other) const;

    inline CodeVector& operator[] (int row_dimension) {  return _rows[row_dimension];  }
    // product
    CodeVector  operator* (const CodeVector other) const;

    int _row_dimension;
    int _col_dimension;
    CodeVector* _rows;
};

} // namespace ns3

#endif // CodedBulk_CODE_MATRIX_H