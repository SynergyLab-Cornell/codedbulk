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
#ifndef ADDRESS_H
#define ADDRESS_H
#include <netinet/in.h>
#include <string>

typedef struct in_addr Address;
typedef std::string IPCAddress;

int IpAddressPtoN(const char* ip, Address& addr);

Address IpToAddress(const char* ip);

IPCAddress AddressToIPCAddress(Address& addr);

#endif /* ADDRESS_H */