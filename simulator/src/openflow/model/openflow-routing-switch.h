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
 * Author: Shih-Hao Tseng  <st688@cornell.edu>
 */

#ifndef OPENFLOW_ROUTING_SWITCH_H
#define OPENFLOW_ROUTING_SWITCH_H

#include "openflow-switch-net-device.h"
#include "ns3/CodedBulk-codec-manager.h"
#include "ns3/tcp-proxy.h"
#define OPENFLOW_ROUTING_SWITCH_ANY_PATH_ID 0xffffffff

namespace ns3 {

/**
 * \ingroup openflow 
 * \brief A net device that switches multiple LAN segments via an OpenFlow-compatible flow table
 */
class OpenFlowRoutingSwitch : public OpenFlowSwitchNetDevice
{
private:
typedef struct _routing_key_ {
  //uint32_t path_id;
  uint32_t nw_src;
  uint32_t nw_dst;
  uint16_t tp_src;
  uint16_t tp_dst;

  _routing_key_ () :
    //path_id(0),
    nw_src(0),
    nw_dst(0),
    tp_src(0),
    tp_dst(0)
  {}
  
  _routing_key_ (const struct _routing_key_& r) :
    //path_id(r.path_id),
    nw_src(r.nw_src),
    nw_dst(r.nw_dst),
    tp_src(r.tp_src),
    tp_dst(r.tp_dst)
  {}

  bool operator< (const struct _routing_key_& r) const {
    if(nw_src < r.nw_src) {
      return true;
    } else if (nw_src > r.nw_src) {
      return false;
    }
    if(nw_dst < r.nw_dst) {
      return true;
    } else if (nw_dst > r.nw_dst) {
      return false;
    }
    if(tp_src < r.tp_src) {
      return true;
    } else if (tp_src > r.tp_src) {
      return false;
    }
    if(tp_dst < r.tp_dst) {
      return true;
    }
    return false;
  }
  void operator= (const struct _routing_key_& r) {
    //path_id = r.path_id;
    nw_src  = r.nw_src;
    nw_dst  = r.nw_dst;
    tp_src  = r.tp_src;
    tp_dst  = r.tp_dst;
  }
} routing_key;
typedef struct _routing_action_ {
  int  out_port;
  //bool encode;
  void operator= (const struct _routing_action_& r) {
    out_port = r.out_port;
    //encode   = r.encode;
  }
} routing_action;

public:
  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  static uint32_t GetAnyPathID() {  return OPENFLOW_ROUTING_SWITCH_ANY_PATH_ID;  }

  OpenFlowRoutingSwitch ();

  void SetProxy (Ptr<TcpProxy> proxy);
  Ptr<TcpProxy> GetProxy (void);

  void SetupMatch (
    uint32_t nw_src, uint32_t nw_dst, uint16_t tp_src, uint16_t tp_dst,
    int out_port
  );
  void SetupMatch (
    Ipv4Address nw_src, Ipv4Address nw_dst, uint16_t tp_src, uint16_t tp_dst,
    int out_port
  );

  void SetupTpDstMatch (
    uint32_t nw_src, uint32_t nw_dst, uint16_t tp_dst,
    int out_port
  );
  void SetupTpDstMatch (
    Ipv4Address nw_src, Ipv4Address nw_dst, uint16_t tp_dst,
    int out_port
  );
  void SetupTpSrcMatch (
    uint32_t nw_src, uint32_t nw_dst, uint16_t tp_src,
    int out_port
  );
  void SetupTpSrcMatch (
    Ipv4Address nw_src, Ipv4Address nw_dst, uint16_t tp_src,
    int out_port
  );

  void SetCodecManager (Ptr<CodedBulkCodecManager> codec_manager);
  Ptr<CodedBulkCodecManager> GetCodecManager (void);

protected:
  virtual void FlowTableLookup (sw_flow_key key, ofpbuf* buffer, uint32_t packet_uid, int port, bool send_to_controller);

  void RoutingPacket (routing_action& action, uint32_t packet_uid, int port, bool send_to_controller);

  void HandleCodedBulk (routing_key& key_look, uint32_t packet_uid, int port, bool send_to_controller);

  routing_action FindAction (routing_key& key_look, uint32_t packet_uid, int port, bool send_to_controller);

  std::map<routing_key,routing_action> m_routing_table;

  Ptr<TcpProxy> m_proxy;
};

} // namespace ns3

#endif /* OPENFLOW_ROUTING_SWITCH_H */
