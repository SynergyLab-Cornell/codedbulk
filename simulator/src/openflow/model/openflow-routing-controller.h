/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef OPENFLOW_ROUTING_CONTROLLER_H
#define OPENFLOW_ROUTING_CONTROLLER_H

#include "openflow-interface.h"
#include "openflow-routing-switch.h"
#include "ns3/CodedBulk-path.h"
#include "ns3/CodedBulk-simple-net-device.h"
#include <iostream>
#include <vector>
#include <list>

namespace ns3 {
namespace ofi {

/* Openflow Multipath Routing Controller */
class OpenFlowRoutingController : public Controller {
public:
  static TypeId GetTypeId (void);

  OpenFlowRoutingController();
  ~OpenFlowRoutingController();

  void AddSwitch (Ptr<OpenFlowSwitchNetDevice> swtch);

  void ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer);

  // call this function after register all the switches
  void InitializeOutputMap (void);

  // it is a directed link: a -> b
  void RegisterLink (int node_a, int node_b);
  // there is a host at the node, which will help get the right outport map
  void RegisterHostAt (int node, Ptr<SimpleNetDevice> device);
  // specify the address of the host at the node
  void RegisterHostAddress (int node, Ipv4Address address);
  // get the address of the host at the node
  Ptr<SimpleNetDevice>& GetHostDevice (int node);
  // get the address of the host at the node
  Ipv4Address& GetHostAddress (int node);
  // get the TCP proxy at the ovs at the node
  Ptr<TcpProxy> GetProxyAt (int node);

  // set a simple forward rule
  void SetForwardRuleAt (const int node, const uint32_t from_path_id, const uint32_t to_path_id);

  // int encode:
  // 0 non-coded single path TCP-IP
  // 1 non-coded multipath proxy
  // 2 coded multipath proxy
  void EstablishPath (CodedBulkUnicastPath& path, uint8_t encode);
  void EstablishPath (Ptr<CodedBulkUnicastPath>& path, uint8_t encode);

  void AddCodedBulkEncoderAt (VirtualLink* encode_link, int node);
  void AddCodedBulkDecoderAt (VirtualLink* decode_basis_map, int node);
/*
  // ns-3 OpenFlow Wildcard matching was not implemented correctly,
  // hence we implement wildcard matching ourself at the switch
  void SetupMatch (
    Ptr<OpenFlowSwitchNetDevice> swtch,
    uint32_t nw_src, uint32_t nw_dst, uint16_t dl_vlan,
    int out_port
  );
*/

  uint64_t GetTotalProxyOccupation (void) const;

private:
  class HostInfo : public SimpleRefCount<HostInfo> {
  public:
    Ptr<SimpleNetDevice>  m_device;
    Ipv4Address           m_address;
  };

  std::vector<Ptr<OpenFlowRoutingSwitch> > m_switches_vector;

  std::map<int,Ptr<HostInfo> >             m_host_info_map; // host node id -> host address

  std::vector<std::map<int,int> >     m_outport_map; // [from node id][to node id] -> output port number
  std::vector<int>                    m_outport_counter;
};

}

}

#endif /* OPENFLOW_ROUTING_CONTROLLER_H */