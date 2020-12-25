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
#include "settings/common_settings.h"
#include "settings/topology/butterfly.cc_part"

// testing coding
NS_LOG_COMPONENT_DEFINE ("ButterflyExample");

int 
main (int argc, char *argv[])
{
  double APP_STOP_TIME = 0.0;

  #include "settings/traffic/traffic_setup.cc_part"

  ResetMeasureVariables ();

  #include "settings/topology/topology_creation.cc_part"
  GENERATE_TOPOLOGY()
  #include "settings/topology/topology_setup_ipv4.cc_part"

  traffic_manager->clearAllCodedBulkTraffic ();
  Ptr<CodedBulkTraffic> traffic = traffic_manager->addCodedBulkTraffic(0);
  traffic->addDst(5);
  traffic->addDst(6);
  traffic->SetSteinerTree(true);
  //traffic->ApplyCodedBulk(true);
  //traffic->SetUnicast(false);

  // using Steiner tree algorithm
  CodedBulk_controller->SetRoutingAlgorithm (Create<CodedBulkWatel2014SteinerRouting> ());
  CodedBulk_controller->InitializeAlgorithms();

  #include "settings/controller_setup_and_run.cc_part"

  #include "settings/path_codec_forward.cc_part"

  return 0;
}
