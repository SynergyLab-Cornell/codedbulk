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
#ifndef OPENFLOW_ROUTING_CONTROLLER_H
#define OPENFLOW_ROUTING_CONTROLLER_H

#include "simple-ref-count.h"
#include "ptr.h"
#include "address.h"
#include "CodedBulk-path.h"
#include <iostream>
#include <vector>
#include <list>
#include <map>

/* Openflow Multipath Routing Controller */
class OpenFlowRoutingController : public SimpleRefCount<OpenFlowRoutingController> {
public:
  OpenFlowRoutingController();
  ~OpenFlowRoutingController();

  // call this function after register all the switches
  void InitializeOutputMap (void);

  // it is a directed link: a -> b
  void RegisterLink (int node_a, int node_b);
  // there is a host at the node, which will help get the right outport map
  void RegisterHostAt (int node);
  // specify the address of the host at the node
  void RegisterHostAddress (int node, IPCAddress address);
  // get the address of the host at the node
  IPCAddress& GetHostAddress (int node);
  // get the TCP proxy at the ovs at the node
  //Ptr<TcpProxy> GetProxyAt (int node);

  // set a simple forward rule
  void SetForwardRuleAt (const int node, const uint32_t from_path_id, const uint32_t to_path_id);

  void EstablishPath (CodedBulkUnicastPath& path, bool encode);
  void EstablishPath (Ptr<CodedBulkUnicastPath>& path, bool encode);

  void AddCodedBulkEncoderAt (VirtualLink* encode_link, int node);
  void AddCodedBulkDecoderAt (VirtualLink* decode_basis_map, int node);

  std::map<int, std::list<VirtualLink*> > m_all_codecs;

private:
  class HostInfo : public SimpleRefCount<HostInfo> {
  public:
    IPCAddress           m_address;
  };

  std::vector<Address>                m_switch_address_vector;

  std::map<int,Ptr<HostInfo> >        m_host_info_map; // host node id -> host address

  std::vector<std::map<int,int> >     m_outport_map; // [from node id][to node id] -> output port number
  std::vector<int>                    m_outport_counter;
};

#endif /* OPENFLOW_ROUTING_CONTROLLER_H */