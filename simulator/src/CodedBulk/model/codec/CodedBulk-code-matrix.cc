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
#include "CodedBulk-code-matrix.h"

namespace ns3 {

CodeMatrix::CodeMatrix () {
    _row_dimension = 0;
    _col_dimension = 0;
    _rows = NULL;
}

CodeMatrix::CodeMatrix (int row_dimension, int col_dimension) {
    _rows = NULL;
    if(row_dimension == 0) {
        _row_dimension = 0;
        _col_dimension = 0;
        return;
    }
    _row_dimension = row_dimension;
    _col_dimension = col_dimension;
    _rows = new CodeVector[_row_dimension];
    for(int i = 0; i < _row_dimension; ++i) {
        _rows[i].forceResize(_col_dimension);
    }
}

CodeMatrix::CodeMatrix (const CodeMatrix& other) {
    _rows = NULL;
    _row_dimension = other._row_dimension;
    _col_dimension = other._col_dimension;
    if(_row_dimension == 0) {
        _col_dimension = 0;
        return;
    }
    _rows = new CodeVector[_row_dimension];
    for(int i = 0; i < _row_dimension; ++i) {
        _rows[i] = other._rows[i];
    }
}

CodeMatrix::~CodeMatrix() {
    _row_dimension = 0;
    _col_dimension = 0;
    if(_rows != NULL) {
        delete [] _rows;
        _rows = NULL;
    }
}

void
CodeMatrix::listMatrix (std::ostream& os) const {
    if(_row_dimension == 0) {
        return;
    }
    _rows[0].listVector(os);
    for(int i = 1; i < _row_dimension; ++i) {
        os << std::endl;
        _rows[i].listVector(os);
    }
}

void
CodeMatrix::forceResize (int row_dimension, int col_dimension) {
    bool lack_row = (_row_dimension < row_dimension);
    bool lack_col = (_col_dimension < col_dimension);
    _row_dimension = row_dimension;
    _col_dimension = col_dimension;

    if(lack_row) {
        if(_rows != NULL) {
            delete [] _rows;
        }
        _rows = new CodeVector[_row_dimension];
        lack_col = true;
    }
    if(lack_col) {
        for(int i = 0; i < _row_dimension; ++i) {
            _rows[i].forceResize(_col_dimension);
        }
    }
}

void
CodeMatrix::singleRow (CodeVector& vector) {
    forceResize(1,vector.getDimension());
    for(int j = 0; j < _col_dimension; ++j) {
        _rows[0][j] = vector[j];
    }
}

void
CodeMatrix::addRow (CodeVector& vector) {
    if( vector.getDimension() == _col_dimension ) {
        CodeVector* prev = _rows;
        ++_row_dimension;
        _rows = new CodeVector[_row_dimension];
        for(int i = 0; i < _row_dimension - 1; ++i) {
            _rows[i] = prev[i];
        }
        delete [] prev;
        _rows[_row_dimension-1] = vector;
    }
}

void
CodeMatrix::diagonal(const GF256 other) {
    int dim = _row_dimension;
    if (_col_dimension < _row_dimension) {
        dim = _col_dimension;
    }
    for(int i = 0; i < dim; ++i) {
        _rows[i][i] = other;
    }
}

void
CodeMatrix::inverse() {
    if( _row_dimension != _col_dimension ) {
        // cannot be inversed
        return;
    }
    // Gauss-Jordan Elimination Method
    CodeMatrix x(_row_dimension,_col_dimension);
    x.diagonal(1);
    
    int non_zero_row = -1;
    CodeVector swap(_col_dimension);
    for (int j = 0; j < _col_dimension; ++j) {
        // find the first non-zero
        non_zero_row = -1;
        for (int i = j; i < _row_dimension; ++i) {
            if(_rows[i][j] != 0) {
                non_zero_row = i;
                break;
            }
        }
        if (non_zero_row == -1) {
            // not invertible
            // the matrix has been modified
            return;
        } else if (non_zero_row != j) {
            // swap it
            swap = x[non_zero_row];
            x[non_zero_row] = x[j];
            x[j] = swap;

            swap = _rows[non_zero_row];
            _rows[non_zero_row] = _rows[j];
            _rows[j] = swap;
        }

        // normalization
        if ( _rows[j][j] != 1 ) {
            x[j]     /= _rows[j][j];
            _rows[j] /= _rows[j][j];
        }

        // elimination
        for(int i = j+1; i < _row_dimension; ++i) {
            if (_rows[i][j] != 0) {
                x[i]     -= (x[j]*_rows[i][j]);
                _rows[i] -= (_rows[j]*_rows[i][j]);
            }
        }
    }
    // now we have an upper triangle
    // work on the lower triangle
    for (int j = _col_dimension-1; j >= 0; --j) {
        for(int i = 0; i < j; ++i) {
            // elimination
            if (_rows[i][j] != 0) {
                x[i]     -= (x[j]*_rows[i][j]);
                _rows[i] -= (_rows[j]*_rows[i][j]);
            }
        }
    }

    *this = x;
}

CodeMatrix
CodeMatrix::getInverse() const {
    CodeMatrix x = *this;
    x.inverse();
    return x;
}

CodeMatrix&
CodeMatrix::operator=(const CodeMatrix& other) {
    forceResize(other._row_dimension, other._col_dimension);
    for(int i = 0; i < _row_dimension; ++i) {
        _rows[i] = other._rows[i];
    }
    return *this;
}

CodeMatrix&
CodeMatrix::operator+=(const CodeMatrix& other) {
    if( (_row_dimension == other._row_dimension) &&
        (_col_dimension == other._col_dimension) ){
        for(int i = 0; i < _row_dimension; ++i) {
            _rows[i] += other._rows[i];
        }
    }
    return *this;
}

CodeMatrix&
CodeMatrix::operator-=(const CodeMatrix& other) {
    if( (_row_dimension == other._row_dimension) &&
        (_col_dimension == other._col_dimension) ){
        for(int i = 0; i < _row_dimension; ++i) {
            _rows[i] -= other._rows[i];
        }
    }
    return *this;
}

CodeMatrix&
CodeMatrix::operator*=(const GF256 other) {
    for(int i = 0; i < _row_dimension; ++i) {
        _rows[i] *= other;
    }
    return *this;
}

CodeMatrix&
CodeMatrix::operator/=(const GF256 other) {
    for(int i = 0; i < _row_dimension; ++i) {
        _rows[i] /= other;
    }
    return *this;
}

CodeMatrix 
CodeMatrix::operator+(const CodeMatrix& other) const {
    CodeMatrix x = *this;
    x += other;
    return x;
}

CodeMatrix 
CodeMatrix::operator-(const CodeMatrix& other) const {
    CodeMatrix x = *this;
    x -= other;
    return x;
}

CodeMatrix
CodeMatrix::operator*(const GF256& other) const {
    CodeMatrix x = *this;
    x *= other;
    return x;
}

CodeMatrix
CodeMatrix::operator/(const GF256& other) const {
    CodeMatrix x = *this;
    x /= other;
    return x;
}

CodeVector
CodeMatrix::operator* (const CodeVector other) const {
    if(other.getDimension() != getRowDimension()) {
        // an arbitrary thing, another way is to use assert
        return other;
    }

    CodeVector x = other;
    for(int i = 0; i < _row_dimension; ++i){
        x[i] = _rows[i]*other;
    }
    return x;
}

} // namespace ns3