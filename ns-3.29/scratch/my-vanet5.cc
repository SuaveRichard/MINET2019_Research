
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
	//double freq = 5.9e9;
	uint32_t packetSize = 1024;
	//double interval = 1.0;
	uint32_t port1 = 1000;
  uint32_t port2 = 1001;

	uint32_t totalTime = 20;
	uint32_t numvehicleNodes1 = 10;
  uint32_t numvehicleNodes2 = 10;
	//uint32_t MaxPacketsCount = 1;
  double maxRange = 100.0;
  

	//std::string traceFile = "/home/fk/sumo/test1mobility.tcl";


	Address serverAddress1;
  Address serverAddress2;

	LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
  LogComponentEnable ("MyVanet1", LOG_LEVEL_INFO);
  //LogComponentEnable ("OlsrRoutingProtocol", LOG_LEVEL_INFO);


    //Time interPacketInterval = Seconds (interval);

    // disable fragmentation for frames below 2200 bytes
  	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
 	// turn off RTS/CTS for frames below 2200 bytes
  	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  	// Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));


  //ここ変えると到達率が変わるよ
  Config::SetDefault ("ns3::OnOffApplication::PacketSize",StringValue ("64"));


  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue ("2048bps"));

//node create
	NodeContainer vehicleNodes1;
	vehicleNodes1.Create (numvehicleNodes1);

  NodeContainer vehicleNodes2;
  vehicleNodes2.Create (numvehicleNodes2);

	NodeContainer p2pNode;
	p2pNode.Create (1);
  

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

    //wifi80211p.EnableLogComponents (); 
    //waveHelper.EnableLogComponents ();


    /*WifiHelper wifi;
    wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "DataMode",StringValue ("DsssRate11Mbps"),
                                  "ControlMode",StringValue ("DsssRate11Mbps"));


    wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                        "DataMode",StringValue (phyMode),
                                        "ControlMode",StringValue (phyMode));

    waveHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                        "DataMode",StringValue (phyMode),
                                        "ControlMode",StringValue (phyMode));


    double m_txp = 20;

    wifiPhy.Set ("TxPowerStart",DoubleValue (m_txp));
    wifiPhy.Set ("TxPowerEnd", DoubleValue (m_txp));

    wavePhy.Set ("TxPowerStart",DoubleValue (m_txp));
    wavePhy.Set ("TxPowerEnd", DoubleValue (m_txp));
*/
  	WifiMacHelper wifiMac;
    wifiMac.SetType ("ns3::AdhocWifiMac");

    QosWaveMacHelper waveMac = QosWaveMacHelper::Default ();

    
    //NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, vehicleNodes);
//////////////////////////////////////////////////////////////////////
    NetDeviceContainer devices = wifi80211p.Install (wifiPhy, wifi80211pMac, vehicleNodes1);

    NetDeviceContainer devices2 = wifi80211p.Install (wifiPhy, wifi80211pMac, vehicleNodes2);

    //wifiPhy.EnablePcap ("my-vanet-80211p", devices);

//p2p設定
    PointToPointHelper p2p;
  	p2p.SetDeviceAttribute ("DataRate",StringValue ("20Mbps"));
  	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  	NetDeviceContainer p2pdevices;
  	p2pdevices = p2p.Install (vehicleNodes1.Get (0), p2pNode.Get (0));

    NetDeviceContainer p2pdevices2;
    p2pdevices2 = p2p.Install (p2pNode.Get (0),vehicleNodes2.Get (0));

  	//std::cout << "Creating point-to-pointnodes.\n"


//olsr設定
  	OlsrHelper olsr;
    //olsr使わないノード・インターフェイス設定
    olsr.ExcludeInterface (vehicleNodes1.Get (0), 2);
    olsr.ExcludeInterface (vehicleNodes2.Get (0), 2);

/*//////////////////////////////////
ExcludeInterface(node,uint32_t interface);
interfaceはノードにインストールされているインターフェイスから
弱い順にソートされているらしい
point-to-pointは2
*///////////////////////////////////


//mobility
  	MobilityHelper p2pmobility;
  	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  	//positionAlloc->Add (Vector (495.0, 200.0, 0.0));
    positionAlloc->Add (Vector (500.0, 200.0, 0.0));
  	p2pmobility.SetPositionAllocator (positionAlloc);
  	p2pmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  	p2pmobility.Install (p2pNode);

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
    internet_olsr2.SetRoutingHelper (list);
    internet_olsr2.Install (vehicleNodes2);


  	InternetStackHelper internet_csma;
  	internet_csma.Install (p2pNode);


 	//olsr
  	Ipv4AddressHelper olsrAddress;
  	olsrAddress.SetBase ("10.1.1.0", "255.255.255.0");

  	Ipv4InterfaceContainer olsrInterface1;
  	olsrInterface1 = olsrAddress.Assign (devices);


    Ipv4AddressHelper olsrAddress2;
    olsrAddress2.SetBase ("11.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer olsrInterface2;
    olsrInterface2 = olsrAddress2.Assign (devices2);



  	//p2p
 	Ipv4AddressHelper p2pAddress;
 	p2pAddress.SetBase ("172.1.1.0", "255.255.255.0");

  	Ipv4InterfaceContainer p2pInterface;
    p2pInterface = p2pAddress.Assign (p2pdevices);


  	serverAddress1 = Address (p2pInterface.GetAddress(1) );

    Ipv4InterfaceContainer p2pInterface2;
    p2pInterface2 = p2pAddress.Assign (p2pdevices2);

    serverAddress2 = Address (p2pInterface2.GetAddress(1) );





	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  
  



//application

	UdpServerHelper server (port1);
  server.SetAttribute ("NodeNumber", UintegerValue (p2pNode.Get(0)->GetId()));
  	ApplicationContainer serverApps = server.Install(p2pNode.Get (0));
  	serverApps.Start (Seconds (1.0));
  	serverApps.Stop (Seconds (totalTime));  	

  	UdpClientHelper client1 (serverAddress1, port1);
  	client1.SetAttribute ("MaxPackets", UintegerValue (1));
  	client1.SetAttribute ("Interval", TimeValue (Seconds (1000)));
  	client1.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client1.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes1.Get(0)->GetId()));

  	ApplicationContainer clientApps1 = client1.Install (vehicleNodes1.Get(0));
  	clientApps1.Start (Seconds (10.0));
  	clientApps1.Stop (Seconds (totalTime));

    UdpClientHelper client2 (serverAddress2, port2);
    client2.SetAttribute ("MaxPackets", UintegerValue (1));
    client2.SetAttribute ("Interval", TimeValue (Seconds (1000)));
    client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client2.SetAttribute ("NodeNumber", UintegerValue (p2pNode.Get(0)->GetId()));

    ApplicationContainer clientApps2 = client2.Install (p2pNode.Get (0));
    clientApps2.Start (Seconds (10.002430));
    clientApps2.Stop (Seconds (totalTime));

UdpServerHelper server2 (port2);
  server2.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes2.Get(0)->GetId()));
    ApplicationContainer serverApps2 = server2.Install(vehicleNodes2.Get (0));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (totalTime));



/*//////////////////////////////////////////////////////////////////////
uint32_t port3 = 80;

UdpServerHelper server0 (port3);
ApplicationContainer serverApps0 = server0.Install(p2pNode.Get (0));
serverApps0.Start (Seconds (0.1));
serverApps0.Stop (Seconds (totalTime));   

UdpClientHelper client0 (serverAddress1, port3);
client0.SetAttribute ("MaxPackets", UintegerValue (1000));
client0.SetAttribute ("Interval", TimeValue (Seconds (1)));
client0.SetAttribute ("PacketSize", UintegerValue (64));

ApplicationContainer clientApps0;

for(uint32_t i = 0; i < numvehicleNodes1; i++){
  client0.Install (vehicleNodes1.Get (i));
}

for(uint32_t i = 0; i < numvehicleNodes2; i++){
  client0.Install (vehicleNodes2.Get (i));
}

clientApps0.Start (MicroSeconds (0));
clientApps0.Stop (Seconds (totalTime));
*///////////////////////////////////////////////////////////////////////




uint32_t m_port = 9;
/*
OnOffHelper onoff1 ("ns3::UdpSocketFactory", 
                    InetSocketAddress(olsrInterface1.GetAddress(numvehicleNodes1 - 1), m_port));
onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

ApplicationContainer onoffApp1 = onoff1.Install(vehicleNodes1.Get(0));
onoffApp1.Start (Seconds (1.0));
onoffApp1.Stop (Seconds (totalTime));

OnOffHelper onoff2 ("ns3::UdpSocketFactory", 
                    InetSocketAddress(olsrInterface2.GetAddress(numvehicleNodes2 - 1), m_port));
onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

ApplicationContainer onoffApp2 = onoff2.Install(vehicleNodes2.Get(0));
onoffApp2.Start (MicroSeconds (1004852));
onoffApp2.Stop (Seconds (totalTime));
*/

//UdpClientHelper source1 (Address(InetSocketAddress(Ipv4Address("255.255.255.255"), m_port)));
UdpClientHelper source1 (Address(olsrInterface1.GetAddress(numvehicleNodes1 - 1)), m_port);
source1.SetAttribute ("MaxPackets", UintegerValue (1));
source1.SetAttribute ("Interval", TimeValue (Seconds (1000)));
source1.SetAttribute ("PacketSize", UintegerValue (packetSize));
source1.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes1.Get(0)->GetId()));

ApplicationContainer sourceApps1 = source1.Install (vehicleNodes1.Get (0));
sourceApps1.Start (Seconds (10.0));
sourceApps1.Stop (Seconds (totalTime));

UdpClientHelper source2 (Address(olsrInterface2.GetAddress(numvehicleNodes2 - 1)), m_port);
source2.SetAttribute ("MaxPackets", UintegerValue (1));
source2.SetAttribute ("Interval", TimeValue (Seconds (1000)));
source2.SetAttribute ("PacketSize", UintegerValue (packetSize));
source2.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes2.Get(0)->GetId()));

ApplicationContainer sourceApps2 = source2.Install (vehicleNodes2.Get (0));
sourceApps2.Start (Seconds (10.004852));
sourceApps2.Stop (Seconds (totalTime));

/*
PacketSinkHelper sink1 ("ns3::UdpSocketFactory",
                      InetSocketAddress(olsrInterface1.GetAddress(0), m_port));
ApplicationContainer sinkApp1 = sink1.Install(vehicleNodes1.Get(numvehicleNodes1 - 1));
sinkApp1.Start (Seconds (1.0));
sinkApp1.Stop (Seconds (totalTime));


PacketSinkHelper sink2 ("ns3::UdpSocketFactory",
                      InetSocketAddress(olsrInterface2.GetAddress(0), m_port));
ApplicationContainer sinkApp2 = sink2.Install(vehicleNodes2.Get(numvehicleNodes2 - 1));
sinkApp2.Start (Seconds (1.0));
sinkApp2.Stop (Seconds (totalTime));
*/


  UdpServerHelper sink1 (m_port);
  sink1.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes1.Get(numvehicleNodes1 - 1 )->GetId()));
  ApplicationContainer sinkApps1 = sink1.Install(vehicleNodes1.Get (numvehicleNodes1 - 1 ));
  sinkApps1.Start (Seconds (1.0));
  sinkApps1.Stop (Seconds (totalTime));  

  UdpServerHelper sink2 (m_port);
  sink2.SetAttribute ("NodeNumber", UintegerValue (vehicleNodes2.Get(numvehicleNodes2 - 1 )->GetId()));
  ApplicationContainer sinkApps2 = sink2.Install(vehicleNodes2.Get (numvehicleNodes2 - 1 ));
  sinkApps2.Start (Seconds (1.0));
  sinkApps2.Stop (Seconds (totalTime));  
/*
  Ptr<Socket> recvSink1 = Socket::CreateSocket (vehicleNodes1.Get (numvehicleNodes1 - 1), tid);
  InetSocketAddress local1 = InetSocketAddress (Ipv4Address::GetAny (), m_port);
  recvSink1->Bind (local1);
  recvSink1-> SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink2 = Socket::CreateSocket (vehicleNodes2.Get (numvehicleNodes2 - 1), tid);
  InetSocketAddress local2 = InetSocketAddress (Ipv4Address::GetAny (), m_port);
  recvSink2->Bind (local2);
  recvSink2-> SetRecvCallback (MakeCallback (&ReceivePacket));


  Ptr<Socket> source = Socket::CreateSocket (vehicleNodes1.Get (0), tid);
  InetSocketAddress remote = InetSocketAddress (olsrInterface1.GetAddress (0) ,m_port);
  source->Connect (remote);
*/




/*
    std::cout << "server node address " << p2pInterface.GetAddress (1) 
              << " " << p2pInterface2.GetAddress (0) << "\n";
    std::cout << "vehicle2/p2p node address " << p2pInterface2.GetAddress (1) << "\n";

*/
//node check
  	for(uint32_t i = 0; i < numvehicleNodes1; i++){

	  	std::cout << "vehicle1 nodes node " << i << " address " 
	  			  << olsrInterface1.GetAddress (i) << "\n";
	}
    for(uint32_t t = 0; t < numvehicleNodes2; t++){

      std::cout << "vehicle2 nodes node " << (t + 10) << " address " 
            << olsrInterface2.GetAddress (t) << "\n";
  }
/*
	std::cout << "server node address " << p2pInterface.GetAddress (0) << "\n";
	std::cout << "vanet/p2p node address " << olsrInterface.GetAddress (0) << " " << p2pInterface.GetAddress (1) << "\n";



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

 	AnimationInterface anim ("my-vanet5.xml");

 	NS_LOG_INFO ("Run Simulation.");
  	Simulator::Run ();
  	Simulator::Destroy ();
	NS_LOG_INFO ("Done.");



  return 0;

}



