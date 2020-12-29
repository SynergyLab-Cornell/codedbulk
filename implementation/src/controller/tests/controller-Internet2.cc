/***********
 * CodedBulkController - Internet2
 ***********/
#include "../settings/topology/topology_macros.h"
#include "CodedBulk-controller.h"
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char* argv[]){
  CodedBulkController::__simualtion_name="Internet2";
  #include "../settings/controller_setup.cc_part"

  #include "../settings/topology/Internet2.cc_part"

  traffic_manager->clearAllCodedBulkTraffic ();
  Ptr<CodedBulkTraffic> traffic = traffic_manager->addCodedBulkTraffic(0);
  traffic->SetPriority(CodedBulkTraffic::Bulk);
  traffic->addDst(5);
  traffic->addDst(6);
  traffic->ApplyCodedBulk(true);
  traffic->SetUnicast(false);

  CodedBulk_controller->GenerateAllFiles();

  return 0;
}