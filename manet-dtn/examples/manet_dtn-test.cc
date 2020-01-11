/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 *
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
 * This is an example script for AODV manet routing protocol. 
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include <iostream>
#include <cmath>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/yans-wifi-helper.h"
//for use UDP
#include "ns3/udp-client-server-helper.h"
/*for trace SUMO Mobility*/
#include "ns3/ns2-mobility-helper.h"
/*for use Netanim*/
#include "ns3/netanim-module.h"
/*for OLSR*/
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/manet_dtn-helper.h"
#include <fstream>
#include <string>
#include <vector>
#include "ns3/flow-monitor-module.h"
#include "ns3/applications-module.h"
using namespace ns3;


class ManetDtnExample 
{
public:
  ManetDtnExample ();
  /**
   * \brief Configure script parameters
   * \param argc is the command line argument count
   * \param argv is the command line arguments
   * \return true on successful configuration
  */
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /**
   * Report results
   * \param os the output stream
   */
  void Report (std::ostream & os);

private:

  // parameters
  /// Number of nodes
  uint32_t size;
  /// Distance between nodes, meters
  int seed;
  double step;
  uint32_t width;
  bool flag;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;

  // network
  /// nodes used in the example
  NodeContainer nodes;
  /// devices used in the example
  NetDeviceContainer devices;
  /// interfaces used in the example
  Ipv4InterfaceContainer interfaces;

private:
  /// Create the nodes
  void CreateNodes ();
  /// Create the devices
  void CreateDevices ();
  /// Create the network
  void InstallInternetStack ();
  /// Create the simulation applications
  void InstallApplications ();
};

int main (int argc, char **argv)
{
  ManetDtnExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();
  test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
ManetDtnExample::ManetDtnExample () :
  size (4),
  seed (10),
  step (50),
  width (2),
  flag (false),
  totalTime (150),
  pcap (true),
  printRoutes (true)
{
}

bool
ManetDtnExample::Configure (int argc, char **argv)
{

  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("width", "Grid step, m", width);
  cmd.AddValue ("seed", "Random", seed);
  cmd.AddValue ("flag", "Random", flag);
  cmd.Parse (argc, argv);
  SeedManager::SetSeed (seed);
  return true;
}

void
ManetDtnExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  Simulator::Stop (Seconds (totalTime));
  AnimationInterface anim("manet_dtn-test.xml");
  Simulator::Run ();
  Simulator::Destroy ();
}

void
ManetDtnExample::Report (std::ostream &)
{ 
}

void
ManetDtnExample::CreateNodes ()
{
  LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
  LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
  if(flag == true){
  //  LogComponentEnable("UdpClient", LOG_LEVEL_FUNCTION);
    LogComponentEnable("ManetDtnRoutingProtocol",LOG_LEVEL_ALL);
  //  LogComponentEnable("Ipv4StaticRouting",LOG_LEVEL_FUNCTION);
  //  LogComponentEnable("Ipv4ListRouting",LOG_LEVEL_ALL);
  //  LogComponentEnable("Ipv4L3Protocol",LOG_LEVEL_ALL);
  }
 nodes.Create (size);
MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (50),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (50),
                                 "DeltaY", DoubleValue (50),
                                 "GridWidth", UintegerValue (width),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

}

void
ManetDtnExample::CreateDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes); 

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("manet_dtn"));
    }
}

void
ManetDtnExample::InstallInternetStack ()
{
    
  InternetStackHelper internet;
 // Ipv4StaticRoutingHelper staticRouting;
  ManetDtnHelper manet_dtn;
  //Ipv4ListRoutingHelper list;
 // list.Add(staticRouting,0);
 //list.Add(manet_dtn,10);
  internet.SetRoutingHelper(manet_dtn);
  internet.Install(nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("manet_dtn.routes", std::ios::out);
      manet_dtn.PrintRoutingTableAllAt (Seconds (20), routingStream);
    }
}

void
ManetDtnExample::InstallApplications ()
{

//Server
    uint16_t port = 4000;
    UdpServerHelper server(port);
    ApplicationContainer serverApps = server.Install(nodes.Get(0));
    serverApps.Start(Seconds(0));
    serverApps.Stop(Seconds(150));
//Client
    UdpClientHelper client(interfaces.GetAddress(0),port);
    uint32_t MaxPacketSize = 1024;
    Time interPacketInterval = Seconds(1);
    uint32_t maxPacketCount = 1;
    client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client.SetAttribute("Interval", TimeValue(interPacketInterval));
    client.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));

    ApplicationContainer clientApps = client.Install (nodes.Get (size-1));
    clientApps.Start(Seconds(20));
    clientApps.Stop(Seconds(100));
    
}

