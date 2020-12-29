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
#include "openflow-routing-controller.h"

OpenFlowRoutingController::OpenFlowRoutingController()
{
  m_switch_address_vector.clear();
  m_host_info_map.clear();
} // OpenFlowRoutingController::OpenFlowRoutingController

OpenFlowRoutingController::~OpenFlowRoutingController()
{
  for(std::map<int, std::list<VirtualLink*> >::iterator
    it_map  = m_all_codecs.begin();
    it_map != m_all_codecs.end(); ++it_map
  ) {
    for(std::list<VirtualLink*>::iterator
      it  = it_map->second.begin();
      it != it_map->second.end(); ++it
    ) {
      delete *it;
    }
  }
} // OpenFlowRoutingController::~OpenFlowRoutingController

void
OpenFlowRoutingController::InitializeOutputMap (void)
{
/*
  int total_switches = m_switch_address_vector.size();
  m_outport_map.resize(total_switches);
  m_outport_counter.resize(total_switches);
  for(int n = 0; n < total_switches; ++n) {
    m_outport_map[n].clear();
    m_outport_counter[n] = 0;
  }
*/
}

void
OpenFlowRoutingController::RegisterHostAt (int node)
{
  m_host_info_map[node] = Create<HostInfo> ();
  ++m_outport_counter[node];
}

void
OpenFlowRoutingController::RegisterHostAddress (int node, IPCAddress address)
{
  if( m_host_info_map.find(node) != m_host_info_map.end() ) {
    m_host_info_map[node]->m_address = address;
  }
}

IPCAddress&
OpenFlowRoutingController::GetHostAddress (int node)
{
  return m_host_info_map[node]->m_address;
}

/*
Ptr<TcpProxy>
OpenFlowRoutingController::GetProxyAt (int node)
{
  return m_switch_address_vector[node]->GetProxy();
}
*/

void
OpenFlowRoutingController::SetForwardRuleAt (const int node, const uint32_t from_path_id, const uint32_t to_path_id)
{
  //GetProxyAt(node)->SetForwardRule (from_path_id, to_path_id);
}

void
OpenFlowRoutingController::RegisterLink (int node_a, int node_b)
{
  // it is a directed link
  //m_outport_map[node_a][node_b] = m_outport_counter[node_a]++;
}

void
OpenFlowRoutingController::EstablishPath (CodedBulkUnicastPath& path, bool encode)
{
  /*
  if( path._nodes.empty() ) {
    return;
  }

  std::list<int>::iterator it = path._nodes.begin();
  std::list<int>::iterator it_next = path._nodes.begin(); 
  ++it_next;
  */
/*
  //if(encode) {
    // MPLS based path
    // establish hop-by-hop connection
    // source to proxy
    GetProxyAt(*it)->AddLocal(path._path_id, ture);
    m_switch_address_vector[*it]->SetupTpSrcMatch(
      GetProxyAt(*it)->GetBaseAddr(),
      GetHostAddress(*it),
      path._path_id,
      0
    );
    if(!encode) {
      // simple forwarding
      SetForwardRuleAt (*it, path._path_id, path._path_id);
    }
    for(; it_next != path._nodes.end(); ++it, ++it_next)
    {
      // proxy to proxy
      GetProxyAt(*it)->RegisterProxy(
        path._path_id, GetProxyAt(*it_next)
      );
      m_switch_address_vector[*it]->SetupTpDstMatch(
        GetProxyAt(*it)->GetBaseAddr(),
        GetProxyAt(*it_next)->GetBaseAddr(),
        path._path_id,
        m_outport_map[*it][*it_next]
      );
      if(!encode) {
        // simple forwarding
        SetForwardRuleAt (*it, path._path_id, path._path_id);
      }
      m_switch_address_vector[*it_next]->SetupTpSrcMatch(
        GetProxyAt(*it_next)->GetBaseAddr(),
        GetProxyAt(*it)->GetBaseAddr(),
        path._path_id,
        m_outport_map[*it_next][*it]
      );
    }
    // proxy to destination
    GetProxyAt(*it)->RegisterReceiverAddr(
      path._path_id, GetHostAddress(*it)
    );
    m_switch_address_vector[*it]->SetupTpDstMatch(
      GetProxyAt(*it)->GetBaseAddr(),
      GetHostAddress(*it),
      path._path_id,
      0
    );
    if(!encode) {
      // simple forwarding
      SetForwardRuleAt (*it, path._path_id, path._path_id);
    }
*/
} // OpenFlowRoutingController::EstablishPath

void
OpenFlowRoutingController::EstablishPath (Ptr<CodedBulkUnicastPath>& path, bool is_CodedBulk_path)
{
  //EstablishPath(*path,is_CodedBulk_path);
} // OpenFlowRoutingController::EstablishPath

void
OpenFlowRoutingController::AddCodedBulkEncoderAt (VirtualLink* encode_map, int node)
{
  if(m_all_codecs.find(node) == m_all_codecs.end()) {
    m_all_codecs[node].clear();
  }
  m_all_codecs[node].push_back(encode_map);
/*
  if( (node >= 0) && (node < (int)m_switch_address_vector.size()) ) {
    m_switch_address_vector[node]->GetCodecManager()->addCodedBulkCodec(encode_map);
  }
*/
}

void
OpenFlowRoutingController::AddCodedBulkDecoderAt (VirtualLink* decode_basis_map, int node)
{
  if(m_all_codecs.find(node) == m_all_codecs.end()) {
    m_all_codecs[node].clear();
  }
  decode_basis_map->inverse();
  m_all_codecs[node].push_back(decode_basis_map);
/*
  if( (node >= 0) && (node < (int)m_switch_address_vector.size()) ) {
    decode_basis_map->inverse();
    m_switch_address_vector[node]->GetCodecManager()->addCodedBulkCodec(decode_basis_map);
  }
*/
}
