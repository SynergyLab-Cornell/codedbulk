/***********
 * Test CodedBulkBulkSendApplication
 ***********/
#include "CodedBulk-bulk-send-application.h"
#include <iostream>
using namespace std;

int main(int argc, char* argv[]){
  std::list<uint32_t> path_ids;
  path_ids.push_back(1234);
  path_ids.push_back(1235);

  Address address;
  IpAddressPtoN ("10.12.23.34", address);
  std::list<Address> addresses;
  addresses.push_back(address);
  addresses.push_back(address);

  Ptr<CodedBulkBulkSendApplication> sender = Create<CodedBulkBulkSendApplication> ();
  sender->SetApplicationID(1001);
  // create interactive bulk send
  sender->SetPriority(6 << 2);
  // send only 2000 bytes
  sender->SetMaxBytes(2000);
  sender->SetRemoteAddresses(addresses);
  sender->AddMulticastPath(path_ids);
  sender->SetThroughProxy(true);

  sender->StartApplication();

  // halt the main program here
  for(;;) {}

  return 0;
}