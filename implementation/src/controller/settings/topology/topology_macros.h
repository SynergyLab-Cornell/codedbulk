/* Author:  Shih-Hao Tseng (st688@cornell.edu) */
#ifndef __TOPOLOGY_MACROS__
#define __TOPOLOGY_MACROS__

#define REG_SW_ADDR(A,B)\
CodedBulk_controller->RegisterProxyAddress(A,B);

#define DIRECTED_LINK_SW_TO_SW(A,B,C)\
controller->RegisterLink(A,B);\
graph->addEdge(A,B,true)._capacity = C;

#define LINK_SW_TO_SW(A,B,C)\
controller->RegisterLink(A,B);\
graph->addEdge(A,B,true)._capacity = C;\
controller->RegisterLink(B,A);\
graph->addEdge(B,A,true)._capacity = C;

#define DIRECTED_LINK_SW_TO_SW_IP(A,B,C,D,E)\
controller->RegisterLink(A,C);\
REG_SW_ADDR(A,B)\
{CodedBulkEdge& e = graph->addEdge(A,C,true);\
e._use_edge_ip = true;\
e._node_head_ip = B;\
e._node_tail_ip = D;\
e._capacity = E;}

#define LINK_SW_TO_SW_IP(A,B,C,D,E)\
DIRECTED_LINK_SW_TO_SW_IP(A,B,C,D,E)\
DIRECTED_LINK_SW_TO_SW_IP(C,D,A,B,E)

#define COMMA ,

#define LIST_OF_PUBLIC_ADDRESSES(ID,ADDRESSES)\
std::string public_addr_##ID[] = { ADDRESSES };\
std::string* public_addr_ptr_##ID = public_addr_##ID;

#define LIST_OF_PRIVATE_ADDRESSES(ID,ADDRESSES)\
std::string private_addr_##ID[] = { ADDRESSES };\
std::string* private_addr_ptr_##ID = private_addr_##ID;

#define PRIVATE_ADDRESSES_IS_THE_SAME_AS_PUBLIC(ID) \
std::string* private_addr_##ID = public_addr_##ID;\
std::string* private_addr_ptr_##ID = public_addr_##ID;

#define AUTO_REG_SW_ADDR(A)\
CodedBulk_controller->RegisterProxyAddress(A,private_addr_##A[0]);

#define AUTO_DIRECT_LINK_SW_TO_SW_IP(A,C,E)\
controller->RegisterLink(A,C);\
CodedBulk_controller->RegisterProxyAddress(A,*private_addr_ptr_##A);\
{CodedBulkEdge& e = graph->addEdge(A,C,true);\
e._use_edge_ip = true;\
e._node_head_ip = *private_addr_ptr_##A;\
++private_addr_ptr_##A;\
e._node_tail_ip = *public_addr_ptr_##C;\
++public_addr_ptr_##C;\
e._capacity = E;}

#define AUTO_SW_TO_SW_IP(A,C,E)\
AUTO_DIRECT_LINK_SW_TO_SW_IP(A,C,E)\
AUTO_DIRECT_LINK_SW_TO_SW_IP(C,A,E)

#endif  // __TOPOLOGY_MACROS__