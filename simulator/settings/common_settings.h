/* Author:  Shih-Hao Tseng (st688@cornell.edu) */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/openflow-module.h"
#include "ns3/CodedBulk-module.h"
using namespace ns3;

#include "settings/topology/topology_macros.h"
#include "settings/measure_utils.h"

#include <iostream>
#include <fstream>
#include <sstream>

#define APP_START_TIME 0.0
#define SIMULATION_HORIZON 3000.0