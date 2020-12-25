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
#ifndef CodedBulk_FINITE_FIELD_H
#define CodedBulk_FINITE_FIELD_H

#include <stdint.h>

namespace ns3 {

// GF(2^8)
class GF256 {
public:
    uint8_t _num;

    static const int __code_length;
    static const uint8_t ltable[256];
    static const uint8_t atable[256];

    GF256() : _num(0) {}
    GF256(uint8_t num) : _num(num) {}

    inline uint8_t operator*() const {  return _num;  }
    inline unsigned int  operator+() const {  return static_cast<unsigned int>(_num);  }

    inline bool operator==(const uint8_t& other) const {  return (_num == other);  }
    inline bool operator==(const GF256& other)   const {  return (*this == other._num);  }
    inline bool operator!=(const uint8_t& other) const {  return (_num != other);  }
    inline bool operator!=(const GF256& other)   const {  return (*this != other._num);  }

    inline bool operator< (const GF256& other) const {  return (_num < other._num);  }
    inline bool operator<=(const GF256& other) const {  return (_num <= other._num);  }
    inline bool operator> (const GF256& other) const {  return (_num > other._num);  }
    inline bool operator>=(const GF256& other) const {  return (_num >= other._num);  }
/*
    GF256& operator=(const uint8_t& other);
    GF256& operator=(const GF256& other);
    GF256& operator+=(const uint8_t& other);
    GF256& operator+=(const GF256& other);
    GF256& operator-=(const uint8_t& other);
    GF256& operator-=(const GF256& other);
    GF256& operator*=(const uint8_t& other);
    GF256& operator*=(const GF256& other);
    GF256& operator/=(const uint8_t& other);
    GF256& operator/=(const GF256& other);

    GF256  operator+(const uint8_t& other) const;
    GF256  operator+(const GF256& other) const;
    GF256  operator-(const uint8_t& other) const;
    GF256  operator-(const GF256& other) const;
    GF256  operator-() const;
    GF256  operator*(const uint8_t& other) const;
    GF256  operator*(const GF256& other) const;
    GF256  operator/(const uint8_t& other) const;
    GF256  operator/(const GF256& other) const;
*/
    inline GF256& operator=(const uint8_t& other) {
        _num = other;
        return *this;
    }
    
    inline GF256& operator=(const GF256& other) {
        _num = other._num;
        return *this;
    }
    
    inline GF256& operator+=(const uint8_t& other) {
        _num ^= other;
        return *this;
    }
    
    inline GF256& operator+=(const GF256& other) {
        return (*this += other._num);
    }
    
    inline GF256& operator-=(const uint8_t& other) {
        _num ^= other;
        return *this;
    }
    
    inline GF256& operator-=(const GF256& other) {
        return (*this -= other._num);
    }
    
    inline GF256& operator*=(const uint8_t& other) {
        if (_num == 0) {
            return *this;
        }
        if (other == 0) {
            _num = 0;
            return *this;
        }
        uint16_t x = ltable[_num]+ltable[other];
        if(x > 0xff){
            x %= 0xff;
        }
        _num = atable[static_cast<uint8_t>(x)];
        return *this;
    }
    
    inline GF256& operator*=(const GF256& other) {
        return (*this *= other._num);
    }
    
    inline GF256& operator/=(const uint8_t& other) {
        if (_num == 0) {
            return *this;
        }
        if (other == 0) {
            _num = 0;
            return *this;
        }
        uint8_t a = ltable[_num];
        uint8_t b = ltable[other];
        if(a < b) {
            _num = atable[0xff - b + a];
        } else {
            _num = atable[a - b];
        }
        return *this;
    }
    
    inline GF256& operator/=(const GF256& other) {
        return (*this /= other._num);
    }
    
    
    inline GF256 operator+(const uint8_t& other) const {
        return GF256(_num^other);
    }
    
    inline GF256 operator+(const GF256& other) const {
        return (*this)+other._num;
    }
    
    inline GF256 operator-(const uint8_t& other) const {
        return GF256(_num^other);
    }
    
    inline GF256 operator-(const GF256& other) const {
        return (*this)-other._num;
    }
    
    inline GF256 operator-() const {
        return _num^0;
    }
    
    inline GF256 operator*(const uint8_t& other) const {
    /*
    	uint8_t p = 0; // the product of the multiplication
        uint8_t a = _num;
        uint8_t b = other;
        while ((a != 0) && (b != 0)) {
                if ((b & 0x01) != 0 ) // if b is odd, then add the corresponding a to p (final product = sum of all a's corresponding to odd b's)
                    p ^= a; // since we're in GF(2^m), addition is an XOR
    
                if ((a & 0x80) != 0) { // GF modulo: if a >= 128, then it will overflow when shifted left, so reduce
                    a = (a << 1) ^ 0x1b; 
                    // XOR with the primitive polynomial x^8 + x^4 + x^3 + x + 1 
                    // (0b1_0001_1011) = 0x11b – you can change it but it must be irreducible
                } else {
                    a <<= 1; // equivalent to a*2
                }
                // the following two lines equivalent to b / 2
                b >>= 1; 
                b &= 0x7f;
    	}
        return GF256(p);
    */
        if (_num == 0) {
            return GF256(0);
        }
        if (other == 0) {
            return GF256(0);
        }
        uint16_t x = ltable[_num]+ltable[other];
        if(x > 0xff){
            x %= 0xff;
        }
        return GF256(atable[static_cast<uint8_t>(x)]);
    }
    
    inline GF256 operator*(const GF256& other) const {
        return (*this)*other._num;
    }
    
    inline GF256 operator/(const uint8_t& other) const {
        if (_num == 0) {
            // actually, we should not divide 0, other != 0
            return GF256(0);
        }
        if (other == 0) {
            return GF256(0);
        }
        uint8_t a = ltable[_num];
        uint8_t b = ltable[other];
        uint8_t diff = 0;
        if(a < b) {
            diff = 0xff - b + a;
        } else {
            diff = a - b;
        }
    
        return GF256(atable[diff]);
    }
    
    inline GF256 operator/(const GF256& other) const {
        return (*this)/other._num;
    }
};

} // namespace ns3

#endif // CodedBulk_FINITE_FIELD_H
