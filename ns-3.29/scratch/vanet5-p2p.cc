
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/olsr-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-bsm-helper.h"
#include "ns3/wave-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/rng-seed-manager.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyVanet1");




static inline std::string
PrintReceivedRoutingPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address srcAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (srcAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (srcAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address srcAddress;
  while ((packet = socket->RecvFrom (srcAddress))){
    std::cout << PrintReceivedRoutingPacket (socket, packet, srcAddress) << "\n";
  }
}

int main (int argc, char *argv[])
{
	std::string phyMode ("OfdmRate6MbpsBW10MHz");
	uint32_t packetSize = 1024;
	double intervalTime = 10.0;
	uint32_t totalTime = 20;
	uint32_t numvehicleNodes1 = 10;
  uint32_t numvehicleNodes2 = 10;
  uint32_t numvehicleNodes3 = 10;
  uint32_t nump2pNodes = 2;
  uint32_t maxPacketsCount = 1;
  double maxRange = 100.0;
	Address serverAddress1;
  Address serverAddress2;
  Address serverAddress3;
  Address serverAddress4;


  RngSeedManager seed;
  seed.SetSeed(1);


  Time interval = Seconds (intervalTime);


  uint32_t scenario = 1;
  // 1はすべて先頭が送る、2はVANETの最後尾が次のVANETへ送る

	LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
  LogComponentEnable ("MyVanet1", LOG_LEVEL_INFO);
  //LogComponentEnable ("OlsrRoutingProtocol", LOG_LEVEL_INFO);


  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
 	// turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));


 

  //node create
	NodeContainer vehicleNodes1;
	vehicleNodes1.Create (numvehicleNodes1);

  NodeContainer vehicleNodes2;
  vehicleNodes2.Create (numvehicleNodes2);

  NodeContainer vehicleNodes3;
  vehicleNodes3.Create (numvehicleNodes3);

	NodeContainer p2pNodes;
	p2pNodes.Create (nump2pNodes);
  

  //wifi設定

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  //LossModel
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (maxRange));
  //wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  //wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel", "Frequency", DoubleValue (freq));


  Ptr<YansWifiChannel> channel = wifiChannel.Create ();

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (channel);
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
  
  YansWavePhyHelper wavePhy =  YansWavePhyHelper::Default ();
  wavePhy.SetChannel (channel);
  wavePhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
   

  NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  WaveHelper waveHelper = WaveHelper::Default ();
  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();

  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");

  QosWaveMacHelper waveMac = QosWaveMacHelper::Default ();

  //devices install
  NetDeviceContainer devices = wifi80211p.Install (wifiPhy, wifi80211pMac, vehicleNodes1);
  NetDeviceContainer devices2 = wifi80211p.Install (wifiPhy, wifi80211pMac, vehicleNodes2);
  NetDeviceContainer devices3 = wifi80211p.Install (wifiPhy, wifi80211pMac, vehicleNodes3);




  //p2p設定
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate",StringValue ("20Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pdevices, p2pdevices2, p2pdevices3, p2pdevices4;

  if(scenario == 1){

    p2pdevices = p2p.Install (vehicleNodes1.Get (0), p2pNodes.Get (0));
    p2pdevices2 = p2p.Install (p2pNodes.Get (0), vehicleNodes2.Get (0));
    p2pdevices3 = p2p.Install (vehicleNodes1.Get (0), p2pNodes.Get (1));
    p2pdevices4 = p2p.Install (p2pNodes.Get (1), vehicleNodes3.Get (0));

  }else if(scenario == 2){

    p2pdevices = p2p.Install (vehicleNodes1.Get (numvehicleNodes1 - 1), p2pNodes.Get (0));
    p2pdevices2 = p2p.Install (p2pNodes.Get (0), vehicleNodes2.Get (0));
    p2pdevices3 = p2p.Install (vehicleNodes2.Get (numvehicleNodes2 - 1), p2pNodes.Get (1));
    p2pdevices4 = p2p.Install (p2pNodes.Get (1), vehicleNodes3.Get (0));

  }

  	//std::cout << "Creating point-to-pointnodes.\n"


//olsr設定
  OlsrHelper olsr;
  //olsr使わないノード・インターフェイス設定

  olsr.ExcludeInterface (p2pNodes.Get (0), 2);
  olsr.ExcludeInterface (p2pNodes.Get (1), 2);


  if(scenario == 1){

    olsr.ExcludeInterface (vehicleNodes1.Get (0), 2);
    olsr.ExcludeInterface (vehicleNodes1.Get (0), 3);
    olsr.ExcludeInterface (vehicleNodes2.Get (0), 2);
    olsr.ExcludeInterface (vehicleNodes3.Get (0), 2);

  }else if(scenario == 2){

    olsr.ExcludeInterface (vehicleNodes1.Get (numvehicleNodes1 - 1), 2);
    olsr.ExcludeInterface (vehicleNodes2.Get (0), 2);
    olsr.ExcludeInterface (vehicleNodes2.Get (numvehicleNodes2 - 1), 2);
    olsr.ExcludeInterface (vehicleNodes3.Get (0), 2);

  } 

/*//////////////////////////////////
ExcludeInterface(node,uint32_t interface);
interfaceはノードにインストールされているインターフェイスから
弱い順にソートされているらしい
point-to-pointは2
*///////////////////////////////////


//mobility
  MobilityHelper p2pmobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (500.0, 200.0, 0.0));
  positionAlloc->Add (Vector (1000.0, 200.0, 0.0));
	p2pmobility.SetPositionAllocator (positionAlloc);
 	p2pmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
 	p2pmobility.Install (p2pNodes);

//仮配置
////////////////////////////////////////////////////////////////////////
 	MobilityHelper vehiclemobility1;
 	vehiclemobility1.SetPositionAllocator ("ns3::GridPositionAllocator",
                              	         "MinX", DoubleValue (0.0),
                                         "MinY", DoubleValue (0.0),
                                         "DeltaX", DoubleValue (50.0),
                                         "DeltaY", DoubleValue (5.0),
                                         "GridWidth", UintegerValue (5),
                                         "LayoutType", StringValue ("RowFirst"));
 	vehiclemobility1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  vehiclemobility1.Install (vehicleNodes1);

  MobilityHelper vehiclemobility2;
  vehiclemobility2.SetPositionAllocator ("ns3::GridPositionAllocator",
                                         "MinX", DoubleValue (500.0),
                                         "MinY", DoubleValue (0.0),
                                         "DeltaX", DoubleValue (50.0),
                                         "DeltaY", DoubleValue (5.0),
                                         "GridWidth", UintegerValue (5),
                                         "LayoutType", StringValue ("RowFirst"));

  vehiclemobility2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  vehiclemobility2.Install (vehicleNodes2);


  MobilityHelper vehiclemobility3;
  vehiclemobility3.SetPositionAllocator ("ns3::GridPositionAllocator",
                                         "MinX", DoubleValue (1000.0),
                                         "MinY", DoubleValue (0.0),
                                         "DeltaX", DoubleValue (50.0),
                                         "DeltaY", DoubleValue (5.0),
                                         "GridWidth", UintegerValue (5),
                                         "LayoutType", StringValue ("RowFirst"));
  vehiclemobility3.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  vehiclemobility3.Install (vehicleNodes3);


/*///////////////////////////////////////////////////////////////////////

    MobilityHelper vehiclemobility2 = Ns2MobilityHelper ("/home/fk/sumo/test1mobility.tcl");
    vehiclemobility2.Install(vehicleNodes);
*////////////////////////////////////////////////////////////////////////


//Ipv4
 	//NS_LOG_INFO ("Assign IP Addresses.");

  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper list;
	list.Add (staticRouting, 0);
  list.Add (olsr, 10); //優先度

  InternetStackHelper internet_olsr;
	internet_olsr.SetRoutingHelper (list);
	internet_olsr.Install (vehicleNodes1);

  Ipv4StaticRoutingHelper staticRouting2;
  Ipv4ListRoutingHelper list2;
  list2.Add (staticRouting2, 0);
  list2.Add (olsr,10); //優先度

  InternetStackHelper internet_olsr2;
  internet_olsr2.SetRoutingHelper (list2); ////////////////////////////////
  internet_olsr2.Install (vehicleNodes2);

  Ipv4StaticRoutingHelper staticRouting3;
  Ipv4ListRoutingHelper list3;
  list3.Add (staticRouting3, 0);
  list3.Add (olsr,10); //優先度

  InternetStackHelper internet_olsr3;
  internet_olsr3.SetRoutingHelper (list3);
  internet_olsr3.Install (vehicleNodes3);

	InternetStackHelper internet_csma;
 	internet_csma.Install (p2pNodes);


 	//olsr
  Ipv4AddressHelper olsrAddress;
  olsrAddress.SetBase ("11.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer olsrInterface1;
  olsrInterface1 = olsrAddress.Assign (devices);

  Ipv4AddressHelper olsrAddress2;
  olsrAddress2.SetBase ("12.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer olsrInterface2;
  olsrInterface2 = olsrAddress2.Assign (devices2);

  Ipv4AddressHelper olsrAddress3;
  olsrAddress3.SetBase ("13.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer olsrInterface3;
  olsrInterface3 = olsrAddress3.Assign (devices3);


  //p2p
 	Ipv4AddressHelper p2pAddress;
 	p2pAddress.SetBase ("172.1.1.0", "255.255.255.0");

  Ipv4AddressHelper p2pAddress2;
  p2pAddress2.SetBase ("172.2.1.0", "255.255.255.0");

  Ipv4AddressHelper p2pAddress3;
  p2pAddress3.SetBase ("172.3.1.0", "255.255.255.0");

  Ipv4AddressHelper p2pAddress4;
  p2pAddress4.SetBase ("172.4.1.0", "255.255.255.0");


  Ipv4InterfaceContainer p2pInterface1, p2pInterface2, p2pInterface3, p2pInterface4;

  //v1-0 -> p-0 || v1-e -> p-0
  p2pInterface1 = p2pAddress.Assign (p2pdevices);
  serverAddress1 = Address (p2pInterface1.GetAddress(1) );

  //p-0 -> v2-0
  p2pInterface2 = p2pAddress2.Assign (p2pdevices2);
  serverAddress2 = Address (p2pInterface2.GetAddress(1) );

  //v1-0 -> p-1 || v2-e -> p-1 
  p2pInterface3 = p2pAddress3.Assign (p2pdevices3);
  serverAddress3 = Address (p2pInterface3.GetAddress(1) );/////////////////////////////////////////////////

  //p-1 -> v3-0
  p2pInterface4 = p2pAddress4.Assign (p2pdevices4);
  serverAddress4 = Address (p2pInterface4.GetAddress(1) );



	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

//p2p application
  uint32_t port1 = 81; //s1
  uint32_t port2 = 82; //v2
  uint32_t port3 = 83; //s2
  uint32_t port4 = 84; //v3


  if(scenario == 1){
    //v1-0 -> p-0
    UdpClientHelper client1 (serverAddress1, port1);
    client1.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
    client1.SetAttribute ("Interval", TimeValue (interval));
    client1.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client1.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes1.Get(0)->GetId()));
    ApplicationContainer clientApps1 = client1.Install (vehicleNodes1.Get (0));
    clientApps1.Start (Seconds (10.0));
    clientApps1.Stop (Seconds (totalTime));
    //p-0 -> v2-0
    UdpClientHelper client2 (serverAddress2, port2);
    client2.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
    client2.SetAttribute ("Interval", TimeValue (interval));
    client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client2.SetAttribute ("NodeNumber", UintegerValue (p2pNodes.Get(0)->GetId()));
    ApplicationContainer clientApps2 = client2.Install (p2pNodes.Get(0));
    clientApps2.Start (Seconds (11.0)); 
    //clientApps2.Start (Seconds (10.0309095999)); 
    clientApps2.Stop (Seconds (totalTime));
    //v1-0 -> p-1
    UdpClientHelper client3 (serverAddress3, port3);
    client3.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
    client3.SetAttribute ("Interval", TimeValue (interval));
    client3.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client3.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes1.Get(0)->GetId()));
    ApplicationContainer clientApps3 = client3.Install (vehicleNodes1.Get (0));
    clientApps3.Start (Seconds (12.0));
    clientApps3.Stop (Seconds (totalTime));
    //p-1 -> v3-0
    UdpClientHelper client4 (serverAddress4, port4);
    client4.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
    client4.SetAttribute ("Interval", TimeValue (interval));
    client4.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client4.SetAttribute ("NodeNumber", UintegerValue (p2pNodes.Get(1)->GetId()));
    ApplicationContainer clientApps4 = client4.Install (p2pNodes.Get (1));
    clientApps4.Start (Seconds (13.0));
    //clientApps4.Start (Seconds (10.04662885));
    clientApps4.Stop (Seconds (totalTime));

  }else if(scenario == 2){
    //v1-e -> p-0
    UdpClientHelper client1 (serverAddress1, port1);
    client1.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
    client1.SetAttribute ("Interval", TimeValue (interval));
    client1.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client1.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes1.Get(numvehicleNodes1 - 1)->GetId()));
    ApplicationContainer clientApps1 = client1.Install (vehicleNodes1.Get (numvehicleNodes1 - 1));
    clientApps1.Start (Seconds (10.0));
    //clientApps1.Start (Seconds (10.028488));
    clientApps1.Stop (Seconds (totalTime));
    //p-0 -> v2-0
    UdpClientHelper client2 (serverAddress2, port2);
    client2.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
    client2.SetAttribute ("Interval", TimeValue (interval));
    client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client2.SetAttribute ("NodeNumber", UintegerValue (p2pNodes.Get(0)->GetId()));
    ApplicationContainer clientApps2 = client2.Install (p2pNodes.Get(0));
    clientApps2.Start (Seconds (10.0));
    //clientApps2.Start (Seconds (10.0309095999));
    clientApps2.Stop (Seconds (totalTime));    
    //v2-e -> p-1
    UdpClientHelper client3 (serverAddress3, port3);/////////////////////////////////////////////////////////////////////
    client3.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
    client3.SetAttribute ("Interval", TimeValue (interval));
    client3.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client3.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes2.Get(numvehicleNodes2 - 1)->GetId()));
    ApplicationContainer clientApps3 = client3.Install (vehicleNodes2.Get (numvehicleNodes2 - 1));
    clientApps3.Start (Seconds (10.0));
    //clientApps3.Start (Seconds (10.044207245));
    clientApps3.Stop (Seconds (totalTime));
    //p-1 -> v3-0
    UdpClientHelper client4 (serverAddress4, port4);
    client4.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
    client4.SetAttribute ("Interval", TimeValue (interval));
    client4.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client4.SetAttribute ("NodeNumber", UintegerValue (p2pNodes.Get(1)->GetId()));
    ApplicationContainer clientApps4 = client4.Install (p2pNodes.Get (1));
    clientApps4.Start (Seconds (10.0));
    //clientApps4.Start (Seconds (10.04662885));
    clientApps4.Stop (Seconds (totalTime));

  }


  //p0
	UdpServerHelper server (port1);
  server.SetAttribute ("NodeNumber", UintegerValue (p2pNodes.Get(0)->GetId()));
  ApplicationContainer serverApps = server.Install(p2pNodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (totalTime));
  //v2
  UdpServerHelper server2 (port2);
  server2.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes2.Get(0)->GetId()));
  ApplicationContainer serverApps2 = server2.Install(vehicleNodes2.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (totalTime));  
  //p1
  UdpServerHelper server3 (port3);
  server3.SetAttribute ("NodeNumber", UintegerValue (p2pNodes.Get(1)->GetId()));
  ApplicationContainer serverApps3 = server3.Install(p2pNodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (totalTime));  
  //v3
  UdpServerHelper server4 (port4);
  server4.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes3.Get(0)->GetId()));
  ApplicationContainer serverApps4 = server4.Install(vehicleNodes3.Get (0));
  serverApps4.Start (Seconds (1.0));
  serverApps4.Stop (Seconds (totalTime));      



  uint32_t m_port = 90;

//create vanet packets 
  //v1
  UdpClientHelper source1 (Address(olsrInterface1.GetAddress(numvehicleNodes1 - 1)), m_port);
  source1.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
  source1.SetAttribute ("Interval", TimeValue (interval));
  source1.SetAttribute ("PacketSize", UintegerValue (packetSize));
  source1.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes1.Get(0)->GetId()));
  ApplicationContainer sourceApps1 = source1.Install (vehicleNodes1.Get (0));
  sourceApps1.Start (Seconds (10.0));
  sourceApps1.Stop (Seconds (totalTime));
  //v2
  UdpClientHelper source2 (Address(olsrInterface2.GetAddress(numvehicleNodes2 - 1)), m_port);
  source2.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
  source2.SetAttribute ("Interval", TimeValue (interval));
  source2.SetAttribute ("PacketSize", UintegerValue (packetSize));
  source2.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes2.Get(0)->GetId()));
  ApplicationContainer sourceApps2 = source2.Install (vehicleNodes2.Get (0));
  sourceApps2.Start (Seconds (10.0));
  //sourceApps2.Start (Seconds (10.0333312));
  sourceApps2.Stop (Seconds (totalTime));
  //v3
  UdpClientHelper source3 (Address(olsrInterface3.GetAddress(numvehicleNodes3 - 1)), m_port);
  source3.SetAttribute ("MaxPackets", UintegerValue (maxPacketsCount));
  source3.SetAttribute ("Interval", TimeValue (interval));
  source3.SetAttribute ("PacketSize", UintegerValue (packetSize));
  source3.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes3.Get(0)->GetId()));
  ApplicationContainer sourceApps3 = source3.Install (vehicleNodes3.Get (0));
  sourceApps3.Start (Seconds (10.0));
  //sourceApps3.Start (Seconds (10.04905045));
  sourceApps3.Stop (Seconds (totalTime));

//sink vanet packets
  //v1
  UdpServerHelper sink1 (m_port);
  sink1.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes1.Get(numvehicleNodes1 - 1 )->GetId()));
  ApplicationContainer sinkApps1 = sink1.Install(vehicleNodes1.Get (numvehicleNodes1 - 1 ));
  sinkApps1.Start (Seconds (1.0));
  sinkApps1.Stop (Seconds (totalTime));  
  //v2
  UdpServerHelper sink2 (m_port);
  sink2.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes2.Get(numvehicleNodes2 - 1 )->GetId()));
  ApplicationContainer sinkApps2 = sink2.Install(vehicleNodes2.Get (numvehicleNodes2 - 1 ));
  sinkApps2.Start (Seconds (1.0));
  sinkApps2.Stop (Seconds (totalTime));
  //v3
  UdpServerHelper sink3 (m_port);
  sink3.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes3.Get(numvehicleNodes3 - 1 )->GetId()));
  ApplicationContainer sinkApps3 = sink3.Install(vehicleNodes3.Get (numvehicleNodes3 - 1 ));
  sinkApps3.Start (Seconds (1.0));
  sinkApps3.Stop (Seconds (totalTime));







/*
    std::cout << "server node address " << p2pInterface.GetAddress (1) 
              << " " << p2pInterface2.GetAddress (0) << "\n";
    std::cout << "vehicle2/p2p node address " << p2pInterface2.GetAddress (1) << "\n";

*/
//node check
/*  
  	for(uint32_t i = 0; i < numvehicleNodes1; i++){

	  	std::cout << "vehicle1 nodes node " << i << " address " 
	  			  << olsrInterface1.GetAddress (i) << "\n";
	}
    for(uint32_t i = 0; i < numvehicleNodes2; i++){

      std::cout << "vehicle2 nodes node " << (i + numvehicleNodes1) << " address " 
            << olsrInterface2.GetAddress (i) << "\n";
  }

    for(uint32_t i = 0; i < numvehicleNodes3; i++){

      std::cout << "vehicle3 nodes node " << (i + numvehicleNodes2) << " address " 
            << olsrInterface3.GetAddress (i) << "\n";
  }
*/
	std::cout << "client1 node address " << p2pInterface1.GetAddress (0) << std::endl;
  std::cout << "server1 node address " << p2pInterface1.GetAddress (1) << std::endl;
  std::cout << "client2 node address " << p2pInterface2.GetAddress (0) << std::endl;
  std::cout << "server2 node address " << p2pInterface2.GetAddress (1) << std::endl;
  std::cout << "create3 node address " << p2pInterface3.GetAddress (0) << std::endl;
  std::cout << "server3 node address " << p2pInterface3.GetAddress (1) << std::endl;
  std::cout << "client4 node address " << p2pInterface4.GetAddress (0) << std::endl;
  std::cout << "server4 node address " << p2pInterface4.GetAddress (1) << std::endl;
/*	
  std::cout << "serveraddress1 " << Ipv4Address::ConvertFrom (serverAddress1) << std::endl;
  std::cout << "serveraddress2 " << Ipv4Address::ConvertFrom (serverAddress2) << std::endl;
  std::cout << "serveraddress3 " << Ipv4Address::ConvertFrom (serverAddress3) << std::endl;
  std::cout << "serveraddress4 " << Ipv4Address::ConvertFrom (serverAddress4) << std::endl;



	uint32_t num1 = 0;
	for (NodeContainer::Iterator j = vehicleNodes1.Begin (); j != vehicleNodes1.End (); ++j)
    {
       Ptr<Node> object = *j;
       Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
       Vector pos = position->GetPosition ();
       std::cout << "vehicle node" << num1 << " x=" << pos.x << ", y=" << pos.y << std::endl;
       num1++;
     }
 
*/

	Simulator::Stop (Seconds (totalTime));

 	AnimationInterface anim ("vanet3-p2p.xml");

 	NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
	Simulator::Destroy ();
	NS_LOG_INFO ("Done.");



  return 0;

}



