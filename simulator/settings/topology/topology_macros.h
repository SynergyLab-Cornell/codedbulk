/* Author:  Shih-Hao Tseng (st688@cornell.edu) */
#ifndef __TOPOLOGY_MACROS__
#define __TOPOLOGY_MACROS__

#define LINK_HOST_TO_SW(H,S)\
devLink = helperCodedBulkSimple.Install(NodeContainer(host[H],sw[S]));\
devHosts[H] = DynamicCast<SimpleNetDevice>(devLink.Get(0));\
ofs[S]->AddSwitchPort(devLink.Get(1));\
controller->RegisterHostAt(S,devHosts[H]);\
graph->setHostNodeAt(S,host[H]);\
map_host_to_sw[H] = S;

#define LINK_SW_TO_SW(A,B,C)\
devLink = helperCodedBulkSimple.Install(NodeContainer(sw[A],sw[B]));\
ofs[A]->AddSwitchPort(devLink.Get(0));\
controller->RegisterLink(A,B);\
graph->addEdge(A,B,true)._capacity = C;\
graph->_all_edges.back()._net_device = DynamicCast<CodedBulkSimpleNetDevice,NetDevice>(devLink.Get(0));\
ofs[B]->AddSwitchPort(devLink.Get(1));\
controller->RegisterLink(B,A);\
graph->addEdge(B,A,true)._capacity = C;\
graph->_all_edges.back()._net_device = DynamicCast<CodedBulkSimpleNetDevice,NetDevice>(devLink.Get(1));

#define DIRECTED_LINK_SW_TO_SW(A,B,C)\
devLink = helperCodedBulkSimple.Install(NodeContainer(sw[A],sw[B]));\
ofs[A]->AddSwitchPort(devLink.Get(0));\
controller->RegisterLink(A,B);\
graph->addEdge(A,B,true)._capacity = C;\
graph->_all_edges.back()._net_device = DynamicCast<CodedBulkSimpleNetDevice,NetDevice>(devLink.Get(0));\
ofs[B]->AddSwitchPort(devLink.Get(1));

#endif  // __TOPOLOGY_MACROS__