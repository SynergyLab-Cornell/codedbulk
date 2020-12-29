/***********
 * Test CodedBulkReceiver
 ***********/
#include "CodedBulk-receiver.h"
#include <iostream>
using namespace std;

int main(int argc, char* argv[]){
  Address local_address;
  IpAddressPtoN ("10.12.23.34", local_address);

  Ptr<CodedBulkReceiver> receiver = Create<CodedBulkReceiver> ();
  receiver->SetApplicationID(1001);
  receiver->SetLocal(local_address);
  receiver->SetThroughProxy(true);

  receiver->StartApplication();

  // halt the main program here
  for(;;) {}

  return 0;
}