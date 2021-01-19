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
#include "CodedBulk-code-vector.h"

namespace ns3 {

CodeVector::CodeVector () {
    _dimension = 0;
    _vector = NULL;
}

CodeVector::CodeVector (int dimension) {
    if(dimension > 0) {
        _dimension = dimension;
        _vector = new GF256[_dimension];
    } else {
        _dimension = 0;
        _vector = NULL;
    }
}

CodeVector::CodeVector (const CodeVector& other) {
    _dimension = 0;
    _vector = NULL;
    (*this) = other;
}

CodeVector::~CodeVector () {
    if(_vector != NULL) {
        delete [] _vector;
        _vector = NULL;
    }
}

void
CodeVector::forceResize (int dimension) {
    if(_dimension < dimension) {
        if(_vector != NULL) {
            delete [] _vector;
        }
        _dimension = dimension;
        _vector = new GF256[_dimension];
    } else {
        _dimension = dimension;
    }
}

void
CodeVector::fillWith (uint8_t number) {
    for(int i = 0; i < _dimension; ++i) {
        _vector[i] = number;
    }
}

void
CodeVector::listVector (std::ostream& os) const {
    os << "[ ";
    for(int i = 0; i < _dimension; ++i){
        if (i == 0){
            os << std::hex << +_vector[0];
        } else {
            os << ", " << std::hex << +_vector[i];
        }
    }
    os << " ]";
}

CodeVector&
CodeVector::operator=(const CodeVector& other) {
    forceResize(other._dimension);
    for(int i = 0; i < _dimension; ++i) {
        _vector[i] = other._vector[i];
    }
    return *this;
}

CodeVector&
CodeVector::operator+=(const CodeVector& other) {
    if(_dimension == other._dimension) {
        for(int i = 0; i < _dimension; ++i) {
            _vector[i] += other._vector[i];
        }
    }
    return *this;
}

CodeVector&
CodeVector::operator-=(const CodeVector& other) {
    if(_dimension == other._dimension) {
        for(int i = 0; i < _dimension; ++i) {
            _vector[i] -= other._vector[i];
        }
    }
    return *this;
}

CodeVector&
CodeVector::operator*=(const GF256 other) {
    for(int i = 0; i < _dimension; ++i) {
        _vector[i] *= other;
    }
    return *this;
}

CodeVector&
CodeVector::operator/=(const GF256 other) {
    for(int i = 0; i < _dimension; ++i) {
        _vector[i] /= other;
    }
    return *this;
}

CodeVector 
CodeVector::operator+(const CodeVector& other) const {
    CodeVector x = *this;
    x += other;
    return x;
}

CodeVector 
CodeVector::operator-(const CodeVector& other) const {
    CodeVector x = *this;
    x -= other;
    return x;
}

CodeVector
CodeVector::operator*(const GF256& other) const {
    CodeVector x = *this;
    x *= other;
    return x;
}

CodeVector
CodeVector::operator/(const GF256& other) const {
    CodeVector x = *this;
    x /= other;
    return x;
}

GF256
CodeVector::operator* (const CodeVector& other) const {
    if(_dimension != other._dimension) {
        // an arbitrary thing, another way is to use assert
        return GF256(1);
    }

    GF256 x = 0;
    for(int i = 0; i < _dimension; ++i) {
        x += _vector[i]*other._vector[i];
    }
    return x;
}

} // namespace ns3