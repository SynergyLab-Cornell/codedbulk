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
#include "settings/fig/fig_main_measure.h"

#include "settings/topology/one_link.cc_part"

// testing coding
NS_LOG_COMPONENT_DEFINE ("OneLinkTest");

#undef TOTAL_EXPERIMENTS
#define TOTAL_EXPERIMENTS 2

#define USER_DEFINED_TRAFFIC() \
  Ptr<CodedBulkTraffic> traffic = traffic_manager->addCodedBulkTraffic(map_host_to_sw[0]);\
  traffic->addDst(map_host_to_sw[1]);\
  traffic->SetPriority(CodedBulkTraffic::Bulk);\
  traffic_at_source[map_host_to_sw[0]] = traffic;

int 
main (int argc, char *argv[])
{
  double APP_STOP_TIME = 5.1;

  std::string sim_name = "One-Link-Test";
  std::stringstream sim_serial;
  sim_serial.str("");

  #include "settings/fig/fig_file.cc_part"

  #include "settings/traffic/traffic_setup.cc_part"

  int TOTAL_NUM_TRAFFIC = 1;
  int total_num_destinations = 1;
  std::cout << "Total number of multicast traffic = " << TOTAL_NUM_TRAFFIC << std::endl;

  #include "settings/fig/fig_main.cc_part"
  return 0;
}
