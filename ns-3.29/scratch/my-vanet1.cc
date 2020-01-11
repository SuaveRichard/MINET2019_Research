
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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyVanet1");


int main (int argc, char *argv[])
{
	std::string phyMode ("DsssRate1Mbps");
	//double rss = -80;
	uint32_t packetSize = 1024;
	double interval = 1.0;
	uint32_t port = 1000;

	uint32_t totalTime = 10;
	uint32_t numvehicleNodes = 20;
	uint32_t MaxPacketsCount = 320;
	//std::string traceFile = "/home/fk/sumo/test1mobility.tcl";


	Address serverAddress;

	LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
    //LogComponentEnable ("OlsrRoutingProtocol", LOG_LEVEL_INFO);


    //Time interPacketInterval = Seconds (interval);

    // disable fragmentation for frames below 2200 bytes
  	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
 	// turn off RTS/CTS for frames below 2200 bytes
  	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  	// Fix non-unicast data rate to be the same as that of unicast
  	Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

//node create
	NodeContainer vehicleNodes;
	vehicleNodes.Create (numvehicleNodes);

	NodeContainer p2pNode;
	p2pNode.Create (1);


//wifi設定
  	WifiHelper wifi;
  	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();

  	wifiPhy.Set ("RxGain", DoubleValue (0) );
    wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  	YansWifiChannelHelper wifiChannel;
  	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

    wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  	//wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));

  	wifiPhy.SetChannel (wifiChannel.Create ());

  	WifiMacHelper wifiMac;
  	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
    	                          "DataMode",StringValue (phyMode),
        	                      "ControlMode",StringValue (phyMode));

    wifiMac.SetType ("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, vehicleNodes);


//p2p設定
    PointToPointHelper p2p;
  	p2p.SetDeviceAttribute ("DataRate",StringValue ("20Mbps"));
  	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  	NetDeviceContainer p2pdevices;
  	p2pdevices = p2p.Install (vehicleNodes.Get (0), p2pNode.Get (0));

  	//std::cout << "Creating point-to-pointnodes.\n"


//olsr設定
  	OlsrHelper olsr;
    olsr.ExcludeInterface (vehicleNodes.Get (0), 2);

//mobility
  	MobilityHelper p2pmobility;
  	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  	positionAlloc->Add (Vector (200.0, 100.0, 0.0));
  	p2pmobility.SetPositionAllocator (positionAlloc);
  	p2pmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  	p2pmobility.Install (p2pNode);

////////////////////////////////////////////////////////////////////////
  	MobilityHelper vehiclemobility;
  	vehiclemobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 	      "MinX", DoubleValue (0.0),
                                          "MinY", DoubleValue (0.0),
                                          "DeltaX", DoubleValue (50.0),
                                          "DeltaY", DoubleValue (5.0),
                                          "GridWidth", UintegerValue (10),
                                          "LayoutType", StringValue ("RowFirst"));

  	vehiclemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    vehiclemobility.Install (vehicleNodes);
/*///////////////////////////////////////////////////////////////////////

    MobilityHelper vehiclemobility2 = Ns2MobilityHelper ("/home/fk/sumo/test1mobility.tcl");
    vehiclemobility2.Install(vehicleNodes);
*////////////////////////////////////////////////////////////////////////


//Ipv4
 	NS_LOG_INFO ("Assign IP Addresses.");

    Ipv4StaticRoutingHelper staticRouting;
    Ipv4ListRoutingHelper list;
  	list.Add (staticRouting, 0);
  	list.Add (olsr, 10);

	InternetStackHelper internet_olsr;
  	internet_olsr.SetRoutingHelper (list);
  	internet_olsr.Install (vehicleNodes);

  	InternetStackHelper internet_csma;
  	internet_csma.Install (p2pNode);

 	//olsr
  	Ipv4AddressHelper olsrAddress;
  	olsrAddress.SetBase ("10.1.1.0", "255.255.255.0");

  	Ipv4InterfaceContainer olsrInterface;
  	olsrInterface = olsrAddress.Assign (devices);

  	//p2p
 	Ipv4AddressHelper p2pAddress;
 	p2pAddress.SetBase ("172.1.1.0", "255.255.255.0");

  	Ipv4InterfaceContainer p2pInterface = p2pAddress.Assign (p2pdevices);


  	serverAddress = Address (p2pInterface.GetAddress(1) );

/*	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	Ptr<Socket> source = Socket::CreateSocket (vehicleNodes.Get (0), tid);
	InetSocketAddress remote = InetSocketAddress (olsrInterface.GetAddress (0) );
  	source->Connect (remote);
*/


//application

	UdpServerHelper server (port);
  	ApplicationContainer serverApps = server.Install(p2pNode.Get (0));
  	serverApps.Start (Seconds (1.0));
  	serverApps.Stop (Seconds (60.0));  	

  	UdpClientHelper client1 (serverAddress, port);
  	client1.SetAttribute ("MaxPackets", UintegerValue (MaxPacketsCount));
  	client1.SetAttribute ("Interval", TimeValue (Seconds (interval)));
  	client1.SetAttribute ("PacketSize", UintegerValue (packetSize));

  	ApplicationContainer clientApps = client1.Install (vehicleNodes.Get(0));
  	clientApps.Start (Seconds (1.0));
  	clientApps.Stop (Seconds (60.0));



//node check
  	for(uint32_t i = 0; i < numvehicleNodes; i++){

	  	std::cout << "vehicle nodes node " << i << " address " 
	  			  << olsrInterface.GetAddress (i) << "\n";
	}

	std::cout << "server node address " << p2pInterface.GetAddress (0) << "\n";
	std::cout << "vanet/p2p node address " << olsrInterface.GetAddress (0) << " " << p2pInterface.GetAddress (1) << "\n";


	uint32_t num = 0;
	for (NodeContainer::Iterator j = vehicleNodes.Begin (); j != vehicleNodes.End (); ++j)
    {
       Ptr<Node> object = *j;
       Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
       Vector pos = position->GetPosition ();
       std::cout << "vehicle node" << num << " x=" << pos.x << ", y=" << pos.y << std::endl;
       num++;
     }
 


  	Simulator::Stop (Seconds (totalTime));

 	AnimationInterface anim ("my-vanet1.xml");

 	NS_LOG_INFO ("Run Simulation.");
  	Simulator::Run ();
  	Simulator::Destroy ();
	NS_LOG_INFO ("Done.");



  return 0;

}



