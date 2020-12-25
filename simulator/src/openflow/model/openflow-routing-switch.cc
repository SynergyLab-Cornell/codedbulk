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
#ifdef NS3_OPENFLOW

#include "openflow-routing-switch.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/CodedBulk-proxy.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OpenFlowRoutingSwitch");

TypeId 
OpenFlowRoutingSwitch::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::OpenFlowRoutingSwitch")
    .SetParent<OpenFlowSwitchNetDevice> ()
    .SetGroupName ("Openflow")
    .AddConstructor<OpenFlowRoutingSwitch> ()
  ;

  return tid;
}

OpenFlowRoutingSwitch::OpenFlowRoutingSwitch () :
  m_proxy(NULL)
{}

void
OpenFlowRoutingSwitch::SetProxy (Ptr<TcpProxy> proxy)
{
  m_proxy = proxy;
}

Ptr<TcpProxy>
OpenFlowRoutingSwitch::GetProxy (void) {
  return m_proxy;
}

void
OpenFlowRoutingSwitch::SetupMatch (
  uint32_t nw_src, uint32_t nw_dst, uint16_t tp_src, uint16_t tp_dst,
  int out_port
) {

  NS_LOG_DEBUG ( GetNode()->GetId() << 
               " src = " << Ipv4Address(nw_src) << ":" << tp_src << 
             " , dst = " << Ipv4Address(nw_dst) << ":" << tp_dst << " go to port " << out_port);

  //NS_LOG_FUNCTION_NOARGS ();
  routing_key key;
  //key.path_id = 0;
  key.nw_src = ntohl(nw_src);
  key.nw_dst = ntohl(nw_dst);
  key.tp_src = ntohs(tp_src);
  key.tp_dst = ntohs(tp_dst);

  routing_action action;
  action.out_port = out_port;

  m_routing_table[key] = action;
} // OpenFlowRoutingSwitch::SetupMatch

void
OpenFlowRoutingSwitch::SetupMatch (
  Ipv4Address nw_src, Ipv4Address nw_dst, uint16_t tp_src, uint16_t tp_dst,
  int out_port
) {
  SetupMatch(nw_src.Get(),nw_dst.Get(),tp_src,tp_dst,out_port);
} // OpenFlowRoutingSwitch::SetupMatch

void
OpenFlowRoutingSwitch::SetupTpDstMatch (
  uint32_t nw_src, uint32_t nw_dst, uint16_t tp_dst,
  int out_port
) {
  SetupMatch(nw_src,nw_dst,GetAnyPathID(),tp_dst,out_port);
}
void
OpenFlowRoutingSwitch::SetupTpDstMatch (
  Ipv4Address nw_src, Ipv4Address nw_dst, uint16_t tp_dst,
  int out_port
) {
  SetupTpDstMatch(nw_src.Get(),nw_dst.Get(),tp_dst,out_port);
}
void
OpenFlowRoutingSwitch::SetupTpSrcMatch (
  uint32_t nw_src, uint32_t nw_dst, uint16_t tp_src,
  int out_port
) {
  SetupMatch(nw_src,nw_dst,tp_src,GetAnyPathID(),out_port);
}
void
OpenFlowRoutingSwitch::SetupTpSrcMatch (
  Ipv4Address nw_src, Ipv4Address nw_dst, uint16_t tp_src,
  int out_port
) {
  SetupTpSrcMatch(nw_src.Get(),nw_dst.Get(),tp_src,out_port);
}

void
OpenFlowRoutingSwitch::FlowTableLookup (sw_flow_key key, ofpbuf* buffer, uint32_t packet_uid, int port, bool send_to_controller) {
  //NS_LOG_FUNCTION_NOARGS ();
  // TODO
  routing_key key_look;
  key_look.nw_src = key.flow.nw_src;
  key_look.nw_dst = key.flow.nw_dst;
  key_look.tp_src = key.flow.tp_src;
  key_look.tp_dst = key.flow.tp_dst;

  routing_action action = FindAction (key_look, packet_uid, port, send_to_controller);

  NS_LOG_DEBUG ("port src = " << ntohs(key_look.tp_src) << ", port dst = " << ntohs(key_look.tp_dst));
  RoutingPacket (action, packet_uid, port, send_to_controller);
/*
  if ( (action.out_port != -1) && action.encode ) {
    NS_LOG_DEBUG ("coded packet");
    HandleCodedBulk (key_look, packet_uid, port, send_to_controller);
  } else {
    // no path id => normal TCP/IP routing
    NS_LOG_DEBUG ("normal TCP/IP packet");
    RoutingPacket (action, packet_uid, port, send_to_controller);
  }
*/
} // OpenFlowRoutingSwitch::FlowTableLookup

void
OpenFlowRoutingSwitch::RoutingPacket (routing_action& action, uint32_t packet_uid, int port = 0, bool send_to_controller = false)
{
  if (action.out_port == -1) {
    return;
  }
  NS_LOG_INFO ("Flow matched -> route to port " << action.out_port);
  OutputPacket (packet_uid, action.out_port);

  ofpbuf* buffer = m_packetData.find (packet_uid)->second.buffer;
  // Clean up; at this point we're done with the packet.
  m_packetData.erase (packet_uid);
  discard_buffer (packet_uid);
  ofpbuf_delete (buffer);
} // OpenFlowRoutingSwitch::RoutingPacket

void
OpenFlowRoutingSwitch::HandleCodedBulk (routing_key& key_look, uint32_t packet_uid, int port_in, bool send_to_controller)
{
/*
  // TODO, need to use the proxy to peel the TCP/IP headers
  uint16_t protocolNumber = ETH_TYPE_MPLS_UNICAST;
  ofi::SwitchPacketMetadata data_in = m_packetData.find (packet_uid)->second;


  // the header is the serial number
  //MPLSHeader mpls_rm(0);
  //data_in.packet->RemoveHeader(mpls_rm);
  uint32_t serial_number = 0;//mpls_rm.GetValue();
  data_in.protocolNumber = Ipv4L3Protocol::PROT_NUMBER;

  // should remember hardware address to forward packet?
  int receive = m_codec_manager->receivePacket (key_look.tp_dst,serial_number,data_in.packet);
  if( receive == 0 ) {
    // no codec receives the packet
    routing_action action = FindAction (key_look,packet_uid,port_in,send_to_controller);
    RoutingPacket (action, packet_uid, port_in, send_to_controller);
    return;
  }

  ofpbuf *buffer = NULL;
  Ptr<CodedBulkOutput> output;
  while ( (output = m_codec_manager->popOutput ()) != NULL ) {
    Ptr<Packet> packet   = output->_output_packet;

    //MPLSHeader mpls_hd_serial (output->_serial_number);
    //packet->AddHeader (mpls_hd_serial);

    // send
    buffer = BufferFromPacket (packet,data_in.src,data_in.dst,GetMtu (),protocolNumber);

    uint32_t packet_uid_out = save_buffer (buffer);
    ofi::SwitchPacketMetadata data_out;
    data_out.packet = packet;
    data_out.buffer = buffer;
    data_out.protocolNumber = protocolNumber;
    data_out.src = data_in.src;  // not important
    data_out.dst = data_in.dst;  // not important
    m_packetData.insert (std::make_pair (packet_uid_out, data_out));

    NS_LOG_DEBUG("output CodedBulk serial number " << output->_serial_number << " with size " << packet->GetSize());

    key_look.tp_dst = output->_path_id;
    routing_action action = FindAction (key_look,packet_uid_out,port_in,send_to_controller);
    RoutingPacket (action,packet_uid_out,port_in,send_to_controller);
  }

  buffer = data_in.buffer;
  m_packetData.erase (packet_uid);
  discard_buffer (packet_uid);
  ofpbuf_delete (buffer);
*/
} // OpenFlowRoutingSwitch::HandleCodedBulk

OpenFlowRoutingSwitch::routing_action
OpenFlowRoutingSwitch::FindAction (routing_key& key_look, uint32_t packet_uid, int port, bool send_to_controller)
{
  routing_action action;
  action.out_port = -1;
  if (packet_uid == 0xffffffff) {
    NS_LOG_DEBUG ("packet uid overflown");
    // buffer insert fail
    return action;
  }

  NS_LOG_DEBUG ( GetNode()->GetId() << 
               " src = " << Ipv4Address(ntohl(key_look.nw_src)) << ":" << ntohs(key_look.tp_src) << 
             " , dst = " << Ipv4Address(ntohl(key_look.nw_dst)) << ":" << ntohs(key_look.tp_dst));

  // try forward(tp_dst) matching first
  uint16_t src_port = key_look.tp_src;
  key_look.tp_src = GetAnyPathID();
  if (m_routing_table.find(key_look) == m_routing_table.end()) {
    key_look.tp_src = src_port;
    key_look.tp_dst = GetAnyPathID();

    // try backward(tp_src) matching
    if (m_routing_table.find(key_look) == m_routing_table.end()) {
      // try overall port
      key_look.tp_src = GetAnyPathID();
      
      if (m_routing_table.find(key_look) == m_routing_table.end()) {
        NS_LOG_INFO ("Flow not matched.");
        if (send_to_controller){
          OutputControl (packet_uid, port, m_missSendLen, OFPR_NO_MATCH);
        }
        // Clean up; at this point we're done with the packet.
        m_packetData.erase (packet_uid);
        return action;
      }
    }
  }
  action = m_routing_table[key_look];
  return action;
} // OpenFlowRoutingSwitch::FindAction

void
OpenFlowRoutingSwitch::SetCodecManager (Ptr<CodedBulkCodecManager> codec_manager)
{
  Ptr<CodedBulkProxy> nc_proxy = DynamicCast<CodedBulkProxy, TcpProxy> (m_proxy);
  nc_proxy->SetCodecManager( codec_manager );
} // OpenFlowRoutingSwitch::SetupCodecManager

Ptr<CodedBulkCodecManager>
OpenFlowRoutingSwitch::GetCodecManager (void)
{
  Ptr<CodedBulkProxy> nc_proxy = DynamicCast<CodedBulkProxy, TcpProxy> (m_proxy);
  return nc_proxy->GetCodecManager();
}

} // namespace ns3

#endif