/***********
 * Test proxy local connection
 ***********/
#include "tcp-proxy.h"
#include <iostream>
using namespace std;

int main(int argc, char* argv[]){
  Ptr<TcpProxy> proxy = Create<TcpProxy> ();
  Address local_addr;
  IpAddressPtoN ("10.12.23.35", local_addr);
  //IpAddressPtoN ("128.84.155.87", local_addr);
  proxy->SetBaseAddr (local_addr);

  Address remote_addr;
  IpAddressPtoN ("10.12.23.34", remote_addr);
  proxy->RegisterReceiver (1234, remote_addr, 1000);
  proxy->RegisterReceiver (1235, remote_addr, 1000);

  proxy->StartProxy ();

  // pause the main thread here
  for(;;){}

  return 0;
}