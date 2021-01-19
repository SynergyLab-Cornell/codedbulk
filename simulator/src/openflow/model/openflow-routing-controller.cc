/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifdef NS3_OPENFLOW

#include "openflow-routing-controller.h"
#include "openflow-switch-net-device.h"

#include "ns3/ipv4-l3-protocol.h"
#include "ns3/tcp-l4-protocol.h"

#define PROXY_PORT 1000

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OpenFlowRoutingController");

namespace ofi {

TypeId
OpenFlowRoutingController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ofi::OpenFlowRoutingController")
    .SetParent (Controller::GetTypeId ())
    .SetGroupName ("Openflow")
    .AddConstructor<OpenFlowRoutingController> ()
  ;
  return tid;
} // OpenFlowRoutingController::GetTypeId

OpenFlowRoutingController::OpenFlowRoutingController()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_switches_vector.clear();
  m_host_info_map.clear();
} // OpenFlowRoutingController::OpenFlowRoutingController

OpenFlowRoutingController::~OpenFlowRoutingController()
{
  NS_LOG_FUNCTION_NOARGS ();
} // OpenFlowRoutingController::~OpenFlowRoutingController

void
OpenFlowRoutingController::AddSwitch (Ptr<OpenFlowSwitchNetDevice> swtch)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_switches.find (swtch) != m_switches.end ())
    {
      NS_LOG_INFO ("This Controller has already registered this switch!");
    }
  else
    {
      m_switches.insert (swtch);
      m_switches_vector.push_back (DynamicCast<OpenFlowRoutingSwitch,OpenFlowSwitchNetDevice>(swtch));
    }
} // OpenFlowRoutingController::AddSwitch

void
OpenFlowRoutingController::ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Drop it
  if (m_switches.find (swtch) == m_switches.end ()) {
    NS_LOG_ERROR ("Can't receive from this switch, not registered to the Controller.");
    return;
  }

  // We have received any packet at this point, so we pull the header to figure out what type of packet we're handling.
  uint8_t type = GetPacketType (buffer);
  switch( type ) {
    case OFPT_PACKET_IN:
      {
      NS_LOG_INFO ("Receive an OFPT_PACKET_IN");
      // The switch didn't understand the packet it received, so it forwarded it to the controller.
      ofp_packet_in * opi = (ofp_packet_in*)ofpbuf_try_pull (buffer, offsetof (ofp_packet_in, data));
      // now the buffer contains the true data
      int port = ntohs (opi->in_port);
      NS_LOG_INFO ("Packet was from " << port);

      // Create matching key.
      sw_flow_key key;
      key.wildcards = 0;
      flow_extract (buffer, port != -1 ? port : OFPP_NONE, &key.flow);

      // the packet from hosts 
      if(key.flow.nw_proto == 0){
        // no l4 proto
        if(key.flow.dl_type == ntohs(ETH_TYPE_ARP)){
          NS_LOG_DEBUG ("ETH_TYPE_ARP, dl type " << std::hex << htons(key.flow.dl_type) );

/*
          // openflow 0.8.9 use ethernet protocol when IP cannot be found
          // the types are defined under openflow/lib/packets.h
          //ARP
          //arp_eth_header* arp_h = (arp_eth_header*)buffer->l3; // it is the same as below
          arp_eth_header* arp_h = (arp_eth_header*)((uint8_t*)buffer->data + ETH_HEADER_LEN);
          uint32_t switchIndex = arp_h->ar_tpa - ntohl(BASE_ADDRESS) - 1;
          // ignore the ethernet header
          uint16_t packet_len = buffer->size - ETH_HEADER_LEN;// - ARP_ETH_HEADER_LEN;

          ofp_packet_out* opo = (ofp_packet_out*)malloc (sizeof(ofp_packet_out) + sizeof(ofp_action_output) + packet_len);
          opo->header.version = OFP_VERSION;
          opo->header.type = OFPT_PACKET_OUT;
          opo->header.length = htons (sizeof(ofp_packet_out) + sizeof(ofp_action_output) + packet_len);

          opo->buffer_id = htonl((uint32_t)-1);
          opo->in_port   = htons(OFPP_NONE);
          opo->actions_len = htons (sizeof(ofp_action_output));

          ofp_action_output* oao = (ofp_action_output*)((uint8_t*)opo + sizeof(ofp_packet_out));
          oao->type = htons (OFPAT_OUTPUT);
          oao->len  = htons (sizeof(ofp_action_output));
          oao->port = 0;

          void* packet_data = (uint8_t*)buffer->data + ETH_HEADER_LEN;// + ARP_ETH_HEADER_LEN;
          memcpy((uint8_t*)opo+sizeof(ofp_packet_out)+sizeof(ofp_action_output), packet_data, packet_len);

          NS_LOG_INFO ("Send ARP to switch " << switchIndex);
          // Ask/answer its target
          //SendToSwitch (m_switches_vector[switchIndex], opo, opo->header.length);
          Simulator::Schedule (Seconds(0), &UDRController::SendToSwitch, this,
             m_switches_vector[switchIndex], opo, opo->header.length);
*/
        } else {
          NS_LOG_ERROR ("Not an ETH_TYPE_ARP, dl type " << std::hex << htons(key.flow.dl_type) );
        }
      }

      }
      break;
    case OFPT_FLOW_EXPIRED:
      NS_LOG_INFO ("Receive an OFPT_FLOW_EXPIRED");
      break;
    case OFPT_PORT_STATUS:
      NS_LOG_INFO ("Receive an OFPT_PORT_STATUS");
      break;
    case OFPT_PACKET_OUT:
      NS_LOG_INFO ("Receive an OFPT_PACKET_OUT");
      break;
    default:
      NS_LOG_ERROR ("An not identified packet with type " << (uint32_t)type);
      break;
  }

} // OpenFlowRoutingController::ReceiveFromSwitch

void
OpenFlowRoutingController::InitializeOutputMap (void)
{
  int total_switches = m_switches_vector.size();
  m_outport_map.resize(total_switches);
  m_outport_counter.resize(total_switches);
  for(int n = 0; n < total_switches; ++n) {
    m_outport_map[n].clear();
    m_outport_counter[n] = 0;
  }
}

void
OpenFlowRoutingController::RegisterHostAt (int node, Ptr<SimpleNetDevice> device)
{
  NS_LOG_INFO(node);
  m_host_info_map[node] = Create<HostInfo> ();
  m_host_info_map[node]->m_device = device;
  ++m_outport_counter[node];
}

void
OpenFlowRoutingController::RegisterHostAddress (int node, Ipv4Address address)
{
  if( m_host_info_map.find(node) != m_host_info_map.end() ) {
    m_host_info_map[node]->m_address = address;
  }
}

Ptr<SimpleNetDevice>&
OpenFlowRoutingController::GetHostDevice (int node)
{
  return m_host_info_map[node]->m_device;
}

Ipv4Address&
OpenFlowRoutingController::GetHostAddress (int node)
{
  return m_host_info_map[node]->m_address;
}

Ptr<TcpProxy>
OpenFlowRoutingController::GetProxyAt (int node)
{
  return m_switches_vector[node]->GetProxy();
}

void
OpenFlowRoutingController::SetForwardRuleAt (const int node, const uint32_t from_path_id, const uint32_t to_path_id)
{
  GetProxyAt(node)->SetForwardRule (from_path_id, to_path_id);
}

void
OpenFlowRoutingController::RegisterLink (int node_a, int node_b)
{
  // it is a directed link
  m_outport_map[node_a][node_b] = m_outport_counter[node_a]++;
}

void
OpenFlowRoutingController::EstablishPath (CodedBulkUnicastPath& path, uint8_t encode)
{
  if( path._nodes.empty() ) {
    NS_LOG_DEBUG("The path is empty!");
    return;
  }

  std::list<int>::iterator it = path._nodes.begin();
  std::list<int>::iterator it_next = path._nodes.begin(); 
  ++it_next;
  switch ( encode ) {
    case 6:
      {
        // TCP/IP path
        Ipv4Address& nw_src = m_host_info_map[path._nodes.front()]->m_address;
        Ipv4Address& nw_dst = m_host_info_map[path._nodes.back()]->m_address;

        m_switches_vector[*it]->SetupTpSrcMatch(
          nw_dst,
          nw_src,
          OpenFlowRoutingSwitch::GetAnyPathID(),
          0
        );
        for(; it_next != path._nodes.end(); ++it, ++it_next)
        {
          m_switches_vector[*it]->SetupTpDstMatch(
            nw_src,
            nw_dst,
            path._application_port,
            m_outport_map[*it][*it_next]
          );
          m_switches_vector[*it_next]->SetupTpSrcMatch(
            nw_dst,
            nw_src,
            path._application_port,
            m_outport_map[*it_next][*it]
          );
        }
        m_switches_vector[*it]->SetupTpDstMatch(
          nw_src,
          nw_dst,
          OpenFlowRoutingSwitch::GetAnyPathID(),
          0
        );
      }
      break;

    case 4:
      {
        // MPLS based path
        // simple forwarding and bulk coding
        // establish hop-by-hop connection
        // source to proxy
        
        // forward
        GetProxyAt(*it)->RegisterSender(path._path_id);
        GetProxyAt(*it)->RegisterPriority(path._path_id,2);
        SetForwardRuleAt (*it, path._path_id, path._path_id);

        // coding
        GetProxyAt(*it)->RegisterSender(path._path_id+1);
        GetProxyAt(*it)->RegisterPriority(path._path_id+1,4);

        m_switches_vector[*it]->SetupTpSrcMatch(
          GetProxyAt(*it)->GetBaseAddr(),
          GetHostAddress(*it),
          PROXY_PORT,
          0
        );

        for(; it_next != path._nodes.end(); ++it, ++it_next)
        {
          // proxy to proxy
          // forward
          GetProxyAt(*it)->RegisterProxyAddr(
            path._path_id, GetProxyAt(*it_next)->GetBaseAddr()
          );
          GetProxyAt(*it)->RegisterPriority(path._path_id,2);
          SetForwardRuleAt (*it, path._path_id, path._path_id);

          // coding
          GetProxyAt(*it)->RegisterProxyAddr(
            path._path_id+1, GetProxyAt(*it_next)->GetBaseAddr()
          );
          GetProxyAt(*it)->RegisterPriority(path._path_id+1,4);

          m_switches_vector[*it]->SetupTpDstMatch(
            GetProxyAt(*it)->GetBaseAddr(),
            GetProxyAt(*it_next)->GetBaseAddr(),
            PROXY_PORT,
            m_outport_map[*it][*it_next]
          );
          m_switches_vector[*it_next]->SetupTpSrcMatch(
            GetProxyAt(*it_next)->GetBaseAddr(),
            GetProxyAt(*it)->GetBaseAddr(),
            PROXY_PORT,
            m_outport_map[*it_next][*it]
          );
        }
        // proxy to destination
        // forward
        GetProxyAt(*it)->RegisterReceiver(
          path._path_id,
          GetHostAddress(*it),
          path._application_port
        );
        GetProxyAt(*it)->RegisterPriority(path._path_id,2);
        SetForwardRuleAt (*it, path._path_id, path._path_id);

        // coding
        GetProxyAt(*it)->RegisterReceiver(
          path._path_id+1,
          GetHostAddress(*it),
          path._application_port
        );
        GetProxyAt(*it)->RegisterPriority(path._path_id+1,4);

        m_switches_vector[*it]->SetupTpDstMatch(
          GetProxyAt(*it)->GetBaseAddr(),
          GetHostAddress(*it),
          path._application_port,
          0
        );
      }
      break;

    case 3:
      {
        // MPLS based path for Steiner trees
        // codecs serve as forwarding nodes
        // establish hop-by-hop connection
        // source to proxy

        // coding
        GetProxyAt(*it)->RegisterSender(path._path_id+1);
        GetProxyAt(*it)->RegisterPriority(path._path_id+1,4);

        m_switches_vector[*it]->SetupTpSrcMatch(
          GetProxyAt(*it)->GetBaseAddr(),
          GetHostAddress(*it),
          PROXY_PORT,
          0
        );

        for(; it_next != path._nodes.end(); ++it, ++it_next)
        {
          // proxy to proxy

          // coding
          GetProxyAt(*it)->RegisterProxyAddr(
            path._path_id+1, GetProxyAt(*it_next)->GetBaseAddr()
          );
          GetProxyAt(*it)->RegisterPriority(path._path_id+1,4);

          m_switches_vector[*it]->SetupTpDstMatch(
            GetProxyAt(*it)->GetBaseAddr(),
            GetProxyAt(*it_next)->GetBaseAddr(),
            PROXY_PORT,
            m_outport_map[*it][*it_next]
          );
          m_switches_vector[*it_next]->SetupTpSrcMatch(
            GetProxyAt(*it_next)->GetBaseAddr(),
            GetProxyAt(*it)->GetBaseAddr(),
            PROXY_PORT,
            m_outport_map[*it_next][*it]
          );
        }
        // proxy to destination
        // coding
        GetProxyAt(*it)->RegisterReceiver(
          path._path_id+1,
          GetHostAddress(*it),
          path._application_port
        );
        GetProxyAt(*it)->RegisterPriority(path._path_id+1,4);

        m_switches_vector[*it]->SetupTpDstMatch(
          GetProxyAt(*it)->GetBaseAddr(),
          GetHostAddress(*it),
          path._application_port,
          0
        );
      }
      break;

    case 2:
      {
        // MPLS based path
        // simple forwarding
        // establish hop-by-hop connection
        // source to proxy
        GetProxyAt(*it)->RegisterSender(path._path_id);
        GetProxyAt(*it)->RegisterPriority(path._path_id,2);
        m_switches_vector[*it]->SetupTpSrcMatch(
          GetProxyAt(*it)->GetBaseAddr(),
          GetHostAddress(*it),
          PROXY_PORT,
          0
        );
        SetForwardRuleAt (*it, path._path_id, path._path_id);
        for(; it_next != path._nodes.end(); ++it, ++it_next)
        {
          // proxy to proxy
          GetProxyAt(*it)->RegisterProxyAddr(
            path._path_id, GetProxyAt(*it_next)->GetBaseAddr()
          );
          GetProxyAt(*it)->RegisterPriority(path._path_id,2);
          m_switches_vector[*it]->SetupTpDstMatch(
            GetProxyAt(*it)->GetBaseAddr(),
            GetProxyAt(*it_next)->GetBaseAddr(),
            PROXY_PORT,
            m_outport_map[*it][*it_next]
          );
          SetForwardRuleAt (*it, path._path_id, path._path_id);
          m_switches_vector[*it_next]->SetupTpSrcMatch(
            GetProxyAt(*it_next)->GetBaseAddr(),
            GetProxyAt(*it)->GetBaseAddr(),
            PROXY_PORT,
            m_outport_map[*it_next][*it]
          );
        }
        // proxy to destination
        GetProxyAt(*it)->RegisterReceiver(
          path._path_id,
          GetHostAddress(*it),
          path._application_port
        );
        GetProxyAt(*it)->RegisterPriority(path._path_id,2);
        m_switches_vector[*it]->SetupTpDstMatch(
          GetProxyAt(*it)->GetBaseAddr(),
          GetHostAddress(*it),
          path._application_port,
          0
        );
        SetForwardRuleAt (*it, path._path_id, path._path_id);
      }
      break;
  }
} // OpenFlowRoutingController::EstablishPath

void
OpenFlowRoutingController::EstablishPath (Ptr<CodedBulkUnicastPath>& path, uint8_t encode)
{
  EstablishPath(*path,encode);
} // OpenFlowRoutingController::EstablishPath

void
OpenFlowRoutingController::AddCodedBulkEncoderAt (VirtualLink* encode_link, int node)
{
  if( (node >= 0) && (node < (int)m_switches_vector.size()) ) {
    m_switches_vector[node]->GetCodecManager()->addCodedBulkCodec(encode_link);
  }
}

void
OpenFlowRoutingController::AddCodedBulkDecoderAt (VirtualLink* decode_basis_map, int node)
{
  if( (node >= 0) && (node < (int)m_switches_vector.size()) ) {
    decode_basis_map->inverse();
    m_switches_vector[node]->GetCodecManager()->addCodedBulkCodec(decode_basis_map);
  }
}

uint64_t
OpenFlowRoutingController::GetTotalProxyOccupation (void) const
{
  uint64_t total_bytes = 0;
  for(std::vector<Ptr<OpenFlowRoutingSwitch> >::const_iterator
    it  = m_switches_vector.begin();
    it != m_switches_vector.end();
    ++it
  ) {
    total_bytes += (*it)->GetProxy()->GetBufferedBytes ();
  }
  return total_bytes;
}

/*
void
OpenFlowRoutingController::SetupMatch (
  Ptr<OpenFlowSwitchNetDevice> swtch,
  uint32_t nw_src, uint32_t nw_dst, uint16_t tp_dst,
  uint16_t out_port
){
  NS_LOG_FUNCTION_NOARGS ();

  // Create matching key.
  sw_flow_key key;
  key.wildcards = htonl(OFPFW_ALL ^ OFPFW_TP_DST ^ 
    OFPFW_NW_SRC_MASK ^ OFPFW_NW_SRC_ALL ^ 
    OFPFW_NW_DST_MASK ^ OFPFW_NW_DST_ALL ^ 
    OFPFW_NW_PROTO ^ OFPFW_DL_TYPE);
  //key.wildcards = htonl(0b1100000000000001001111);
  key.flow.dl_type = htons(Ipv4L3Protocol::PROT_NUMBER); //htons(ETH_TYPE_IP);
  key.flow.nw_proto = TcpL4Protocol::PROT_NUMBER; //IPPROTO_TCP;
  key.flow.nw_src = htonl(nw_src);
  key.flow.nw_dst = htonl(nw_dst);
  key.flow.tp_dst = htons(tp_dst);
  
  // Create output-to-port action
  ofp_action_output x[1];                              // allocate a memory for action
  x[0].type = htons (OFPAT_OUTPUT);
  x[0].len  = htons (sizeof(ofp_action_output));
  x[0].port = out_port;

  size_t x_size = sizeof(x);
  ofp_flow_mod* ofm = BuildFlow (key, (uint32_t) -1, OFPFC_ADD, x, x_size, OFP_FLOW_PERMANENT, OFP_FLOW_PERMANENT);
  SendToSwitch(swtch, ofm, ofm->header.length);
} // OpenFlowRoutingController::SetupMatch
*/
}

}

#endif // NS3_OPENFLOW