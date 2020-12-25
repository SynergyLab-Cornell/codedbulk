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
#ifndef CodedBulk_SYSTEM_PARAMETERS_H
#define CodedBulk_SYSTEM_PARAMETERS_H

#define MAX_CODEC_INPUT_SIZE 15   // 15 by 15 would be sufficient?

#define DATASEG_SIZE 1000

#define PROXY_PORT 1000
#define INTERACTIVE_PORT 5000

// store this amount of data before forwarding
// 10G
//#define STORE_AND_FORWARD_SIZE 10000000000
// 100k
#define STORE_AND_FORWARD_SIZE 1000000

// number of packets, each 1kB
// 1000 is the largest my machine can support
#define MAX_PROXY_INPUT_BUFFER_SIZE 1000  //100000

#endif /* CodedBulk_SYSTEM_PARAMETERS_H */
