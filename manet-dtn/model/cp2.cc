/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright (c) 2004 Francisco J. Ros
  * Copyright (c) 2007 INESC Porto
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
  * Authors: Francisco J. Ros  <fjrm@dif.um.es>
  *          Gustavo J. A. M. Carneiro <gjc@inescporto.pt>
  */
 
 
 
 #define NS_LOG_APPEND_CONTEXT                                   \
   if (GetObject<Node> ()) { std::clog << "[node " << GetObject<Node> ()->GetId () << "] "; }
 
 
 #include "manet_dtn-routing-protocol.h"
 #include "ns3/socket-factory.h"
 #include "ns3/udp-socket-factory.h"
 #include "ns3/simulator.h"
 #include "ns3/log.h"
 #include "ns3/names.h"
 #include "ns3/inet-socket-address.h"
 #include "ns3/ipv4-routing-protocol.h"
 #include "ns3/ipv4-routing-table-entry.h"
 #include "ns3/ipv4-route.h"
 #include "ns3/boolean.h"
 #include "ns3/uinteger.h"
 #include "ns3/enum.h"
 #include "ns3/trace-source-accessor.h"
 #include "ns3/ipv4-header.h"
 #include "ns3/ipv4-packet-info-tag.h"
 #include "ns3/ipv4-l3-protocol.h"
 #include "ns3/mobility-model.h"
 #include "ns3/seq-ts-header.h"

#include <fstream>
 /********** Useful macros **********/
 
 #define DELAY(time) (((time) < (Simulator::Now ())) ? Seconds (0.000001) : \
                      (time - Simulator::Now () + Seconds (0.000001)))
 
 
 
 #define manet_dtn_REFRESH_INTERVAL   m_helloInterval
 
 
 /********** Holding times **********/
 
 #define manet_dtn_NEIGHB_HOLD_TIME   Time (3 * manet_dtn_REFRESH_INTERVAL)
 #define manet_dtn_TOP_HOLD_TIME      Time (3 * m_tcInterval)
 #define manet_dtn_DUP_HOLD_TIME      Seconds (30)
 #define manet_dtn_MID_HOLD_TIME      Time (3 * m_midInterval)
 #define manet_dtn_HNA_HOLD_TIME      Time (3 * m_hnaInterval)
 
 /********** Link types **********/
 
 #define manet_dtn_UNSPEC_LINK        0
 #define manet_dtn_ASYM_LINK          1
 #define manet_dtn_SYM_LINK           2
 #define manet_dtn_LOST_LINK          3
 
 /********** Neighbor types **********/
 
 #define manet_dtn_NOT_NEIGH          0
 #define manet_dtn_SYM_NEIGH          1
 #define manet_dtn_MPR_NEIGH          2
 
 
 /********** Willingness **********/
 
 #define manet_dtn_WILL_NEVER         0
 #define manet_dtn_WILL_LOW           1
 #define manet_dtn_WILL_DEFAULT       3
 #define manet_dtn_WILL_HIGH          6
 #define manet_dtn_WILL_ALWAYS        7
 
 
 /********** Miscellaneous constants **********/
 
 #define manet_dtn_MAXJITTER          (m_helloInterval.GetSeconds () / 4)
 #define manet_dtn_MAX_SEQ_NUM        65535
 #define JITTER (Seconds (m_uniformRandomVariable->GetValue (0, manet_dtn_MAXJITTER)))
 #define RAND (Seconds (m_uniformRandomVariable->GetValue (0, 0.1)))
 
 #define manet_dtn_PORT_NUMBER 698
 #define manet_dtn_MAX_MSGS           64
 
 #define manet_dtn_MAX_HELLOS         12
 
 #define manet_dtn_MAX_ADDRS          64
 
 static bool retransmitted[75];
 static int helloCount_send;
 static int tcCount_send;
 static int svCount_send;
 static int controlCount_send;
 static int derqCount_send;
 static double overhead;
 static int recvCount[75];
 int count = 0;
 FILE *file;
 //static int dtnNodes[] = {25,26,27,28,29,30,31,32,33,34,36,37,38,39,40};
 //static int ACNodes[] = {0,10,20,30,40,50,60,70,80,90};
 namespace ns3 {
 
 NS_LOG_COMPONENT_DEFINE ("ManetDtnRoutingProtocol");
 
 namespace manet_dtn {
 
 /********** manet_dtn class **********/
 
 NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);
 
 TypeId
 RoutingProtocol::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::manet_dtn::RoutingProtocol")
     .SetParent<Ipv4RoutingProtocol> ()
     .SetGroupName ("manet_dtn")
     .AddConstructor<RoutingProtocol> ()
     .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
                    TimeValue (Seconds (2)),
                    MakeTimeAccessor (&RoutingProtocol::m_helloInterval),
                    MakeTimeChecker ())
     .AddAttribute ("SVInterval", "Summary Vector messages emission interval.",
                    TimeValue (Seconds (1)),
                    MakeTimeAccessor (&RoutingProtocol::m_svInterval),
                    MakeTimeChecker ())
     .AddAttribute ("ACMessageInterval", "AC messages emission interval.",
                    TimeValue (Seconds (2)),
                    MakeTimeAccessor (&RoutingProtocol::m_controlInterval),
                    MakeTimeChecker ())
     .AddAttribute ("ManetTimeInterval", "Do MANET function emission interval.",
                    TimeValue (Seconds (20)),
                    MakeTimeAccessor (&RoutingProtocol::m_manetInterval),
                    MakeTimeChecker ())
     .AddAttribute ("TcInterval", "TC messages emission interval.",
                    TimeValue (Seconds (5)),
                    MakeTimeAccessor (&RoutingProtocol::m_tcInterval),
                    MakeTimeChecker ())
     .AddAttribute ("MidInterval", "MID messages emission interval.  Normally it is equal to TcInterval.",
                    TimeValue (Seconds (5)),
                    MakeTimeAccessor (&RoutingProtocol::m_midInterval),
                    MakeTimeChecker ())
     .AddAttribute ("HnaInterval", "HNA messages emission interval.  Normally it is equal to TcInterval.",
                    TimeValue (Seconds (5)),
                    MakeTimeAccessor (&RoutingProtocol::m_hnaInterval),
                    MakeTimeChecker ())
     .AddAttribute ("Willingness", "Willingness of a node to carry and forward traffic for other nodes.",
                    EnumValue (manet_dtn_WILL_DEFAULT),
                    MakeEnumAccessor (&RoutingProtocol::m_willingness),
                    MakeEnumChecker (manet_dtn_WILL_NEVER, "never",
                                     manet_dtn_WILL_LOW, "low",
                                     manet_dtn_WILL_DEFAULT, "default",
                                     manet_dtn_WILL_HIGH, "high",
                                     manet_dtn_WILL_ALWAYS, "always"))
     .AddAttribute ("QueueLength","Maximum number of "
                    "packets that a queue can hold.",
                    UintegerValue (64),
                    MakeUintegerAccessor (&RoutingProtocol::m_maxQueueLen),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QueueEntryExpireTime","Maximum time a packet can live in "
                    "the epidemic queues since it's generated at the source.",
                    TimeValue (Seconds (50)),
                    MakeTimeAccessor (&RoutingProtocol::m_queueEntryExpireTime),
                    MakeTimeChecker ())
     .AddTraceSource ("Rx", "Receive manet_dtn packet.",
                      MakeTraceSourceAccessor (&RoutingProtocol::m_rxPacketTrace),
                      "ns3::manet_dtn::RoutingProtocol::PacketTxRxTracedCallback")
     .AddTraceSource ("Tx", "Send manet_dtn packet.",
                      MakeTraceSourceAccessor (&RoutingProtocol::m_txPacketTrace),
                      "ns3::manet_dtn::RoutingProtocol::PacketTxRxTracedCallback")
     .AddTraceSource ("RoutingTableChanged", "The manet_dtn routing table has changed.",
                      MakeTraceSourceAccessor (&RoutingProtocol::m_routingTableChanged),
                      "ns3::manet_dtn::RoutingProtocol::TableChangeTracedCallback")
   ;
   return tid;
 }
 
 
 RoutingProtocol::RoutingProtocol ()
   : m_routingTableAssociation (0),
   m_ipv4 (0),
   m_maxQueueLen (10),
   m_packetSequenceNumber (0),    
   m_queue (0),
   m_protcol (1),
   m_vType (0),
   m_queueEntryExpireTime (Seconds (0)),
   m_helloTimer (Timer::CANCEL_ON_DESTROY),
   m_svTimer (Timer::CANCEL_ON_DESTROY),
   m_manetTimer (Timer::CANCEL_ON_DESTROY),
   m_tcTimer (Timer::CANCEL_ON_DESTROY),
   m_acTimer (Timer::CANCEL_ON_DESTROY),
   m_midTimer (Timer::CANCEL_ON_DESTROY),
   m_hnaTimer (Timer::CANCEL_ON_DESTROY),
   m_queuedPacketTimer (Timer::CANCEL_ON_DESTROY),
   m_queuedMessagesTimer (Timer::CANCEL_ON_DESTROY)
 {
   m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
 
   m_hnaRoutingTable = Create<Ipv4StaticRouting> ();
 }
   
 RoutingProtocol::~RoutingProtocol ()
 {
  //送信回数
  file = fopen("data/result.txt", "w");
  fprintf(file, "HELLO %d\n", helloCount_send);
  fprintf(file, "TC %d\n", tcCount_send);
  fprintf(file, "SV %d\n", svCount_send);
  fprintf(file, "CONTROL %d\n", controlCount_send);
  fprintf(file, "DREQ %d\n", derqCount_send);
  //overhead
  fprintf(file,"OverHead %f\n",overhead);
/*  file = fopen("data/RecvCount.csv","a");
  fprintf(file,"%d\n",recvCount[count]);*/
  //count++;
  fclose(file);
 }
 void
 RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
 {
   NS_ASSERT (ipv4 != 0);
   NS_ASSERT (m_ipv4 == 0);
   NS_LOG_DEBUG ("Created manet_dtn::RoutingProtocol");
   m_queue.SetMaxQueueLen(m_maxQueueLen);
   m_helloTimer.SetFunction (&RoutingProtocol::HelloTimerExpire, this);
   m_manetTimer.SetFunction (&RoutingProtocol::ManetTimeExpire,this);
   m_acTimer.SetFunction (&RoutingProtocol::ACMessageTimeExpire, this);
   m_tcTimer.SetFunction (&RoutingProtocol::TcTimerExpire, this);
   m_svTimer.SetFunction (&RoutingProtocol::SVTimerExpire, this);
   m_midTimer.SetFunction (&RoutingProtocol::MidTimerExpire, this);
   m_hnaTimer.SetFunction (&RoutingProtocol::HnaTimerExpire, this);
   m_queuedMessagesTimer.SetFunction (&RoutingProtocol::SendQueuedMessages, this);
   m_queuedPacketTimer.SetFunction (&RoutingProtocol::MprBroadcast,this);
   m_packetSequenceNumber = manet_dtn_MAX_SEQ_NUM;
   m_messageSequenceNumber = manet_dtn_MAX_SEQ_NUM;
   m_ansn = manet_dtn_MAX_SEQ_NUM;
 
   m_linkTupleTimerFirstTime = true;
 
   m_ipv4 = ipv4;
   /*auto itr = std::find(std::begin(dtnNodes),std::end(dtnNodes),m_ipv4->GetObject<Node>()->GetId());
   if(itr != std::end(dtnNodes))
   {
     m_protcol = 0;
   }
   else
   {
     m_protcol = 1;
   }*/
    if(29 < m_ipv4->GetObject<Node>()->GetId() && m_ipv4->GetObject<Node>()->GetId()< 45)
   {
     m_protcol = 0;
   }
 /*   if(500 <= m_ipv4->GetObject<Node>()->GetObject<MobilityModel>()->GetPosition().x && m_ipv4->GetObject<Node>()->GetObject<MobilityModel>()->GetPosition().x <= 740)
   {
     m_protcol = 0;
   }*/
  // m_protcol = 1;
   m_vType = 0;
   /* auto itr = std::find(std::begin(ACNodes),std::end(ACNodes),m_ipv4->GetObject<Node>()->GetId());
   if(itr != std::end(ACNodes))
   {
     m_vType = 1;
   }
   else
   {
     m_vType = 0;
   }
   m_protcol = 0;
   */
   for(int i=0; i < 75/*retransmitted.size()*/; i++)
   {
     retransmitted[i] = false;
     recvCount[i] = 0;
   }
   m_hnaRoutingTable->SetIpv4 (ipv4);
 }
 
 void RoutingProtocol::DoDispose ()
 {
   m_ipv4 = 0;
   m_hnaRoutingTable = 0;
   m_routingTableAssociation = 0;
 
   if (m_recvSocket)
     {
       m_recvSocket->Close ();
       m_recvSocket = 0;
     }
 
   for (std::map< Ptr<Socket>, Ipv4InterfaceAddress >::iterator iter = m_sendSockets.begin ();
        iter != m_sendSockets.end (); iter++)
     {
       iter->first->Close ();
     }
   m_sendSockets.clear ();
   m_table.clear ();
 
   Ipv4RoutingProtocol::DoDispose ();
 }
 
 void
 RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
 {
   std::ostream* os = stream->GetStream ();
 
   *os << "Node: " << m_ipv4->GetObject<Node> ()->GetId ()
       << ", Time: " << Now ().As (unit)
       << ", Local time: " << GetObject<Node> ()->GetLocalTime ().As (unit)
       << ", manet_dtn Routing table" << std::endl;
 
   *os << "Destination\t\tNextHop\t\tInterface\tDistance\tlocation\tvType\n";
 
   for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator iter = m_table.begin ();
        iter != m_table.end (); iter++)
     {
       *os << iter->first << "\t\t";
       *os << iter->second.nextAddr << "\t\t";
       if (Names::FindName (m_ipv4->GetNetDevice (iter->second.interface)) != "")
         {
           *os << Names::FindName (m_ipv4->GetNetDevice (iter->second.interface)) << "\t\t";
         }
       else
         {
           *os << iter->second.interface << "\t\t";
         }
       *os << iter->second.distance << "\t\t";
       *os << iter->second.position << "\t    ";
       *os << iter->second.vType << "\t";
       *os << "\n";
     }
 
   // Also print the HNA routing table
   if (m_hnaRoutingTable->GetNRoutes () > 0)
     {
       *os << " HNA Routing Table: ";
       m_hnaRoutingTable->PrintRoutingTable (stream, unit);
     }
   else
     {
       *os << " HNA Routing Table: empty" << std::endl;
     }
 }
 
 void RoutingProtocol::DoInitialize ()
 {
   if (m_mainAddress == Ipv4Address ())
     {
       Ipv4Address loopback ("127.0.0.1");
       for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
         {
           // Use primary address, if multiple
           Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
           if (addr != loopback)
             {
               m_mainAddress = addr;
               break;
             }
         }
 
       NS_ASSERT (m_mainAddress != Ipv4Address ());
     }
 
   NS_LOG_DEBUG ("Starting manet_dtn on node " << m_mainAddress);
 
   Ipv4Address loopback ("127.0.0.1");
 
   bool canRunmanet_dtn = false;
   for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
     {
       Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
       if (addr == loopback)
         {
           continue;
         }
 
       if (addr != m_mainAddress)
         {
           // Create never expiring interface association tuple entries for our
           // own network interfaces, so that GetMainAddress () works to
           // translate the node's own interface addresses into the main address.
           IfaceAssocTuple tuple;
           tuple.ifaceAddr = addr;
           tuple.mainAddr = m_mainAddress;
           AddIfaceAssocTuple (tuple);
           NS_ASSERT (GetMainAddress (addr) == m_mainAddress);
         }
 
       if (m_interfaceExclusions.find (i) != m_interfaceExclusions.end ())
         {
           continue;
         }
 
       // Create a socket to listen on all the interfaces
       if (m_recvSocket == 0)
         {
           m_recvSocket = Socket::CreateSocket (GetObject<Node> (),
                                                UdpSocketFactory::GetTypeId ());
           m_recvSocket->SetAllowBroadcast (true);
           InetSocketAddress inetAddr (Ipv4Address::GetAny (), manet_dtn_PORT_NUMBER);
           m_recvSocket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvPacket,  this));
           if (m_recvSocket->Bind (inetAddr))
             {
               NS_FATAL_ERROR ("Failed to bind() manet_dtn socket");
             }
           m_recvSocket->SetRecvPktInfo (true);
           m_recvSocket->ShutdownSend ();
         }
 
       // Create a socket to send packets from this specific interfaces
       Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                  UdpSocketFactory::GetTypeId ());
       socket->SetAllowBroadcast (true);
       InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), manet_dtn_PORT_NUMBER);
       socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvPacket,  this));
       socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
       if (socket->Bind (inetAddr))
         {
           NS_FATAL_ERROR ("Failed to bind() manet_dtn socket");
         }
       socket->SetRecvPktInfo (true);
       m_sendSockets[socket] = m_ipv4->GetAddress (i, 0);
 
       canRunmanet_dtn = true;
     }
 
   if (canRunmanet_dtn)
     {
       HelloTimerExpire ();
       SVTimerExpire ();
       TcTimerExpire ();
       MidTimerExpire ();
       HnaTimerExpire ();
      // ManetTimeExpire ();
    //   ACMessageTimeExpire ();
       NS_LOG_DEBUG ("manet_dtn on node " << m_mainAddress << " started");
     }
 }
 
 void RoutingProtocol::SetMainInterface (uint32_t interface)
 {
   m_mainAddress = m_ipv4->GetAddress (interface, 0).GetLocal ();
 }
 
 void RoutingProtocol::SetInterfaceExclusions (std::set<uint32_t> exceptions)
 {
   m_interfaceExclusions = exceptions;
 }
 
 //
 // \brief Processes an incoming %manet_dtn packet following \RFC{3626} specification.
 void
 RoutingProtocol::RecvPacket (Ptr<Socket> socket)
 {
   Ptr<Packet> receivedPacket;
   Address sourceAddress;
   receivedPacket = socket->RecvFrom (sourceAddress);
   Ipv4PacketInfoTag interfaceInfo;
   if (!receivedPacket->RemovePacketTag (interfaceInfo))
     {
       NS_ABORT_MSG ("No incoming interface on manet_dtn message, aborting.");
     }
   uint32_t incomingIf = interfaceInfo.GetRecvIf ();
   Ptr<Node> node = this->GetObject<Node> ();
   Ptr<NetDevice> dev = node->GetDevice (incomingIf);
   uint32_t recvInterfaceIndex = m_ipv4->GetInterfaceForDevice (dev);
 
   if (m_interfaceExclusions.find (recvInterfaceIndex) != m_interfaceExclusions.end ())
     {
       return;
     }
 
 
   InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
   Ipv4Address senderIfaceAddr = inetSourceAddr.GetIpv4 ();
 
   int32_t interfaceForAddress = m_ipv4->GetInterfaceForAddress (senderIfaceAddr);
   if (interfaceForAddress != -1)
     {
       NS_LOG_LOGIC ("Ignoring a packet sent by myself.");
       return;
     }
 
   Ipv4Address receiverIfaceAddr = m_ipv4->GetAddress (recvInterfaceIndex, 0).GetLocal ();
   NS_ASSERT (receiverIfaceAddr != Ipv4Address ());
   NS_LOG_DEBUG ("manet_dtn node " << m_mainAddress << " received a manet_dtn packet from "
                              << senderIfaceAddr << " to " << receiverIfaceAddr);
 
   // All routing messages are sent from and to port RT_PORT,
   // so we check it.
   NS_ASSERT (inetSourceAddr.GetPort () == manet_dtn_PORT_NUMBER);
 
   Ptr<Packet> packet = receivedPacket;
 //  MessageHeader messageHeader;
   manet_dtn::PacketHeader manet_dtnPacketHeader;
   
   packet->RemoveHeader (manet_dtnPacketHeader);
 //  packet->RemoveHeader(messageHeader);
   NS_ASSERT (manet_dtnPacketHeader.GetPacketLength () >= manet_dtnPacketHeader.GetSerializedSize ());
   uint32_t sizeLeft = manet_dtnPacketHeader.GetPacketLength () - manet_dtnPacketHeader.GetSerializedSize ();
   MessageList messages;
   while (sizeLeft)
     {
       NS_LOG_DEBUG("sizeLeft" << sizeLeft);
       MessageHeader messageHeader;
       if (packet->RemoveHeader (messageHeader) == 0)
         {
           NS_ASSERT (false);
         }
       NS_LOG_DEBUG("messageHeader.GetSerializedSize ()" << messageHeader.GetSerializedSize ());
       sizeLeft -= messageHeader.GetSerializedSize ();
 
       NS_LOG_DEBUG ("manet_dtn Msg received with type "
                     << std::dec << int (messageHeader.GetMessageType ())
                     << " TTL=" << int (messageHeader.GetTimeToLive ())
                     << " origAddr=" << messageHeader.GetOriginatorAddress ());
       messages.push_back (messageHeader);
     }
   m_rxPacketTrace (manet_dtnPacketHeader, messages);
 
   for (MessageList::const_iterator messageIter = messages.begin ();
        messageIter != messages.end (); messageIter++)
     {
       const MessageHeader &messageHeader = *messageIter;
       // If ttl is less than or equal to zero, or
       // the receiver is the same as the originator,
       // the message must be silently dropped
       if (messageHeader.GetTimeToLive () == 0
           || messageHeader.GetOriginatorAddress () == m_mainAddress)
         {
           packet->RemoveAtStart (messageHeader.GetSerializedSize ()
                                  - messageHeader.GetSerializedSize ());
           continue;
         }
 
       // If the message has been processed it must not be processed again
       bool do_forwarding = true;
       DuplicateTuple *duplicated = m_state.FindDuplicateTuple
           (messageHeader.GetOriginatorAddress (),
           messageHeader.GetMessageSequenceNumber ());
 
       // Get main address of the peer, which may be different from the packet source address
 //       const IfaceAssocTuple *ifaceAssoc = m_state.FindIfaceAssocTuple (inetSourceAddr.GetIpv4 ());
 //       Ipv4Address peerMainAddress;
 //       if (ifaceAssoc != NULL)
 //         {
 //           peerMainAddress = ifaceAssoc->mainAddr;
 //         }
 //       else
 //         {
 //           peerMainAddress = inetSourceAddr.GetIpv4 () ;
 //         }
 
       if (duplicated == NULL)
         {
           switch (messageHeader.GetMessageType ())
             {
             case manet_dtn::MessageHeader::HELLO_MESSAGE:
               NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                             << "s manet_dtn node " << m_mainAddress
                             << " received HELLO message of size " << messageHeader.GetSerializedSize ());
              if(m_protcol == 1){
               ProcessHello (messageHeader, receiverIfaceAddr, senderIfaceAddr);
              }
               break;
             case manet_dtn::MessageHeader::TC_MESSAGE:
               NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                             << "s manet_dtn node " << m_mainAddress
                             << " received TC message of size " << messageHeader.GetSerializedSize ());
               if(m_protcol == 1){
               ProcessTc (messageHeader, senderIfaceAddr);
               }
               break;
             case manet_dtn::MessageHeader::MID_MESSAGE:
               NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                             << "s manet_dtn node " << m_mainAddress
                             <<  " received MID message of size " << messageHeader.GetSerializedSize ());
               ProcessMid (messageHeader, senderIfaceAddr);
               break;
             case manet_dtn::MessageHeader::HNA_MESSAGE:
               NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                             << "s manet_dtn node " << m_mainAddress
                             <<  " received HNA message of size " << messageHeader.GetSerializedSize ());
               ProcessHna (messageHeader, senderIfaceAddr);
               break;
              case manet_dtn::MessageHeader::SV_MESSAGE:
               NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                             << "s manet_dtn node " << m_mainAddress
                             <<  " received SV message of size " << messageHeader.GetSerializedSize ());
               ProcessSV (messageHeader, senderIfaceAddr);
               break;
             case manet_dtn::MessageHeader::DREQ_MESSAGE:
               NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                             << "s manet_dtn node " << m_mainAddress
                             <<  " received DREQ message of size " << messageHeader.GetSerializedSize ());
               ProcessDREQ (messageHeader, senderIfaceAddr);
               break;
             case manet_dtn::MessageHeader::CONTROL_MESSAGE:
               NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                             << "s manet_dtn node " << m_mainAddress
                             <<  " received CONTROL message of size " << messageHeader.GetSerializedSize ());
               ProcessControl (messageHeader, senderIfaceAddr);
               break;
             default:
               NS_LOG_DEBUG ("manet_dtn message type " <<
                             int (messageHeader.GetMessageType ()) <<
                             " not implemented");
             }
         }
       else
         {
           NS_LOG_DEBUG ("manet_dtn message is duplicated, not reading it.");
 
           // If the message has been considered for forwarding, it should
           // not be retransmitted again
           for (std::vector<Ipv4Address>::const_iterator it = duplicated->ifaceList.begin ();
                it != duplicated->ifaceList.end (); it++)
             {
               if (*it == receiverIfaceAddr)
                 {
                   do_forwarding = false;
                   break;
                 }
             }
         }
 
       if (do_forwarding && m_protcol == 1)
         {
           // HELLO messages are never forwarded.
           // TC and MID messages are forwarded using the default algorithm.
           // Remaining messages are also forwarded using the default algorithm.
           if (messageHeader.GetMessageType ()  != manet_dtn::MessageHeader::HELLO_MESSAGE)
             {
               ForwardDefault (messageHeader, duplicated,
                               receiverIfaceAddr, inetSourceAddr.GetIpv4 ());
             }
         }
     }
 
   // After processing all manet_dtn messages, we must recompute the routing table
 //  if(m_protcol == 1 && messageHeader.GetMessageType() != manet_dtn::MessageHeader::SV_MESSAGE && messageHeader.GetMessageType() != manet_dtn::MessageHeader::DREQ_MESSAGE)
 if(m_protcol == 1)
   RoutingTableComputation ();
 }
 
 int
 RoutingProtocol::Degree (NeighborTuple const &tuple)
 {
   int degree = 0;
   for (TwoHopNeighborSet::const_iterator it = m_state.GetTwoHopNeighbors ().begin ();
        it != m_state.GetTwoHopNeighbors ().end (); it++)
     {
       TwoHopNeighborTuple const &nb2hop_tuple = *it;
       if (nb2hop_tuple.neighborMainAddr == tuple.neighborMainAddr)
         {
           const NeighborTuple *nb_tuple =
             m_state.FindNeighborTuple (nb2hop_tuple.neighborMainAddr);
           if (nb_tuple == NULL)
             {
               degree++;
             }
         }
     }
   return degree;
 }
 
 namespace {
 void
 CoverTwoHopNeighbors (Ipv4Address neighborMainAddr, TwoHopNeighborSet & N2)
 {
   // first gather all 2-hop neighbors to be removed
   std::set<Ipv4Address> toRemove;
   for (TwoHopNeighborSet::iterator twoHopNeigh = N2.begin (); twoHopNeigh != N2.end (); twoHopNeigh++)
     {
       if (twoHopNeigh->neighborMainAddr == neighborMainAddr)
         {
           toRemove.insert (twoHopNeigh->twoHopNeighborAddr);
         }
     }
   // Now remove all matching records from N2
   for (TwoHopNeighborSet::iterator twoHopNeigh = N2.begin (); twoHopNeigh != N2.end (); )
     {
       if (toRemove.find (twoHopNeigh->twoHopNeighborAddr) != toRemove.end ())
         {
           twoHopNeigh = N2.erase (twoHopNeigh);
         }
       else
         {
           twoHopNeigh++;
         }
     }
 }
 }  // unnamed namespace
 
 void
 RoutingProtocol::MprComputation ()
 {
   NS_LOG_FUNCTION (this);
 
   // MPR computation should be done for each interface. See section 8.3.1
   // (RFC 3626) for details.
   MprSet mprSet;
 
   // N is the subset of neighbors of the node, which are
   // neighbor "of the interface I"
   NeighborSet N;
   for (NeighborSet::const_iterator neighbor = m_state.GetNeighbors ().begin ();
        neighbor != m_state.GetNeighbors ().end (); neighbor++)
     {
       if (neighbor->status == NeighborTuple::STATUS_SYM) // I think that we need this check
         {
           N.push_back (*neighbor);
         }
     }
 
   // N2 is the set of 2-hop neighbors reachable from "the interface
   // I", excluding:
   // (i)   the nodes only reachable by members of N with willingness WILL_NEVER
   // (ii)  the node performing the computation
   // (iii) all the symmetric neighbors: the nodes for which there exists a symmetric
   //       link to this node on some interface.
   TwoHopNeighborSet N2;
   for (TwoHopNeighborSet::const_iterator twoHopNeigh = m_state.GetTwoHopNeighbors ().begin ();
        twoHopNeigh != m_state.GetTwoHopNeighbors ().end (); twoHopNeigh++)
     {
       // excluding:
       // (ii)  the node performing the computation
       if (twoHopNeigh->twoHopNeighborAddr == m_mainAddress)
         {
           continue;
         }
 
       //  excluding:
       // (i)   the nodes only reachable by members of N with willingness WILL_NEVER
       bool ok = false;
       for (NeighborSet::const_iterator neigh = N.begin ();
            neigh != N.end (); neigh++)
         {
           if (neigh->neighborMainAddr == twoHopNeigh->neighborMainAddr)
             {
               if (neigh->willingness == manet_dtn_WILL_NEVER)
                 {
                   ok = false;
                   break;
                 }
               else
                 {
                   ok = true;
                   break;
                 }
             }
         }
       if (!ok)
         {
           continue;
         }
 
       // excluding:
       // (iii) all the symmetric neighbors: the nodes for which there exists a symmetric
       //       link to this node on some interface.
       for (NeighborSet::const_iterator neigh = N.begin ();
            neigh != N.end (); neigh++)
         {
           if (neigh->neighborMainAddr == twoHopNeigh->twoHopNeighborAddr)
             {
               ok = false;
               break;
             }
         }
 
       if (ok)
         {
           N2.push_back (*twoHopNeigh);
         }
     }
 
 #ifdef NS3_LOG_ENABLE
   {
     std::ostringstream os;
     os << "[";
     for (TwoHopNeighborSet::const_iterator iter = N2.begin ();
          iter != N2.end (); iter++)
       {
         TwoHopNeighborSet::const_iterator next = iter;
         next++;
         os << iter->neighborMainAddr << "->" << iter->twoHopNeighborAddr;
         if (next != N2.end ())
           {
             os << ", ";
           }
       }
     os << "]";
     NS_LOG_DEBUG ("N2: " << os.str ());
   }
 #endif  //NS3_LOG_ENABLE
 
   // 1. Start with an MPR set made of all members of N with
   // N_willingness equal to WILL_ALWAYS
   for (NeighborSet::const_iterator neighbor = N.begin (); neighbor != N.end (); neighbor++)
     {
       if (neighbor->willingness == manet_dtn_WILL_ALWAYS)
         {
           mprSet.insert (neighbor->neighborMainAddr);
           // (not in RFC but I think is needed: remove the 2-hop
           // neighbors reachable by the MPR from N2)
           CoverTwoHopNeighbors (neighbor->neighborMainAddr, N2);
         }
     }
 
   // 2. Calculate D(y), where y is a member of N, for all nodes in N.
   // (we do this later)
 
   // 3. Add to the MPR set those nodes in N, which are the *only*
   // nodes to provide reachability to a node in N2.
   std::set<Ipv4Address> coveredTwoHopNeighbors;
   for (TwoHopNeighborSet::const_iterator twoHopNeigh = N2.begin (); twoHopNeigh != N2.end (); twoHopNeigh++)
     {
       bool onlyOne = true;
       // try to find another neighbor that can reach twoHopNeigh->twoHopNeighborAddr
       for (TwoHopNeighborSet::const_iterator otherTwoHopNeigh = N2.begin (); otherTwoHopNeigh != N2.end (); otherTwoHopNeigh++)
         {
           if (otherTwoHopNeigh->twoHopNeighborAddr == twoHopNeigh->twoHopNeighborAddr
               && otherTwoHopNeigh->neighborMainAddr != twoHopNeigh->neighborMainAddr)
             {
               onlyOne = false;
               break;
             }
         }
       if (onlyOne)
         {
           NS_LOG_LOGIC ("Neighbor " << twoHopNeigh->neighborMainAddr
                                     << " is the only that can reach 2-hop neigh. "
                                     << twoHopNeigh->twoHopNeighborAddr
                                     << " => select as MPR.");
 
           mprSet.insert (twoHopNeigh->neighborMainAddr);
 
           // take note of all the 2-hop neighbors reachable by the newly elected MPR
           for (TwoHopNeighborSet::const_iterator otherTwoHopNeigh = N2.begin ();
                otherTwoHopNeigh != N2.end (); otherTwoHopNeigh++)
             {
               if (otherTwoHopNeigh->neighborMainAddr == twoHopNeigh->neighborMainAddr)
                 {
                   coveredTwoHopNeighbors.insert (otherTwoHopNeigh->twoHopNeighborAddr);
                 }
             }
         }
     }
   // Remove the nodes from N2 which are now covered by a node in the MPR set.
   for (TwoHopNeighborSet::iterator twoHopNeigh = N2.begin ();
        twoHopNeigh != N2.end (); )
     {
       if (coveredTwoHopNeighbors.find (twoHopNeigh->twoHopNeighborAddr) != coveredTwoHopNeighbors.end ())
         {
           // This works correctly only because it is known that twoHopNeigh is reachable by exactly one neighbor,
           // so only one record in N2 exists for each of them. This record is erased here.
           NS_LOG_LOGIC ("2-hop neigh. " << twoHopNeigh->twoHopNeighborAddr << " is already covered by an MPR.");
           twoHopNeigh = N2.erase (twoHopNeigh);
         }
       else
         {
           twoHopNeigh++;
         }
     }
 
   // 4. While there exist nodes in N2 which are not covered by at
   // least one node in the MPR set:
   while (N2.begin () != N2.end ())
     {
 
 #ifdef NS3_LOG_ENABLE
       {
         std::ostringstream os;
         os << "[";
         for (TwoHopNeighborSet::const_iterator iter = N2.begin ();
              iter != N2.end (); iter++)
           {
             TwoHopNeighborSet::const_iterator next = iter;
             next++;
             os << iter->neighborMainAddr << "->" << iter->twoHopNeighborAddr;
             if (next != N2.end ())
               {
                 os << ", ";
               }
           }
         os << "]";
         NS_LOG_DEBUG ("Step 4 iteration: N2=" << os.str ());
       }
 #endif  //NS3_LOG_ENABLE
 
 
       // 4.1. For each node in N, calculate the reachability, i.e., the
       // number of nodes in N2 which are not yet covered by at
       // least one node in the MPR set, and which are reachable
       // through this 1-hop neighbor
       std::map<int, std::vector<const NeighborTuple *> > reachability;
       std::set<int> rs;
       for (NeighborSet::iterator it = N.begin (); it != N.end (); it++)
         {
           NeighborTuple const &nb_tuple = *it;
           int r = 0;
           for (TwoHopNeighborSet::iterator it2 = N2.begin (); it2 != N2.end (); it2++)
             {
               TwoHopNeighborTuple const &nb2hop_tuple = *it2;
               if (nb_tuple.neighborMainAddr == nb2hop_tuple.neighborMainAddr)
                 {
                   r++;
                 }
             }
           rs.insert (r);
           reachability[r].push_back (&nb_tuple);
         }
 
       // 4.2. Select as a MPR the node with highest N_willingness among
       // the nodes in N with non-zero reachability. In case of
       // multiple choice select the node which provides
       // reachability to the maximum number of nodes in N2. In
       // case of multiple nodes providing the same amount of
       // reachability, select the node as MPR whose D(y) is
       // greater. Remove the nodes from N2 which are now covered
       // by a node in the MPR set.
       NeighborTuple const *max = NULL;
       int max_r = 0;
       for (std::set<int>::iterator it = rs.begin (); it != rs.end (); it++)
         {
           int r = *it;
           if (r == 0)
             {
               continue;
             }
           for (std::vector<const NeighborTuple *>::iterator it2 = reachability[r].begin ();
                it2 != reachability[r].end (); it2++)
             {
               const NeighborTuple *nb_tuple = *it2;
               if (max == NULL || nb_tuple->willingness > max->willingness)
                 {
                   max = nb_tuple;
                   max_r = r;
                 }
               else if (nb_tuple->willingness == max->willingness)
                 {
                   if (r > max_r)
                     {
                       max = nb_tuple;
                       max_r = r;
                     }
                   else if (r == max_r)
                     {
                       if (Degree (*nb_tuple) > Degree (*max))
                         {
                           max = nb_tuple;
                           max_r = r;
                         }
                     }
                 }
             }
         }
 
       if (max != NULL)
         {
           mprSet.insert (max->neighborMainAddr);
           CoverTwoHopNeighbors (max->neighborMainAddr, N2);
           NS_LOG_LOGIC (N2.size () << " 2-hop neighbors left to cover!");
         }
     }
 
 #ifdef NS3_LOG_ENABLE
   {
     std::ostringstream os;
     os << "[";
     for (MprSet::const_iterator iter = mprSet.begin ();
          iter != mprSet.end (); iter++)
       {
         MprSet::const_iterator next = iter;
         next++;
         os << *iter;
         if (next != mprSet.end ())
           {
             os << ", ";
           }
       }
     os << "]";
     NS_LOG_DEBUG ("Computed MPR set for node " << m_mainAddress << ": " << os.str ());
   }
 #endif  //NS3_LOG_ENABLE
 
   m_state.SetMprSet (mprSet);
 }
 
 Ipv4Address
 RoutingProtocol::GetMainAddress (Ipv4Address iface_addr) const
 {
   const IfaceAssocTuple *tuple =
     m_state.FindIfaceAssocTuple (iface_addr);
 
   if (tuple != NULL)
     {
       return tuple->mainAddr;
     }
   else
     {
       return iface_addr;
     }
 }
 
 void
 RoutingProtocol::RoutingTableComputation ()
 {
    NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << " s: Node " << m_mainAddress
                                                 << ": RoutingTableComputation begin...");
 
   // 1. All the entries from the routing table are removed.
   Clear ();
 
   // 2. The new routing entries are added starting with the
   // symmetric neighbors (h=1) as the destination nodes.
   const NeighborSet &neighborSet = m_state.GetNeighbors ();
   for (NeighborSet::const_iterator it = neighborSet.begin ();
        it != neighborSet.end (); it++)
     {
       NeighborTuple const &nb_tuple = *it;
       NS_LOG_DEBUG ("Looking at neighbor tuple: " << nb_tuple);
       if (nb_tuple.status == NeighborTuple::STATUS_SYM)
         {
           bool nb_main_addr = false;
           const LinkTuple *lt = NULL;
           const LinkSet &linkSet = m_state.GetLinks ();
           for (LinkSet::const_iterator it2 = linkSet.begin ();
                it2 != linkSet.end (); it2++)
             {
               LinkTuple const &link_tuple = *it2;
               NS_LOG_DEBUG ("Looking at link tuple: " << link_tuple
                                                       << (link_tuple.time >= Simulator::Now () ? "" : " (expired)"));
               if ((GetMainAddress (link_tuple.neighborIfaceAddr) == nb_tuple.neighborMainAddr)
                   && link_tuple.time >= Simulator::Now ())
                 {
                   NS_LOG_LOGIC ("Link tuple matches neighbor " << nb_tuple.neighborMainAddr
                                                                << " => adding routing table entry to neighbor");
                   lt = &link_tuple;
                   AddEntry (link_tuple.neighborIfaceAddr,
                             link_tuple.neighborIfaceAddr,
                             link_tuple.localIfaceAddr,
                             1,
                             link_tuple.position,
                             link_tuple.vType);
                            
                   if (link_tuple.neighborIfaceAddr == nb_tuple.neighborMainAddr)
                     {
                       nb_main_addr = true;
                     }
                 }
               else
                 {
                   NS_LOG_LOGIC ("Link tuple: linkMainAddress= " << GetMainAddress (link_tuple.neighborIfaceAddr)
                                                                 << "; neighborMainAddr =  " << nb_tuple.neighborMainAddr
                                                                 << "; expired=" << int (link_tuple.time < Simulator::Now ())
                                                                 << " => IGNORE");
                 }
             }
 
           // If, in the above, no R_dest_addr is equal to the main
           // address of the neighbor, then another new routing entry
           // with MUST be added, with:
           //      R_dest_addr  = main address of the neighbor;
           //      R_next_addr  = L_neighbor_iface_addr of one of the
           //                     associated link tuple with L_time >= current time;
           //      R_dist       = 1;
           //      R_iface_addr = L_local_iface_addr of the
           //                     associated link tuple.
           if (!nb_main_addr && lt != NULL)
             {
               NS_LOG_LOGIC ("no R_dest_addr is equal to the main address of the neighbor "
                             "=> adding additional routing entry");
               AddEntry (nb_tuple.neighborMainAddr,
                         lt->neighborIfaceAddr,
                         lt->localIfaceAddr,
                         1,
                         lt->position,
                         lt->vType);
             }
         }
     }
 
   //  3. for each node in N2, i.e., a 2-hop neighbor which is not a
   //  neighbor node or the node itself, and such that there exist at
   //  least one entry in the 2-hop neighbor set where
   //  N_neighbor_main_addr correspond to a neighbor node with
   //  willingness different of WILL_NEVER,
   const TwoHopNeighborSet &twoHopNeighbors = m_state.GetTwoHopNeighbors ();
   for (TwoHopNeighborSet::const_iterator it = twoHopNeighbors.begin ();
        it != twoHopNeighbors.end (); it++)
     {
       TwoHopNeighborTuple const &nb2hop_tuple = *it;
 
       NS_LOG_LOGIC ("Looking at two-hop neighbor tuple: " << nb2hop_tuple);
 
       // a 2-hop neighbor which is not a neighbor node or the node itself
       if (m_state.FindSymNeighborTuple (nb2hop_tuple.twoHopNeighborAddr))
         {
           NS_LOG_LOGIC ("Two-hop neighbor tuple is also neighbor; skipped.");
           continue;
         }
 
       if (nb2hop_tuple.twoHopNeighborAddr == m_mainAddress)
         {
           NS_LOG_LOGIC ("Two-hop neighbor is self; skipped.");
           continue;
         }
 
       // ...and such that there exist at least one entry in the 2-hop
       // neighbor set where N_neighbor_main_addr correspond to a
       // neighbor node with willingness different of WILL_NEVER...
       bool nb2hopOk = false;
       for (NeighborSet::const_iterator neighbor = neighborSet.begin ();
            neighbor != neighborSet.end (); neighbor++)
         {
           if (neighbor->neighborMainAddr == nb2hop_tuple.neighborMainAddr
               && neighbor->willingness != manet_dtn_WILL_NEVER)
             {
               nb2hopOk = true;
               break;
             }
         }
       if (!nb2hopOk)
         {
           NS_LOG_LOGIC ("Two-hop neighbor tuple skipped: 2-hop neighbor "
                         << nb2hop_tuple.twoHopNeighborAddr
                         << " is attached to neighbor " << nb2hop_tuple.neighborMainAddr
                         << ", which was not found in the Neighbor Set.");
           continue;
         }
 
       // one selects one 2-hop tuple and creates one entry in the routing table with:
       //                R_dest_addr  =  the main address of the 2-hop neighbor;
       //                R_next_addr  = the R_next_addr of the entry in the
       //                               routing table with:
       //                                   R_dest_addr == N_neighbor_main_addr
       //                                                  of the 2-hop tuple;
       //                R_dist       = 2;
       //                R_iface_addr = the R_iface_addr of the entry in the
       //                               routing table with:
       //                                   R_dest_addr == N_neighbor_main_addr
       //                                                  of the 2-hop tuple;
       RoutingTableEntry entry;
       bool foundEntry = Lookup (nb2hop_tuple.neighborMainAddr, entry);
       if (foundEntry)
         {
           NS_LOG_LOGIC ("Adding routing entry for two-hop neighbor.");
           AddEntry (nb2hop_tuple.twoHopNeighborAddr,
                     entry.nextAddr,
                     entry.interface,
                     2,
                     nb2hop_tuple.position,
                     nb2hop_tuple.vType);
         }
       else
         {
           NS_LOG_LOGIC ("NOT adding routing entry for two-hop neighbor ("
                         << nb2hop_tuple.twoHopNeighborAddr
                         << " not found in the routing table)");
         }
     }
 
   for (uint32_t h = 2;; h++)
     {
       bool added = false;
 
       // 3.1. For each topology entry in the topology table, if its
       // T_dest_addr does not correspond to R_dest_addr of any
       // route entry in the routing table AND its T_last_addr
       // corresponds to R_dest_addr of a route entry whose R_dist
       // is equal to h, then a new route entry MUST be recorded in
       // the routing table (if it does not already exist)
       const TopologySet &topology = m_state.GetTopologySet ();
       for (TopologySet::const_iterator it = topology.begin ();
            it != topology.end (); it++)
         {
           const TopologyTuple &topology_tuple = *it;
           NS_LOG_LOGIC ("Looking at topology tuple: " << topology_tuple);
 
           RoutingTableEntry destAddrEntry, lastAddrEntry;
           bool have_destAddrEntry = Lookup (topology_tuple.destAddr, destAddrEntry);
           bool have_lastAddrEntry = Lookup (topology_tuple.lastAddr, lastAddrEntry);
           if (!have_destAddrEntry && have_lastAddrEntry && lastAddrEntry.distance == h)
             {
               NS_LOG_LOGIC ("Adding routing table entry based on the topology tuple.");
               // then a new route entry MUST be recorded in
               //                the routing table (if it does not already exist) where:
               //                     R_dest_addr  = T_dest_addr;
               //                     R_next_addr  = R_next_addr of the recorded
               //                                    route entry where:
               //                                    R_dest_addr == T_last_addr
               //                     R_dist       = h+1; and
               //                     R_iface_addr = R_iface_addr of the recorded
               //                                    route entry where:
               //                                       R_dest_addr == T_last_addr.
               AddEntry (topology_tuple.destAddr,
                         lastAddrEntry.nextAddr,
                         lastAddrEntry.interface,
                         h + 1,
                         topology_tuple.position,
                         topology_tuple.vType);
               added = true;
             }
           else
             {
               NS_LOG_LOGIC ("NOT adding routing table entry based on the topology tuple: "
                             "have_destAddrEntry=" << have_destAddrEntry
                                                   << " have_lastAddrEntry=" << have_lastAddrEntry
                                                   << " lastAddrEntry.distance=" << (int) lastAddrEntry.distance
                                                   << " (h=" << h << ")");
             }
         }
 
       if (!added)
         {
           break;
         }
     }
 
   // 4. For each entry in the multiple interface association base
   // where there exists a routing entry such that:
   // R_dest_addr == I_main_addr (of the multiple interface association entry)
   // AND there is no routing entry such that:
   // R_dest_addr == I_iface_addr
   const IfaceAssocSet &ifaceAssocSet = m_state.GetIfaceAssocSet ();
   for (IfaceAssocSet::const_iterator it = ifaceAssocSet.begin ();
        it != ifaceAssocSet.end (); it++)
     {
       IfaceAssocTuple const &tuple = *it;
       RoutingTableEntry entry1, entry2;
       bool have_entry1 = Lookup (tuple.mainAddr, entry1);
       bool have_entry2 = Lookup (tuple.ifaceAddr, entry2);
       if (have_entry1 && !have_entry2)
         {
           // then a route entry is created in the routing table with:
           //       R_dest_addr  =  I_iface_addr (of the multiple interface
           //                                     association entry)
           //       R_next_addr  =  R_next_addr  (of the recorded route entry)
           //       R_dist       =  R_dist       (of the recorded route entry)
           //       R_iface_addr =  R_iface_addr (of the recorded route entry).
           AddEntry (tuple.ifaceAddr,
                     entry1.nextAddr,
                     entry1.interface,
                     entry1.distance,
                     entry1.position,
                     entry1.vType);
         }
     }
 
   // 5. For each tuple in the association set,
   //    If there is no entry in the routing table with:
   //        R_dest_addr     == A_network_addr/A_netmask
   //   and if the announced network is not announced by the node itself,
   //   then a new routing entry is created.
   const AssociationSet &associationSet = m_state.GetAssociationSet ();
 
   // Clear HNA routing table
   for (uint32_t i = 0; i < m_hnaRoutingTable->GetNRoutes (); i++)
     {
       m_hnaRoutingTable->RemoveRoute (0);
     }
 
   for (AssociationSet::const_iterator it = associationSet.begin ();
        it != associationSet.end (); it++)
     {
       AssociationTuple const &tuple = *it;
 
       // Test if HNA associations received from other gateways
       // are also announced by this node. In such a case, no route
       // is created for this association tuple (go to the next one).
       bool goToNextAssociationTuple = false;
       const Associations &localHnaAssociations = m_state.GetAssociations ();
       NS_LOG_DEBUG ("Nb local associations: " << localHnaAssociations.size ());
       for (Associations::const_iterator assocIterator = localHnaAssociations.begin ();
            assocIterator != localHnaAssociations.end (); assocIterator++)
         {
           Association const &localHnaAssoc = *assocIterator;
           if (localHnaAssoc.networkAddr == tuple.networkAddr && localHnaAssoc.netmask == tuple.netmask)
             {
               NS_LOG_DEBUG ("HNA association received from another GW is part of local HNA associations: no route added for network "
                             << tuple.networkAddr << "/" << tuple.netmask);
               goToNextAssociationTuple = true;
             }
         }
       if (goToNextAssociationTuple)
         {
           continue;
         }
 
       RoutingTableEntry gatewayEntry;
 
       bool gatewayEntryExists = Lookup (tuple.gatewayAddr, gatewayEntry);
       bool addRoute = false;
 
       uint32_t routeIndex = 0;
 
       for (routeIndex = 0; routeIndex < m_hnaRoutingTable->GetNRoutes (); routeIndex++)
         {
           Ipv4RoutingTableEntry route = m_hnaRoutingTable->GetRoute (routeIndex);
           if (route.GetDestNetwork () == tuple.networkAddr
               && route.GetDestNetworkMask () == tuple.netmask)
             {
               break;
             }
         }
 
       if (routeIndex == m_hnaRoutingTable->GetNRoutes ())
         {
           addRoute = true;
         }
       else if (gatewayEntryExists && m_hnaRoutingTable->GetMetric (routeIndex) > gatewayEntry.distance)
         {
           m_hnaRoutingTable->RemoveRoute (routeIndex);
           addRoute = true;
         }
 
       if (addRoute && gatewayEntryExists)
         {
           m_hnaRoutingTable->AddNetworkRouteTo (tuple.networkAddr,
                                                 tuple.netmask,
                                                 gatewayEntry.nextAddr,
                                                 gatewayEntry.interface,
                                                 gatewayEntry.distance);
 
         }
     }
 
   NS_LOG_DEBUG ("Node " << m_mainAddress << ": RoutingTableComputation end.");
   m_routingTableChanged (GetSize ());
 }
 

 void
 RoutingProtocol::ProcessHello (const manet_dtn::MessageHeader &msg,
                                const Ipv4Address &receiverIface,
                                const Ipv4Address &senderIface)
 {
   NS_LOG_FUNCTION (msg << receiverIface << senderIface);
 
   const manet_dtn::MessageHeader::Hello &hello = msg.GetHello ();
 
   LinkSensing (msg, hello, receiverIface, senderIface);
 
 #ifdef NS3_LOG_ENABLE
   {
     const LinkSet &links = m_state.GetLinks ();
     NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                   << "s ** BEGIN dump Link Set for manet_dtn Node " << m_mainAddress);
     for (LinkSet::const_iterator link = links.begin (); link != links.end (); link++)
       {
         NS_LOG_DEBUG (*link);
       }
     NS_LOG_DEBUG ("** END dump Link Set for manet_dtn Node " << m_mainAddress);
 
     const NeighborSet &neighbors = m_state.GetNeighbors ();
     NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                   << "s ** BEGIN dump Neighbor Set for manet_dtn Node " << m_mainAddress);
     for (NeighborSet::const_iterator neighbor = neighbors.begin (); neighbor != neighbors.end (); neighbor++)
       {
         NS_LOG_DEBUG (*neighbor);
       }
     NS_LOG_DEBUG ("** END dump Neighbor Set for manet_dtn Node " << m_mainAddress);
   }
 #endif // NS3_LOG_ENABLE
 
   PopulateNeighborSet (msg, hello);
   PopulateTwoHopNeighborSet (msg, hello);
 
 #ifdef NS3_LOG_ENABLE
   {
     const TwoHopNeighborSet &twoHopNeighbors = m_state.GetTwoHopNeighbors ();
     NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                   << "s ** BEGIN dump TwoHopNeighbor Set for manet_dtn Node " << m_mainAddress);
     for (TwoHopNeighborSet::const_iterator tuple = twoHopNeighbors.begin ();
          tuple != twoHopNeighbors.end (); tuple++)
       {
         NS_LOG_DEBUG (*tuple);
       }
     NS_LOG_DEBUG ("** END dump TwoHopNeighbor Set for manet_dtn Node " << m_mainAddress);
   }
 #endif // NS3_LOG_ENABLE
 
   MprComputation ();
   PopulateMprSelectorSet (msg, hello);
 }
 SummaryVectorHeader
 RoutingProtocol::GetDisjointSummary(const SummaryVectorHeader listA,/*sender*/
                                     const SummaryVectorHeader listB/*m_queue*/)
  {
    SummaryVectorHeader listC;
    for(std::vector<uint32_t>::const_iterator iter = listA.m_packets.begin();
    iter != listA.m_packets.end(); iter++)
    {
      auto point = std::find(listB.m_packets.begin(),listB.m_packets.end(),*iter);
      if(point == listB.m_packets.end())
      {
        listC.m_packets.push_back(*iter);
      }
    }
    return listC;
  }
 void
 RoutingProtocol::ProcessSV (const manet_dtn::MessageHeader &msg,
                             const Ipv4Address &senderIface)
 {
   NS_LOG_FUNCTION (msg << senderIface);
   const manet_dtn::MessageHeader::SV &sv = msg.GetSv ();
   NS_LOG_FUNCTION ("sv.mpackets.size()" << sv.m_packets.size());
   SummaryVectorHeader recvsummary;
   for(std::vector<uint32_t>::const_iterator iter = sv.m_packets.begin();
   iter != sv.m_packets.end(); iter++)
   {
    recvsummary.m_packets.push_back(*iter);
   }
   /*listA is list of packets held only by sender */
   SummaryVectorHeader listA = GetDisjointSummary(recvsummary,m_queue.GetSummaryVector());
   NS_LOG_FUNCTION ("listA.mpackets.size()" << listA.m_packets.size());
  // listB is list of packets held only by receiver 
   SummaryVectorHeader listB = m_queue.FindDisjointPackets (recvsummary);
  // NS_LOG_FUNCTION ("listB.mpackets.size()" << listB.m_packets.size());
  // NS_LOG_FUNCTION ("m_queue.size()" << m_queue.GetSize());
  if(listA.Size() != 0)
  {
    SendDREQ(senderIface,listA);
  }

   for(std::vector<uint32_t>::const_iterator iter = listB.m_packets.begin();
   iter != listB.m_packets.end(); iter++)
   {
     QueueEntry newEntry = m_queue.Find(*iter);
     if(newEntry.GetPacket())
     {
      Ptr<Packet> packet = ConstCast<Packet> (newEntry.GetPacket());
      std::cout<<"ProcessSV id = " << packet->GetUid() << std::endl;
      Ipv4Header reHeader;
      packet->RemoveHeader(reHeader);
      reHeader.SetSource(m_ipv4->GetAddress (1, 0).GetLocal());
      packet->AddHeader(reHeader);
      UnicastForwardCallback ucb = newEntry.GetUnicastForwardCallback ();
      Ipv4Header header = newEntry.GetIpv4Header ();
      header.SetTtl (header.GetTtl () + 1);
      header.SetPayloadSize (packet->GetSize ());
      Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
      rt->SetSource (header.GetSource ());
      rt->SetDestination (header.GetDestination ());
      rt->SetGateway (senderIface);
      if (m_ipv4->GetInterfaceForAddress (m_mainAddress) != -1)
        {
          rt->SetOutputDevice (m_ipv4->GetNetDevice
                             (m_ipv4->GetInterfaceForAddress (m_mainAddress)));
        }
        Ptr<Packet> pkt = packet->Copy ();
        ucb(rt,pkt,header);
        
    /*for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
          m_sendSockets.begin (); i != m_sendSockets.end (); i++)
       {
         Ptr<Packet> pkt = packet->Copy ();
         i->first->SendTo (pkt, 0, InetSocketAddress (senderIface, manet_dtn_PORT_NUMBER));
       }*/

     }
   }
 }
 void
 RoutingProtocol::ProcessControl(const manet_dtn::MessageHeader &msg,
                               const Ipv4Address &senderIface)
 {
   std::cout<<"aaaaaaaaaaaaaaaaa\n"<<std::endl;
   m_protcol = 1;
   if(not m_manetTimer.IsRunning())
   {
   m_manetTimer.Schedule(m_manetInterval);
   }
 }                       
 void
 RoutingProtocol::ProcessDREQ (const manet_dtn::MessageHeader &msg,
                               const Ipv4Address &senderIface)
 {
   NS_LOG_FUNCTION (msg << senderIface);
   const manet_dtn::MessageHeader::DREQ &dreq = msg.GetDreq ();
   for(std::vector<uint32_t>::const_iterator iter = dreq.m_packets.begin();
   iter != dreq.m_packets.end(); iter++)
   {
   QueueEntry queueEntry = m_queue.Find(*iter);
   Ptr<Packet> packet = ConstCast<Packet> (queueEntry.GetPacket());
   std::cout<<"PROCESSDREQ = " << packet->GetUid() << std::endl;
   ControlTag tag;
   packet->RemovePacketTag(tag);
   ControlTag tempTag(ControlTag::FIRSTDATA);
   packet->AddPacketTag(tempTag);
   UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
   Ipv4Header header = queueEntry.GetIpv4Header ();
   header.SetTtl (header.GetTtl () + 1);
   header.SetPayloadSize (packet->GetSize ());
   Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
   rt->SetSource (header.GetSource ());
   rt->SetDestination (header.GetDestination ());
   rt->SetGateway (senderIface);
   if (m_ipv4->GetInterfaceForAddress (m_mainAddress) != -1)
      {
      rt->SetOutputDevice (m_ipv4->GetNetDevice
                             (m_ipv4->GetInterfaceForAddress (m_mainAddress)));
      }
    Ptr<Packet> pkt = packet->Copy ();
    NS_LOG_DEBUG(rt->GetDestination() <<" " << rt->GetGateway());
    ucb(rt,pkt,header);
   /*for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
          m_sendSockets.begin (); i != m_sendSockets.end (); i++)
       {
         Ptr<Packet> pkt = packet->Copy ();
         
       }*/
   }
 }
 void
 RoutingProtocol::ProcessTc (const manet_dtn::MessageHeader &msg,
                             const Ipv4Address &senderIface)
 {
   NS_LOG_FUNCTION(this);
    const manet_dtn::MessageHeader::Tc &tc = msg.GetTc ();
   Time now = Simulator::Now ();
   // 1. If the sender interface of this message is not in the symmetric
   // 1-hop neighborhood of this node, the message MUST be discarded.
   const LinkTuple *link_tuple = m_state.FindSymLinkTuple (senderIface, now);
   if (link_tuple == NULL)
     {
       return;
     }
 
   // 2. If there exist some tuple in the topology set where:
   //    T_last_addr == originator address AND
   //    T_seq       >  ANSN,
   // then further processing of this TC message MUST NOT be
   // performed.
   const TopologyTuple *topologyTuple =
     m_state.FindNewerTopologyTuple (msg.GetOriginatorAddress (), tc.ansn);
   if (topologyTuple != NULL)
     {
       return;
     }
 
   // 3. All tuples in the topology set where:
   //    T_last_addr == originator address AND
   //    T_seq       <  ANSN
   // MUST be removed from the topology set.
   m_state.EraseOlderTopologyTuples (msg.GetOriginatorAddress (), tc.ansn);
 
   // 4. For each of the advertised neighbor main address received in
   // the TC message:

     for(uint8_t i = 0;i<tc.neighborAddresses.size();i++)
     {
       const Ipv4Address &addr = tc.neighborAddresses[i];
       const uint16_t &position = tc.neighborPositions[i];
       const uint16_t &vType = tc.neighborVTypes[i];
       // 4.1. If there exist some tuple in the topology set where:
       //      T_dest_addr == advertised neighbor main address, AND
       //      T_last_addr == originator address,
       // then the holding time of that tuple MUST be set to:
       //      T_time      =  current time + validity time.
       TopologyTuple *topologyTuple =
         m_state.FindTopologyTuple (addr, msg.GetOriginatorAddress ());
 
       if (topologyTuple != NULL)
         {
           topologyTuple->expirationTime = now + msg.GetVTime ();
         }
       else
         {
           // 4.2. Otherwise, a new tuple MUST be recorded in the topology
           // set where:
           //      T_dest_addr = advertised neighbor main address,
           //      T_last_addr = originator address,
           //      T_seq       = ANSN,
           //      T_time      = current time + validity time.
           TopologyTuple topologyTuple;
           topologyTuple.destAddr = addr;
           topologyTuple.lastAddr = msg.GetOriginatorAddress ();
           topologyTuple.position = position;
           topologyTuple.vType = vType;
           topologyTuple.sequenceNumber = tc.ansn;
           topologyTuple.expirationTime = now + msg.GetVTime ();
           AddTopologyTuple (topologyTuple);
 
           // Schedules topology tuple deletion
           m_events.Track (Simulator::Schedule (DELAY (topologyTuple.expirationTime),
                                                &RoutingProtocol::TopologyTupleTimerExpire,
                                                this,
                                                topologyTuple.destAddr,
                                                topologyTuple.lastAddr));
          }
     }
        
        
 #ifdef NS3_LOG_ENABLE
   {
     const TopologySet &topology = m_state.GetTopologySet ();
     NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                   << "s ** BEGIN dump TopologySet for manet_dtn Node " << m_mainAddress);
     for (TopologySet::const_iterator tuple = topology.begin ();
          tuple != topology.end (); tuple++)
       {
         NS_LOG_DEBUG (*tuple);
       }
     NS_LOG_DEBUG ("** END dump TopologySet Set for manet_dtn Node " << m_mainAddress);
   }
 #endif // NS3_LOG_ENABLE
 }
 
 void
 RoutingProtocol::ProcessMid (const manet_dtn::MessageHeader &msg,
                              const Ipv4Address &senderIface)
 {
   const manet_dtn::MessageHeader::Mid &mid = msg.GetMid ();
   Time now = Simulator::Now ();
 
   NS_LOG_DEBUG ("Node " << m_mainAddress << " ProcessMid from " << senderIface);
   // 1. If the sender interface of this message is not in the symmetric
   // 1-hop neighborhood of this node, the message MUST be discarded.
   const LinkTuple *linkTuple = m_state.FindSymLinkTuple (senderIface, now);
   if (linkTuple == NULL)
     {
       NS_LOG_LOGIC ("Node " << m_mainAddress <<
                     ": the sender interface of this message is not in the "
                     "symmetric 1-hop neighborhood of this node,"
                     " the message MUST be discarded.");
       return;
     }
 
   // 2. For each interface address listed in the MID message
   for (std::vector<Ipv4Address>::const_iterator i = mid.interfaceAddresses.begin ();
        i != mid.interfaceAddresses.end (); i++)
     {
       bool updated = false;
       IfaceAssocSet &ifaceAssoc = m_state.GetIfaceAssocSetMutable ();
       for (IfaceAssocSet::iterator tuple = ifaceAssoc.begin ();
            tuple != ifaceAssoc.end (); tuple++)
         {
           if (tuple->ifaceAddr == *i
               && tuple->mainAddr == msg.GetOriginatorAddress ())
             {
               NS_LOG_LOGIC ("IfaceAssoc updated: " << *tuple);
               tuple->time = now + msg.GetVTime ();
               updated = true;
             }
         }
       if (!updated)
         {
           IfaceAssocTuple tuple;
           tuple.ifaceAddr = *i;
           tuple.mainAddr = msg.GetOriginatorAddress ();
           tuple.time = now + msg.GetVTime ();
           AddIfaceAssocTuple (tuple);
           NS_LOG_LOGIC ("New IfaceAssoc added: " << tuple);
           // Schedules iface association tuple deletion
           Simulator::Schedule (DELAY (tuple.time),
                                &RoutingProtocol::IfaceAssocTupleTimerExpire, this, tuple.ifaceAddr);
         }
     }
 
   // 3. (not part of the RFC) iterate over all NeighborTuple's and
   // TwoHopNeighborTuples, update the neighbor addresses taking into account
   // the new MID information.
   NeighborSet &neighbors = m_state.GetNeighbors ();
   for (NeighborSet::iterator neighbor = neighbors.begin (); neighbor != neighbors.end (); neighbor++)
     {
       neighbor->neighborMainAddr = GetMainAddress (neighbor->neighborMainAddr);
     }
 
   TwoHopNeighborSet &twoHopNeighbors = m_state.GetTwoHopNeighbors ();
   for (TwoHopNeighborSet::iterator twoHopNeighbor = twoHopNeighbors.begin ();
        twoHopNeighbor != twoHopNeighbors.end (); twoHopNeighbor++)
     {
       twoHopNeighbor->neighborMainAddr = GetMainAddress (twoHopNeighbor->neighborMainAddr);
       twoHopNeighbor->twoHopNeighborAddr = GetMainAddress (twoHopNeighbor->twoHopNeighborAddr);
     }
   NS_LOG_DEBUG ("Node " << m_mainAddress << " ProcessMid from " << senderIface << " -> END.");
 }
 
 void
 RoutingProtocol::ProcessHna (const manet_dtn::MessageHeader &msg,
                              const Ipv4Address &senderIface)
 {
 
   const manet_dtn::MessageHeader::Hna &hna = msg.GetHna ();
   Time now = Simulator::Now ();
 
   // 1. If the sender interface of this message is not in the symmetric
   // 1-hop neighborhood of this node, the message MUST be discarded.
   const LinkTuple *link_tuple = m_state.FindSymLinkTuple (senderIface, now);
   if (link_tuple == NULL)
     {
       return;
     }
 
   // 2. Otherwise, for each (network address, netmask) pair in the
   // message:
 
   for (std::vector<manet_dtn::MessageHeader::Hna::Association>::const_iterator it = hna.associations.begin ();
        it != hna.associations.end (); it++)
     {
       AssociationTuple *tuple = m_state.FindAssociationTuple (msg.GetOriginatorAddress (),it->address,it->mask);
 
       // 2.1  if an entry in the association set already exists, where:
       //          A_gateway_addr == originator address
       //          A_network_addr == network address
       //          A_netmask      == netmask
       //      then the holding time for that tuple MUST be set to:
       //          A_time         =  current time + validity time
       if (tuple != NULL)
         {
           tuple->expirationTime = now + msg.GetVTime ();
         }
 
       // 2.2 otherwise, a new tuple MUST be recorded with:
       //          A_gateway_addr =  originator address
       //          A_network_addr =  network address
       //          A_netmask      =  netmask
       //          A_time         =  current time + validity time
       else
         {
           AssociationTuple assocTuple = {
             msg.GetOriginatorAddress (),
             it->address,
             it->mask,
             now + msg.GetVTime ()
           };
           AddAssociationTuple (assocTuple);
 
           //Schedule Association Tuple deletion
           Simulator::Schedule (DELAY (assocTuple.expirationTime),
                                &RoutingProtocol::AssociationTupleTimerExpire, this,
                                assocTuple.gatewayAddr,assocTuple.networkAddr,assocTuple.netmask);
         }
 
     }
 }
 
 void
 RoutingProtocol::ForwardDefault (manet_dtn::MessageHeader manet_dtnMessage,
                                  DuplicateTuple *duplicated,
                                  const Ipv4Address &localIface,
                                  const Ipv4Address &senderAddress)
 {
   Time now = Simulator::Now ();
 
   // If the sender interface address is not in the symmetric
   // 1-hop neighborhood the message must not be forwarded
   const LinkTuple *linkTuple = m_state.FindSymLinkTuple (senderAddress, now);
   if (linkTuple == NULL)
     {
       return;
     }
 
   // If the message has already been considered for forwarding,
   // it must not be retransmitted again
   if (duplicated != NULL && duplicated->retransmitted)
     {
       NS_LOG_LOGIC (Simulator::Now () << "Node " << m_mainAddress << " does not forward a message received"
                     " from " << manet_dtnMessage.GetOriginatorAddress () << " because it is duplicated");
       return;
     }
 
   // If the sender interface address is an interface address
   // of a MPR selector of this node and ttl is greater than 1,
   // the message must be retransmitted
   bool retransmitted = false;
   if (manet_dtnMessage.GetTimeToLive () > 1)
     {
       const MprSelectorTuple *mprselTuple =
         m_state.FindMprSelectorTuple (GetMainAddress (senderAddress));
       if (mprselTuple != NULL)
         {
           manet_dtnMessage.SetTimeToLive (manet_dtnMessage.GetTimeToLive () - 1);
           manet_dtnMessage.SetHopCount (manet_dtnMessage.GetHopCount () + 1);
           // We have to introduce a random delay to avoid
           // synchronization with neighbors.
           QueueMessage (manet_dtnMessage, JITTER);
           retransmitted = true;
         }
     }
 
   // Update duplicate tuple...
   if (duplicated != NULL)
     {
       duplicated->expirationTime = now + manet_dtn_DUP_HOLD_TIME;
       duplicated->retransmitted = retransmitted;
       duplicated->ifaceList.push_back (localIface);
     }
   // ...or create a new one
   else
     {
       DuplicateTuple newDup;
       newDup.address = manet_dtnMessage.GetOriginatorAddress ();
       newDup.sequenceNumber = manet_dtnMessage.GetMessageSequenceNumber ();
       newDup.expirationTime = now + manet_dtn_DUP_HOLD_TIME;
       newDup.retransmitted = retransmitted;
       newDup.ifaceList.push_back (localIface);
       AddDuplicateTuple (newDup);
       // Schedule dup tuple deletion
       Simulator::Schedule (manet_dtn_DUP_HOLD_TIME,
                            &RoutingProtocol::DupTupleTimerExpire, this,
                            newDup.address, newDup.sequenceNumber);
     }
 }
 
 void
 RoutingProtocol::QueueMessage (const manet_dtn::MessageHeader &message, Time delay)
 {
   m_queuedMessages.push_back (message);
   if (not m_queuedMessagesTimer.IsRunning ())
     {
       m_queuedMessagesTimer.SetDelay (delay);
       m_queuedMessagesTimer.Schedule ();
     }
 }
 
 void
 RoutingProtocol::MprPacketQueue (const Ptr<Packet> packet, Time delay)
 {
   MprPacket = packet->Copy();
   ControlTag tag;
   MprPacket->RemovePacketTag(tag);
   ControlTag temptag(ControlTag::BROADCASTDATA);
   MprPacket->AddPacketTag(temptag);
   if (not m_queuedPacketTimer.IsRunning ())
     {
       m_queuedPacketTimer.SetDelay (delay);
       m_queuedPacketTimer.Schedule ();
     }   
 }
 void
 RoutingProtocol::MprBroadcast()
 {
   
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
          m_sendSockets.begin (); i != m_sendSockets.end (); i++)
     {
       Ptr<Packet> pkt = MprPacket->Copy ();
       Ipv4Address bcast = i->second.GetLocal ().GetSubnetDirectedBroadcast (i->second.GetMask ());
       i->first->SendTo (pkt, 0, InetSocketAddress (bcast, manet_dtn_PORT_NUMBER));
     }
 }
 void
 RoutingProtocol::SendPacket (Ptr<Packet> packet,
                              const MessageList &containedMessages)
 {
   NS_LOG_DEBUG ("manet_dtn node " << m_mainAddress << " sending a manet_dtn packet");
 
   // Add a header
   manet_dtn::PacketHeader header;
   header.SetPacketLength (header.GetSerializedSize () + packet->GetSize ());
   header.SetPacketSequenceNumber (GetPacketSequenceNumber ());
   packet->AddHeader (header);
   ControlTag tempTag(ControlTag::CONTROL);
   packet->AddPacketTag(tempTag);
   // Trace it
   m_txPacketTrace (header, containedMessages);
 //  MessageHeader messageHeader;
   // Send it
   for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
          m_sendSockets.begin (); i != m_sendSockets.end (); i++)
     {
       Ptr<Packet> pkt = packet->Copy ();
       Ipv4Address bcast = i->second.GetLocal ().GetSubnetDirectedBroadcast (i->second.GetMask ());
       i->first->SendTo (pkt, 0, InetSocketAddress (bcast, manet_dtn_PORT_NUMBER));
       /*Message Count*/
   /*    pkt->RemoveHeader(messageHeader);
       switch (messageHeader.GetMessageType ())
       {
        case manet_dtn::MessageHeader::HELLO_MESSAGE:
        helloCount++;
        break;
        case manet_dtn::MessageHeader::TC_MESSAGE:
        tcCount++;
        break;
        case manet_dtn::MessageHeader::SV_MESSAGE:
        svCount++;
        break;
        case manet_dtn::MessageHeader::CONTROL_MESSAGE:
        controlCount++;
        break;  
        default:
        break;                      
       }*/
     }
 }
 
 void
 RoutingProtocol::SendQueuedMessages ()
 {
   Ptr<Packet> packet = Create<Packet> ();
   int numMessages = 0;
   NS_LOG_DEBUG ("manet_dtn node " << m_mainAddress << ": SendQueuedMessages");
 
   MessageList msglist;
 
   for (std::vector<manet_dtn::MessageHeader>::const_iterator message = m_queuedMessages.begin ();
        message != m_queuedMessages.end ();
        message++)
     {
       Ptr<Packet> p = Create<Packet> ();
       p->AddHeader (*message);
       packet->AddAtEnd (p);
       msglist.push_back (*message);
       if (++numMessages == manet_dtn_MAX_MSGS)
         {
           SendPacket (packet, msglist);
           msglist.clear ();
           // Reset variables for next packet
           numMessages = 0;
           packet = Create<Packet> ();
         }
     }
 
   if (packet->GetSize ())
     {
       SendPacket (packet, msglist);
     }
 
   m_queuedMessages.clear ();
 }
 void
 RoutingProtocol::SendControlMessage ()
 {
  controlCount_send++;
  manet_dtn::MessageHeader msg;
  msg.SetVTime (manet_dtn_TOP_HOLD_TIME);
  msg.SetOriginatorAddress (m_mainAddress);
  msg.SetTimeToLive (2);
  msg.SetHopCount (0);
  msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());
  manet_dtn::MessageHeader::CONTROL &control = msg.GetControl ();
  control.acAddress = m_mainAddress;
  overhead += msg.GetSerializedSize();
  QueueMessage (msg, JITTER);
 }
 void
 RoutingProtocol::SendHello ()
 {
   helloCount_send++;
   NS_LOG_FUNCTION (this);
   manet_dtn::MessageHeader msg;
   Time now = Simulator::Now ();
   msg.SetVTime (manet_dtn_NEIGHB_HOLD_TIME);
   msg.SetOriginatorAddress (m_mainAddress);
   msg.SetTimeToLive (1);
   msg.SetHopCount (0);
   msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());
   Vector position = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel>()->GetPosition();
   manet_dtn::MessageHeader::Hello &hello = msg.GetHello ();
   hello.SetHTime (m_helloInterval);
   hello.willingness = m_willingness;
   hello.SetPosition(uint16_t(position.x));
 /*  if(m_ipv4->GetObject<Node>()->GetId() == 6 || m_ipv4->GetObject<Node>()->GetId() == 16)
   hello.SetVtype(1);
   else*/
     hello.SetVtype(0);
   std::vector<manet_dtn::MessageHeader::Hello::LinkMessage>
   &linkMessages = hello.linkMessages;
   
   const LinkSet &links = m_state.GetLinks ();
   for (LinkSet::const_iterator link_tuple = links.begin ();
        link_tuple != links.end (); link_tuple++)
     {
       if (!(GetMainAddress (link_tuple->localIfaceAddr) == m_mainAddress
             && link_tuple->time >= now))
         {
           continue;
         }
 
       uint8_t link_type, nb_type = 0xff;
 
       // Establishes link type
       if (link_tuple->symTime >= now)
         {
           link_type = manet_dtn_SYM_LINK;
         }
       else if (link_tuple->asymTime >= now)
         {
           link_type = manet_dtn_ASYM_LINK;
         }
       else
         {
           link_type = manet_dtn_LOST_LINK;
         }
       // Establishes neighbor type.
       if (m_state.FindMprAddress (GetMainAddress (link_tuple->neighborIfaceAddr)))
         {
           nb_type = manet_dtn_MPR_NEIGH;
           NS_LOG_DEBUG ("I consider neighbor " << GetMainAddress (link_tuple->neighborIfaceAddr)
                                                << " to be MPR_NEIGH.");
         }
       else
         {
           bool ok = false;
           for (NeighborSet::const_iterator nb_tuple = m_state.GetNeighbors ().begin ();
                nb_tuple != m_state.GetNeighbors ().end ();
                nb_tuple++)
             {
               if (nb_tuple->neighborMainAddr == GetMainAddress (link_tuple->neighborIfaceAddr))
                 {
                   if (nb_tuple->status == NeighborTuple::STATUS_SYM)
                     {
                       NS_LOG_DEBUG ("I consider neighbor " << GetMainAddress (link_tuple->neighborIfaceAddr)
                                                            << " to be SYM_NEIGH.");
                       nb_type = manet_dtn_SYM_NEIGH;
                     }
                   else if (nb_tuple->status == NeighborTuple::STATUS_NOT_SYM)
                     {
                       nb_type = manet_dtn_NOT_NEIGH;
                       NS_LOG_DEBUG ("I consider neighbor " << GetMainAddress (link_tuple->neighborIfaceAddr)
                                                            << " to be NOT_NEIGH.");
                     }
                   else
                     {
                       NS_FATAL_ERROR ("There is a neighbor tuple with an unknown status!\n");
                     }
                   ok = true;
                   break;
                 }
             }
           if (!ok)
             {
               NS_LOG_WARN ("I don't know the neighbor " << GetMainAddress (link_tuple->neighborIfaceAddr) << "!!!");
               continue;
             }
         }
 
       manet_dtn::MessageHeader::Hello::LinkMessage linkMessage;
       linkMessage.linkCode = (link_type & 0x03) | ((nb_type << 2) & 0x0f);
       linkMessage.neighborInterfaceAddresses.push_back
         (link_tuple->neighborIfaceAddr);
         linkMessage.neighborPositions.push_back(link_tuple->position);
         linkMessage.neighborVTypes.push_back(link_tuple->vType); 
       std::vector<Ipv4Address> interfaces =
         m_state.FindNeighborInterfaces (link_tuple->neighborIfaceAddr);
 
       linkMessage.neighborInterfaceAddresses.insert
         (linkMessage.neighborInterfaceAddresses.end (),
         interfaces.begin (), interfaces.end ());
 
       linkMessages.push_back (linkMessage);
     }
   NS_LOG_DEBUG ("manet_dtn HELLO message size: " << int (msg.GetSerializedSize ())
                                             << " (with " << int (linkMessages.size ()) << " link messages)");
   overhead += msg.GetSerializedSize();
   QueueMessage (msg, JITTER);
 }
 void
 RoutingProtocol::SendDREQ (Ipv4Address dest,SummaryVectorHeader sm)
 {
   derqCount_send++;
   NS_LOG_DEBUG(dest << "にデータ要求");
  manet_dtn::MessageHeader msg;
  msg.SetVTime (manet_dtn_TOP_HOLD_TIME);
  msg.SetOriginatorAddress (m_mainAddress);
  msg.SetTimeToLive (1);
  msg.SetHopCount (0);
  msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());
  manet_dtn::MessageHeader::DREQ &dreq = msg.GetDreq ();
  for(std::vector<uint32_t>::const_iterator iter = sm.m_packets.begin();
  iter != sm.m_packets.end(); iter++)
  {
    dreq.m_packets.push_back(*iter);
  }
  Ptr<Packet> p = Create<Packet> ();
  std::cout<<"SENDDREQ = " << p->GetUid() << std::endl;
  p->AddHeader (msg);
  Ptr<Packet> packet = Create<Packet> ();
  packet->AddAtEnd (p);
  ControlTag tempTag(ControlTag::CONTROL);
  packet->AddPacketTag(tempTag);
  manet_dtn::PacketHeader header;
  header.SetPacketLength (header.GetSerializedSize () + packet->GetSize ());
  header.SetPacketSequenceNumber (GetPacketSequenceNumber ());
  packet->AddHeader (header);
  Ptr<Packet> pkt = packet->Copy ();
     for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
          m_sendSockets.begin (); i != m_sendSockets.end (); i++)
     {
       overhead += pkt->GetSize();
       i->first->SendTo (pkt, 0, InetSocketAddress (dest, manet_dtn_PORT_NUMBER));
     }
 }
 void
 RoutingProtocol::SendSummaryVector ()
 {
  svCount_send++;
  Ptr<Packet> packet_summary = Create<Packet> ();
  manet_dtn::MessageHeader msg;
  msg.SetVTime (manet_dtn_TOP_HOLD_TIME);
  msg.SetOriginatorAddress (m_mainAddress);
  msg.SetTimeToLive (1);
  msg.SetHopCount (0);
  msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());

  manet_dtn::MessageHeader::SV &sv = msg.GetSv ();
  SummaryVectorHeader summary = m_queue.GetSummaryVector();
  for(std::vector<uint32_t>::const_iterator i = summary.m_packets.begin();
  i != summary.m_packets.end(); i++)
  {
    sv.m_packets.push_back(*i);
  }
  overhead += msg.GetSerializedSize();
  QueueMessage (msg,JITTER);
 }

 void
 RoutingProtocol::SendTc ()
 {
   tcCount_send++;
  NS_LOG_FUNCTION (this);
   manet_dtn::MessageHeader msg;
   msg.SetVTime (manet_dtn_TOP_HOLD_TIME);
   msg.SetOriginatorAddress (m_mainAddress);
   msg.SetTimeToLive (255);
   msg.SetHopCount (0);
   msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());
 
   manet_dtn::MessageHeader::Tc &tc = msg.GetTc ();
   tc.ansn = m_ansn;
   for (MprSelectorSet::const_iterator mprsel_tuple = m_state.GetMprSelectors ().begin ();
        mprsel_tuple != m_state.GetMprSelectors ().end (); mprsel_tuple++)
     {
       tc.neighborAddresses.push_back (mprsel_tuple->mainAddr);
       tc.neighborPositions.push_back(mprsel_tuple->position);
       tc.neighborVTypes.push_back(mprsel_tuple->vType);
     }
   overhead += msg.GetSerializedSize();
   QueueMessage (msg, JITTER);
 }
 
 void
 RoutingProtocol::SendMid ()
 {
   manet_dtn::MessageHeader msg;
   manet_dtn::MessageHeader::Mid &mid = msg.GetMid ();
 
   // A node which has only a single interface address participating in
   // the MANET (i.e., running manet_dtn), MUST NOT generate any MID
   // message.
 
   // A node with several interfaces, where only one is participating
   // in the MANET and running manet_dtn (e.g., a node is connected to a
   // wired network as well as to a MANET) MUST NOT generate any MID
   // messages.
 
   // A node with several interfaces, where more than one is
   // participating in the MANET and running manet_dtn MUST generate MID
   // messages as specified.
 
   // [ Note: assuming here that all interfaces participate in the
   // MANET; later we may want to make this configurable. ]
 
   Ipv4Address loopback ("127.0.0.1");
   for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
     {
       Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
       if (addr != m_mainAddress && addr != loopback && m_interfaceExclusions.find (i) == m_interfaceExclusions.end ())
         {
           mid.interfaceAddresses.push_back (addr);
         }
     }
   if (mid.interfaceAddresses.size () == 0)
     {
       return;
     }
 
   msg.SetVTime (manet_dtn_MID_HOLD_TIME);
   msg.SetOriginatorAddress (m_mainAddress);
   msg.SetTimeToLive (255);
   msg.SetHopCount (0);
   msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());
 
   QueueMessage (msg, JITTER);
 }
 
 void
 RoutingProtocol::SendHna ()
 {
 
   manet_dtn::MessageHeader msg;
 
   msg.SetVTime (manet_dtn_HNA_HOLD_TIME);
   msg.SetOriginatorAddress (m_mainAddress);
   msg.SetTimeToLive (255);
   msg.SetHopCount (0);
   msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());
   manet_dtn::MessageHeader::Hna &hna = msg.GetHna ();
 
   std::vector<manet_dtn::MessageHeader::Hna::Association> &associations = hna.associations;
 
   // Add all local HNA associations to the HNA message
   const Associations &localHnaAssociations = m_state.GetAssociations ();
   for (Associations::const_iterator it = localHnaAssociations.begin ();
        it != localHnaAssociations.end (); it++)
     {
       manet_dtn::MessageHeader::Hna::Association assoc = { it->networkAddr, it->netmask};
       associations.push_back (assoc);
     }
   // If there is no HNA associations to send, return without queuing the message
   if (associations.size () == 0)
     {
       return;
     }
 
   // Else, queue the message to be sent later on
   QueueMessage (msg, JITTER);
 }
 
 void
 RoutingProtocol::AddHostNetworkAssociation (Ipv4Address networkAddr, Ipv4Mask netmask)
 {
   // Check if the (networkAddr, netmask) tuple already exist
   // in the list of local HNA associations
   const Associations &localHnaAssociations = m_state.GetAssociations ();
   for (Associations::const_iterator assocIterator = localHnaAssociations.begin ();
        assocIterator != localHnaAssociations.end (); assocIterator++)
     {
       Association const &localHnaAssoc = *assocIterator;
       if (localHnaAssoc.networkAddr == networkAddr && localHnaAssoc.netmask == netmask)
         {
           NS_LOG_INFO ("HNA association for network " << networkAddr << "/" << netmask << " already exists.");
           return;
         }
     }
   // If the tuple does not already exist, add it to the list of local HNA associations.
   NS_LOG_INFO ("Adding HNA association for network " << networkAddr << "/" << netmask << ".");
   m_state.InsertAssociation ( (Association) { networkAddr, netmask} );
 }
 
 void
 RoutingProtocol::RemoveHostNetworkAssociation (Ipv4Address networkAddr, Ipv4Mask netmask)
 {
   NS_LOG_INFO ("Removing HNA association for network " << networkAddr << "/" << netmask << ".");
   m_state.EraseAssociation ( (Association) { networkAddr, netmask} );
 }
 
 void
 RoutingProtocol::SetRoutingTableAssociation (Ptr<Ipv4StaticRouting> routingTable)
 {
   // If a routing table has already been associated, remove
   // corresponding entries from the list of local HNA associations
   if (m_routingTableAssociation != 0)
     {
       NS_LOG_INFO ("Removing HNA entries coming from the old routing table association.");
       for (uint32_t i = 0; i < m_routingTableAssociation->GetNRoutes (); i++)
         {
           Ipv4RoutingTableEntry route = m_routingTableAssociation->GetRoute (i);
           // If the outgoing interface for this route is a non-manet_dtn interface
           if (UsesNonManetDtnOutgoingInterface (route))
             {
               // remove the corresponding entry
               RemoveHostNetworkAssociation (route.GetDestNetwork (), route.GetDestNetworkMask ());
             }
         }
     }
 
   // Sets the routingTableAssociation to its new value
   m_routingTableAssociation = routingTable;
 
   // Iterate over entries of the associated routing table and
   // add the routes using non-manet_dtn outgoing interfaces to the list
   // of local HNA associations
   NS_LOG_DEBUG ("Nb local associations before adding some entries from"
                 " the associated routing table: " << m_state.GetAssociations ().size ());
   for (uint32_t i = 0; i < m_routingTableAssociation->GetNRoutes (); i++)
     {
       Ipv4RoutingTableEntry route = m_routingTableAssociation->GetRoute (i);
       Ipv4Address destNetworkAddress = route.GetDestNetwork ();
       Ipv4Mask destNetmask = route.GetDestNetworkMask ();
 
       // If the outgoing interface for this route is a non-manet_dtn interface,
       if (UsesNonManetDtnOutgoingInterface (route))
         {
           // Add this entry's network address and netmask to the list of local HNA entries
           AddHostNetworkAssociation (destNetworkAddress, destNetmask);
         }
     }
   NS_LOG_DEBUG ("Nb local associations after having added some entries from "
                 "the associated routing table: " << m_state.GetAssociations ().size ());
 }
 
 bool
 RoutingProtocol::UsesNonManetDtnOutgoingInterface (const Ipv4RoutingTableEntry &route)
 {
   std::set<uint32_t>::const_iterator ci = m_interfaceExclusions.find (route.GetInterface ());
   // The outgoing interface is a non-manet_dtn interface if a match is found
   // before reaching the end of the list of excluded interfaces
   return ci != m_interfaceExclusions.end ();
 }
 
 void
 RoutingProtocol::LinkSensing (const manet_dtn::MessageHeader &msg,
                               const manet_dtn::MessageHeader::Hello &hello,
                               const Ipv4Address &receiverIface,
                               const Ipv4Address &senderIface)
 {
   Time now = Simulator::Now ();
   bool updated = false;
   bool created = false;
   NS_LOG_DEBUG ("@" << now.GetSeconds () << ": Olsr node " << m_mainAddress
                     << ": LinkSensing(receiverIface=" << receiverIface
                     << ", senderIface=" << senderIface << ") BEGIN");
 
   NS_ASSERT (msg.GetVTime () > Seconds (0));
   LinkTuple *link_tuple = m_state.FindLinkTuple (senderIface);
   if (link_tuple == NULL)
     {
       LinkTuple newLinkTuple;
       // We have to create a new tuple
       newLinkTuple.neighborIfaceAddr = senderIface;
       newLinkTuple.localIfaceAddr = receiverIface;
       newLinkTuple.position = hello.GetPosition(); 
       newLinkTuple.vType = hello.GetVtype();
       newLinkTuple.symTime = now - Seconds (1);
       newLinkTuple.time = now + msg.GetVTime ();
       link_tuple = &m_state.InsertLinkTuple (newLinkTuple);
       created = true;
       NS_LOG_LOGIC ("Existing link tuple did not exist => creating new one");
     }
   else
     {
       NS_LOG_LOGIC ("Existing link tuple already exists => will update it");
       updated = true;
     }
 
   link_tuple->asymTime = now + msg.GetVTime ();
   for (std::vector<manet_dtn::MessageHeader::Hello::LinkMessage>::const_iterator linkMessage =
          hello.linkMessages.begin ();
        linkMessage != hello.linkMessages.end ();
        linkMessage++)
     {
       int lt = linkMessage->linkCode & 0x03; // Link Type
       int nt = (linkMessage->linkCode >> 2) & 0x03; // Neighbor Type
 
 #ifdef NS3_LOG_ENABLE
       const char *linkTypeName;
       switch (lt)
         {
         case manet_dtn_UNSPEC_LINK:
           linkTypeName = "UNSPEC_LINK";
           break;
         case manet_dtn_ASYM_LINK:
           linkTypeName = "ASYM_LINK";
           break;
         case manet_dtn_SYM_LINK:
           linkTypeName = "SYM_LINK";
           break;
         case manet_dtn_LOST_LINK:
           linkTypeName = "LOST_LINK";
           break;
         default: linkTypeName = "(invalid value!)";
           
         }
 
       const char *neighborTypeName;
       switch (nt)
         {
         case manet_dtn_NOT_NEIGH:
           neighborTypeName = "NOT_NEIGH";
           break;
         case manet_dtn_SYM_NEIGH:
           neighborTypeName = "SYM_NEIGH";
           break;
         case manet_dtn_MPR_NEIGH:
           neighborTypeName = "MPR_NEIGH";
           break;
         default:
           neighborTypeName = "(invalid value!)";
         }
 
       NS_LOG_DEBUG ("Looking at HELLO link messages with Link Type "
                     << lt << " (" << linkTypeName
                     << ") and Neighbor Type " << nt
                     << " (" << neighborTypeName << ")");
 #endif // NS3_LOG_ENABLE
 
       // We must not process invalid advertised links
       if ((lt == manet_dtn_SYM_LINK && nt == manet_dtn_NOT_NEIGH)
           || (nt != manet_dtn_SYM_NEIGH && nt != manet_dtn_MPR_NEIGH
               && nt != manet_dtn_NOT_NEIGH))
         {
           NS_LOG_LOGIC ("HELLO link code is invalid => IGNORING");
           continue;
         }
 
       for (std::vector<Ipv4Address>::const_iterator neighIfaceAddr =
              linkMessage->neighborInterfaceAddresses.begin ();
            neighIfaceAddr != linkMessage->neighborInterfaceAddresses.end ();
            neighIfaceAddr++)
         {
           NS_LOG_DEBUG ("   -> Neighbor: " << *neighIfaceAddr);
           if (*neighIfaceAddr == receiverIface)
             {
               if (lt == manet_dtn_LOST_LINK)
                 {
                   NS_LOG_LOGIC ("link is LOST => expiring it");
                   link_tuple->symTime = now - Seconds (1);
                   updated = true;
                 }
               else if (lt == manet_dtn_SYM_LINK || lt == manet_dtn_ASYM_LINK)
                 {
                   NS_LOG_DEBUG (*link_tuple << ": link is SYM or ASYM => should become SYM now"
                                 " (symTime being increased to " << now + msg.GetVTime ());
                   link_tuple->symTime = now + msg.GetVTime ();
                   link_tuple->time = link_tuple->symTime + manet_dtn_NEIGHB_HOLD_TIME;
                   updated = true;
                 }
               else
                 {
                   NS_FATAL_ERROR ("bad link type");
                 }
               break;
             }
           else
             {
               NS_LOG_DEBUG ("     \\-> *neighIfaceAddr (" << *neighIfaceAddr
                                                           << " != receiverIface (" << receiverIface << ") => IGNORING!");
             }
         }
       NS_LOG_DEBUG ("Link tuple updated: " << int (updated));
     }
   link_tuple->time = std::max (link_tuple->time, link_tuple->asymTime);
 
   if (updated)
     {
       LinkTupleUpdated (*link_tuple, hello.willingness);
     }
 
   // Schedules link tuple deletion
   if (created)
     {
       LinkTupleAdded (*link_tuple, hello.willingness);
       m_events.Track (Simulator::Schedule (DELAY (std::min (link_tuple->time, link_tuple->symTime)),
                                            &RoutingProtocol::LinkTupleTimerExpire, this,
                                            link_tuple->neighborIfaceAddr));
     }
   NS_LOG_DEBUG ("@" << now.GetSeconds () << ": Olsr node " << m_mainAddress
                     << ": LinkSensing END");
 }
 
 void
 RoutingProtocol::PopulateNeighborSet (const manet_dtn::MessageHeader &msg,
                                       const manet_dtn::MessageHeader::Hello &hello)
 {
  NeighborTuple *nb_tuple = m_state.FindNeighborTuple (msg.GetOriginatorAddress ());
   if (nb_tuple != NULL)
     {
       nb_tuple->willingness = hello.willingness;
       nb_tuple->position = hello.GetPosition();
       nb_tuple->vType = hello.GetVtype();
     }
 }
 
 void
 RoutingProtocol::PopulateTwoHopNeighborSet (const manet_dtn::MessageHeader &msg,
                                             const manet_dtn::MessageHeader::Hello &hello)
 {
   Time now = Simulator::Now ();
 
   NS_LOG_DEBUG ("Olsr node " << m_mainAddress << ": PopulateTwoHopNeighborSet BEGIN");
 
   for (LinkSet::const_iterator link_tuple = m_state.GetLinks ().begin ();
        link_tuple != m_state.GetLinks ().end (); link_tuple++)
     {
       NS_LOG_LOGIC ("Looking at link tuple: " << *link_tuple);
       if (GetMainAddress (link_tuple->neighborIfaceAddr) != msg.GetOriginatorAddress ())
         {
           NS_LOG_LOGIC ("Link tuple ignored: "
                         "GetMainAddress (link_tuple->neighborIfaceAddr) != msg.GetOriginatorAddress ()");
           NS_LOG_LOGIC ("(GetMainAddress(" << link_tuple->neighborIfaceAddr << "): "
                                            << GetMainAddress (link_tuple->neighborIfaceAddr)
                                            << "; msg.GetOriginatorAddress (): " << msg.GetOriginatorAddress ());
           continue;
         }
 
       if (link_tuple->symTime < now)
         {
           NS_LOG_LOGIC ("Link tuple ignored: expired.");
           continue;
         }
 
       typedef std::vector<manet_dtn::MessageHeader::Hello::LinkMessage> LinkMessageVec;
       for (LinkMessageVec::const_iterator linkMessage = hello.linkMessages.begin ();
            linkMessage != hello.linkMessages.end (); linkMessage++)
         {
           int neighborType = (linkMessage->linkCode >> 2) & 0x3;
 #ifdef NS3_LOG_ENABLE
           const char *neighborTypeNames[3] = { "NOT_NEIGH", "SYM_NEIGH", "MPR_NEIGH" };
           const char *neighborTypeName = ((neighborType < 3) ?
                                           neighborTypeNames[neighborType]
                                           : "(invalid value)");
           NS_LOG_DEBUG ("Looking at Link Message from HELLO message: neighborType="
                         << neighborType << " (" << neighborTypeName << ")");
 #endif // NS3_LOG_ENABLE


          /* for (std::vector<Ipv4Address>::const_iterator nb2hop_addr_iter = 
                  linkMessage->neighborInterfaceAddresses.begin ();
                nb2hop_addr_iter != linkMessage->neighborInterfaceAddresses.end ();
                nb2hop_addr_iter++)
             {*/
          for(uint8_t i = 0; i < linkMessage->neighborInterfaceAddresses.size(); i++)
          {

               Ipv4Address nb2hop_addr = GetMainAddress (linkMessage->neighborInterfaceAddresses[i]);//*nb2hop_addr_iter);
               NS_LOG_DEBUG ("Looking at 2-hop neighbor address from HELLO message: "
                             << linkMessage->neighborInterfaceAddresses[i]//*nb2hop_addr_iter
                             << " (main address is " << nb2hop_addr << ")");
               if (neighborType == manet_dtn_SYM_NEIGH || neighborType == manet_dtn_MPR_NEIGH)
                 {
                   // If the main address of the 2-hop neighbor address == main address
                   // of the receiving node, silently discard the 2-hop
                   // neighbor address.
                   if (nb2hop_addr == m_mainAddress)
                     {
                       NS_LOG_LOGIC ("Ignoring 2-hop neighbor (it is the node itself)");
                       continue;
                     }
 
                   // Otherwise, a 2-hop tuple is created
                   TwoHopNeighborTuple *nb2hop_tuple =
                     m_state.FindTwoHopNeighborTuple (msg.GetOriginatorAddress (), nb2hop_addr);
                   NS_LOG_LOGIC ("Adding the 2-hop neighbor"
                                 << (nb2hop_tuple ? " (refreshing existing entry)" : ""));
                   if (nb2hop_tuple == NULL)
                     {
                       TwoHopNeighborTuple new_nb2hop_tuple;
                       new_nb2hop_tuple.neighborMainAddr = msg.GetOriginatorAddress ();
                       new_nb2hop_tuple.twoHopNeighborAddr = linkMessage->neighborInterfaceAddresses[i];
                       new_nb2hop_tuple.position = linkMessage->neighborPositions[i];
                       new_nb2hop_tuple.vType = linkMessage->neighborVTypes[i];
                       new_nb2hop_tuple.expirationTime = now + msg.GetVTime ();
                       AddTwoHopNeighborTuple (new_nb2hop_tuple);
                       // Schedules nb2hop tuple deletion
                       m_events.Track (Simulator::Schedule (DELAY (new_nb2hop_tuple.expirationTime),
                                                            &RoutingProtocol::Nb2hopTupleTimerExpire, this,
                                                            new_nb2hop_tuple.neighborMainAddr,
                                                            new_nb2hop_tuple.twoHopNeighborAddr));
                     }
                   else
                     {
                       nb2hop_tuple->expirationTime = now + msg.GetVTime ();
                     }
                 }
               else if (neighborType == manet_dtn_NOT_NEIGH)
                 {
                   // For each 2-hop node listed in the HELLO message
                   // with Neighbor Type equal to NOT_NEIGH all 2-hop
                   // tuples where: N_neighbor_main_addr == Originator
                   // Address AND N_2hop_addr == main address of the
                   // 2-hop neighbor are deleted.
                   NS_LOG_LOGIC ("2-hop neighbor is NOT_NEIGH => deleting matching 2-hop neighbor state");
                   m_state.EraseTwoHopNeighborTuples (msg.GetOriginatorAddress (), nb2hop_addr);
                 }
               else
                 {
                   NS_LOG_LOGIC ("*** WARNING *** Ignoring link message (inside HELLO) with bad"
                                 " neighbor type value: " << neighborType);
                 }
             }
         }
     }
 
   NS_LOG_DEBUG ("Olsr node " << m_mainAddress << ": PopulateTwoHopNeighborSet END");
 }
 
 void
 RoutingProtocol::PopulateMprSelectorSet (const manet_dtn::MessageHeader &msg,
                                          const manet_dtn::MessageHeader::Hello &hello)
 {
    NS_LOG_FUNCTION (this);
 
   Time now = Simulator::Now ();
 
   typedef std::vector<manet_dtn::MessageHeader::Hello::LinkMessage> LinkMessageVec;
   for (LinkMessageVec::const_iterator linkMessage = hello.linkMessages.begin ();
        linkMessage != hello.linkMessages.end ();
        linkMessage++)
     {
       int nt = linkMessage->linkCode >> 2;
       if (nt == manet_dtn_MPR_NEIGH)
         {
           NS_LOG_DEBUG ("Processing a link message with neighbor type MPR_NEIGH");
 
           for (std::vector<Ipv4Address>::const_iterator nb_iface_addr =
                  linkMessage->neighborInterfaceAddresses.begin ();
                nb_iface_addr != linkMessage->neighborInterfaceAddresses.end ();
                nb_iface_addr++)
             {
               if (GetMainAddress (*nb_iface_addr) == m_mainAddress)
                 {
                   NS_LOG_DEBUG ("Adding entry to mpr selector set for neighbor " << *nb_iface_addr);
 
                   // We must create a new entry into the mpr selector set
                   MprSelectorTuple *existing_mprsel_tuple =
                     m_state.FindMprSelectorTuple (msg.GetOriginatorAddress ());
                   if (existing_mprsel_tuple == NULL)
                     {
                       MprSelectorTuple mprsel_tuple;
 
                       mprsel_tuple.mainAddr = msg.GetOriginatorAddress ();
                       mprsel_tuple.position = hello.GetPosition();
                       mprsel_tuple.vType = hello.GetVtype();
                       mprsel_tuple.expirationTime = now + msg.GetVTime ();
                       AddMprSelectorTuple (mprsel_tuple);
 
                       // Schedules mpr selector tuple deletion
                       m_events.Track (Simulator::Schedule
                                         (DELAY (mprsel_tuple.expirationTime),
                                         &RoutingProtocol::MprSelTupleTimerExpire, this,
                                         mprsel_tuple.mainAddr));
                     }
                   else
                     {
                       existing_mprsel_tuple->expirationTime = now + msg.GetVTime ();
                     }
                 }
             }
         }
     }
   NS_LOG_DEBUG ("Computed MPR selector set for node " << m_mainAddress << ": " << m_state.PrintMprSelectorSet ());
 }
 
 
 #if 0
 void
 manet_dtn::mac_failed (Ptr<Packet> p)
 {
   double now              = Simulator::Now ();
   struct hdr_ip* ih       = HDR_IP (p);
   struct hdr_cmn* ch      = HDR_CMN (p);
 
   debug ("%f: Node %d MAC Layer detects a breakage on link to %d\n",
          now,
          manet_dtn::node_id (ra_addr ()),
          manet_dtn::node_id (ch->next_hop ()));
 
   if ((u_int32_t)ih->daddr () == IP_BROADCAST)
     {
       drop (p, DROP_RTR_MAC_CALLBACK);
       return;
     }
 
   manet_dtn_link_tuple* link_tuple = state_.find_link_tuple (ch->next_hop ());
   if (link_tuple != NULL)
     {
       link_tuple->lost_time () = now + manet_dtn_NEIGHB_HOLD_TIME;
       link_tuple->time ()      = now + manet_dtn_NEIGHB_HOLD_TIME;
       nb_loss (link_tuple);
     }
   drop (p, DROP_RTR_MAC_CALLBACK);
 }
 #endif
 
 
 
 
 void
 RoutingProtocol::NeighborLoss (const LinkTuple &tuple)
 {
   NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                 << "s: manet_dtn Node " << m_mainAddress
                 << " LinkTuple " << tuple.neighborIfaceAddr << " -> neighbor loss.");
   LinkTupleUpdated (tuple, manet_dtn_WILL_DEFAULT);
   m_state.EraseTwoHopNeighborTuples (GetMainAddress (tuple.neighborIfaceAddr));
   m_state.EraseMprSelectorTuples (GetMainAddress (tuple.neighborIfaceAddr));
 
   MprComputation ();
   RoutingTableComputation ();
 }
 
 void
 RoutingProtocol::AddDuplicateTuple (const DuplicateTuple &tuple)
 {
   /*debug("%f: Node %d adds dup tuple: addr = %d seq_num = %d\n",
           Simulator::Now (),
           manet_dtn::node_id(ra_addr()),
           manet_dtn::node_id(tuple->addr()),
           tuple->seq_num());*/
   m_state.InsertDuplicateTuple (tuple);
 }
 
 void
 RoutingProtocol::RemoveDuplicateTuple (const DuplicateTuple &tuple)
 {
   /*debug("%f: Node %d removes dup tuple: addr = %d seq_num = %d\n",
     Simulator::Now (),
     manet_dtn::node_id(ra_addr()),
     manet_dtn::node_id(tuple->addr()),
     tuple->seq_num());*/
   m_state.EraseDuplicateTuple (tuple);
 }
 
 void
 RoutingProtocol::LinkTupleAdded (const LinkTuple &tuple, uint8_t willingness)
 {
   // Creates associated neighbor tuple
   NeighborTuple nb_tuple;
   nb_tuple.neighborMainAddr = GetMainAddress (tuple.neighborIfaceAddr);
   nb_tuple.willingness = willingness;
 
   if (tuple.symTime >= Simulator::Now ())
     {
       nb_tuple.status = NeighborTuple::STATUS_SYM;
     }
   else
     {
       nb_tuple.status = NeighborTuple::STATUS_NOT_SYM;
     }
 
   AddNeighborTuple (nb_tuple);
 }
 
 void
 RoutingProtocol::RemoveLinkTuple (const LinkTuple &tuple)
 {
   NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                 << "s: manet_dtn Node " << m_mainAddress
                 << " LinkTuple " << tuple << " REMOVED.");
 
   m_state.EraseNeighborTuple (GetMainAddress (tuple.neighborIfaceAddr));
   m_state.EraseLinkTuple (tuple);
 }
 
 void
 RoutingProtocol::LinkTupleUpdated (const LinkTuple &tuple, uint8_t willingness)
 {
   // Each time a link tuple changes, the associated neighbor tuple must be recomputed
 
   NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                 << "s: manet_dtn Node " << m_mainAddress
                 << " LinkTuple " << tuple << " UPDATED.");
 
   NeighborTuple *nb_tuple =
     m_state.FindNeighborTuple (GetMainAddress (tuple.neighborIfaceAddr));
 
   if (nb_tuple == NULL)
     {
       LinkTupleAdded (tuple, willingness);
       nb_tuple = m_state.FindNeighborTuple (GetMainAddress (tuple.neighborIfaceAddr));
     }
 
   if (nb_tuple != NULL)
     {
       int statusBefore = nb_tuple->status;
 
       bool hasSymmetricLink = false;
 
       const LinkSet &linkSet = m_state.GetLinks ();
       for (LinkSet::const_iterator it = linkSet.begin ();
            it != linkSet.end (); it++)
         {
           const LinkTuple &link_tuple = *it;
           if (GetMainAddress (link_tuple.neighborIfaceAddr) == nb_tuple->neighborMainAddr
               && link_tuple.symTime >= Simulator::Now ())
             {
               hasSymmetricLink = true;
               break;
             }
         }
 
       if (hasSymmetricLink)
         {
           nb_tuple->status = NeighborTuple::STATUS_SYM;
           NS_LOG_DEBUG (*nb_tuple << "->status = STATUS_SYM; changed:"
                                   << int (statusBefore != nb_tuple->status));
         }
       else
         {
           nb_tuple->status = NeighborTuple::STATUS_NOT_SYM;
           NS_LOG_DEBUG (*nb_tuple << "->status = STATUS_NOT_SYM; changed:"
                                   << int (statusBefore != nb_tuple->status));
         }
     }
   else
     {
       NS_LOG_WARN ("ERROR! Wanted to update a NeighborTuple but none was found!");
     }
 }
 
 void
 RoutingProtocol::AddNeighborTuple (const NeighborTuple &tuple)
 {
 //   debug("%f: Node %d adds neighbor tuple: nb_addr = %d status = %s\n",
 //         Simulator::Now (),
 //         manet_dtn::node_id(ra_addr()),
 //         manet_dtn::node_id(tuple->neighborMainAddr),
 //         ((tuple->status() == manet_dtn_STATUS_SYM) ? "sym" : "not_sym"));
 
   m_state.InsertNeighborTuple (tuple);
   IncrementAnsn ();
 }
 
 void
 RoutingProtocol::RemoveNeighborTuple (const NeighborTuple &tuple)
 {
 //   debug("%f: Node %d removes neighbor tuple: nb_addr = %d status = %s\n",
 //         Simulator::Now (),
 //         manet_dtn::node_id(ra_addr()),
 //         manet_dtn::node_id(tuple->neighborMainAddr),
 //         ((tuple->status() == manet_dtn_STATUS_SYM) ? "sym" : "not_sym"));
 
   m_state.EraseNeighborTuple (tuple);
   IncrementAnsn ();
 }
 
 void
 RoutingProtocol::AddTwoHopNeighborTuple (const TwoHopNeighborTuple &tuple)
 {
 //   debug("%f: Node %d adds 2-hop neighbor tuple: nb_addr = %d nb2hop_addr = %d\n",
 //         Simulator::Now (),
 //         manet_dtn::node_id(ra_addr()),
 //         manet_dtn::node_id(tuple->neighborMainAddr),
 //         manet_dtn::node_id(tuple->twoHopNeighborAddr));
 
   m_state.InsertTwoHopNeighborTuple (tuple);
 }
 
 void
 RoutingProtocol::RemoveTwoHopNeighborTuple (const TwoHopNeighborTuple &tuple)
 {
 //   debug("%f: Node %d removes 2-hop neighbor tuple: nb_addr = %d nb2hop_addr = %d\n",
 //         Simulator::Now (),
 //         manet_dtn::node_id(ra_addr()),
 //         manet_dtn::node_id(tuple->neighborMainAddr),
 //         manet_dtn::node_id(tuple->twoHopNeighborAddr));
 
   m_state.EraseTwoHopNeighborTuple (tuple);
 }
 
 void
 RoutingProtocol::IncrementAnsn ()
 {
   m_ansn = (m_ansn + 1) % (manet_dtn_MAX_SEQ_NUM + 1);
 }
 
 void
 RoutingProtocol::AddMprSelectorTuple (const MprSelectorTuple  &tuple)
 {
 //   debug("%f: Node %d adds MPR selector tuple: nb_addr = %d\n",
 //         Simulator::Now (),
 //         manet_dtn::node_id(ra_addr()),
 //         manet_dtn::node_id(tuple->main_addr()));
 
   m_state.InsertMprSelectorTuple (tuple);
   IncrementAnsn ();
 }
 
 void
 RoutingProtocol::RemoveMprSelectorTuple (const MprSelectorTuple &tuple)
 {
 //   debug("%f: Node %d removes MPR selector tuple: nb_addr = %d\n",
 //         Simulator::Now (),
 //         manet_dtn::node_id(ra_addr()),
 //         manet_dtn::node_id(tuple->main_addr()));
 
   m_state.EraseMprSelectorTuple (tuple);
   IncrementAnsn ();
 }
 
 void
 RoutingProtocol::AddTopologyTuple (const TopologyTuple &tuple)
 {
 //   debug("%f: Node %d adds topology tuple: dest_addr = %d last_addr = %d seq = %d\n",
 //         Simulator::Now (),
 //         manet_dtn::node_id(ra_addr()),
 //         manet_dtn::node_id(tuple->dest_addr()),
 //         manet_dtn::node_id(tuple->last_addr()),
 //         tuple->seq());
 
   m_state.InsertTopologyTuple (tuple);
 }
 
 void
 RoutingProtocol::RemoveTopologyTuple (const TopologyTuple &tuple)
 {
 //   debug("%f: Node %d removes topology tuple: dest_addr = %d last_addr = %d seq = %d\n",
 //         Simulator::Now (),
 //         manet_dtn::node_id(ra_addr()),
 //         manet_dtn::node_id(tuple->dest_addr()),
 //         manet_dtn::node_id(tuple->last_addr()),
 //         tuple->seq());
 
   m_state.EraseTopologyTuple (tuple);
 }
 
 void
 RoutingProtocol::AddIfaceAssocTuple (const IfaceAssocTuple &tuple)
 {
 //   debug("%f: Node %d adds iface association tuple: main_addr = %d iface_addr = %d\n",
 //         Simulator::Now (),
 //         manet_dtn::node_id(ra_addr()),
 //         manet_dtn::node_id(tuple->main_addr()),
 //         manet_dtn::node_id(tuple->iface_addr()));
 
   m_state.InsertIfaceAssocTuple (tuple);
 }
 
 void
 RoutingProtocol::RemoveIfaceAssocTuple (const IfaceAssocTuple &tuple)
 {
 //   debug("%f: Node %d removes iface association tuple: main_addr = %d iface_addr = %d\n",
 //         Simulator::Now (),
 //         manet_dtn::node_id(ra_addr()),
 //         manet_dtn::node_id(tuple->main_addr()),
 //         manet_dtn::node_id(tuple->iface_addr()));
 
   m_state.EraseIfaceAssocTuple (tuple);
 }
 
 void
 RoutingProtocol::AddAssociationTuple (const AssociationTuple &tuple)
 {
   m_state.InsertAssociationTuple (tuple);
 }
 
 void
 RoutingProtocol::RemoveAssociationTuple (const AssociationTuple &tuple)
 {
   m_state.EraseAssociationTuple (tuple);
 }
 
 uint16_t RoutingProtocol::GetPacketSequenceNumber ()
 {
   m_packetSequenceNumber = (m_packetSequenceNumber + 1) % (manet_dtn_MAX_SEQ_NUM + 1);
   return m_packetSequenceNumber;
 }
 
 uint16_t RoutingProtocol::GetMessageSequenceNumber ()
 {
   m_messageSequenceNumber = (m_messageSequenceNumber + 1) % (manet_dtn_MAX_SEQ_NUM + 1);
   return m_messageSequenceNumber;
 }
 
 void
 RoutingProtocol::ManetTimeExpire ()
 {
   /**/
   m_protcol = 0;
 }
 void
 RoutingProtocol::ACMessageTimeExpire ()
 {
   if(m_vType == 1)
      SendControlMessage();
    m_acTimer.Schedule (m_controlInterval);
 }
 void
 RoutingProtocol::HelloTimerExpire ()
 {
   /*If Vehicle Type is AC & MANET-MC act*/
   if(m_protcol == 1)
   SendHello ();
   m_helloTimer.Schedule (m_helloInterval);
 }
 void
 RoutingProtocol::SVTimerExpire ()
 {
   /*If Vehicle Type is DTN-MC act*/
   if(m_protcol == 0)
   {
   SendSummaryVector ();
   m_svTimer.Schedule (m_svInterval);
   }
 }
 
 void
 RoutingProtocol::TcTimerExpire ()
 {
   if(m_protcol == 1)
   {
   if (m_state.GetMprSelectors ().size () > 0)
     {
       SendTc ();
     }
   else
     {
       NS_LOG_DEBUG ("Not sending any TC, no one selected me as MPR.");
     }
   m_tcTimer.Schedule (m_tcInterval);
   }
 }
 
 void
 RoutingProtocol::MidTimerExpire ()
 {
   SendMid ();
   m_midTimer.Schedule (m_midInterval);
 }
 
 void
 RoutingProtocol::HnaTimerExpire ()
 {
   if (m_state.GetAssociations ().size () > 0)
     {
       SendHna ();
     }
   else
     {
       NS_LOG_DEBUG ("Not sending any HNA, no associations to advertise.");
     }
   m_hnaTimer.Schedule (m_hnaInterval);
 }
 
 void
 RoutingProtocol::DupTupleTimerExpire (Ipv4Address address, uint16_t sequenceNumber)
 {
   DuplicateTuple *tuple =
     m_state.FindDuplicateTuple (address, sequenceNumber);
   if (tuple == NULL)
     {
       return;
     }
   if (tuple->expirationTime < Simulator::Now ())
     {
       RemoveDuplicateTuple (*tuple);
     }
   else
     {
       m_events.Track (Simulator::Schedule (DELAY (tuple->expirationTime),
                                            &RoutingProtocol::DupTupleTimerExpire, this,
                                            address, sequenceNumber));
     }
 }
 
 void
 RoutingProtocol::LinkTupleTimerExpire (Ipv4Address neighborIfaceAddr)
 {
   Time now = Simulator::Now ();
 
   // the tuple parameter may be a stale copy; get a newer version from m_state
   LinkTuple *tuple = m_state.FindLinkTuple (neighborIfaceAddr);
   if (tuple == NULL)
     {
       return;
     }
   if (tuple->time < now)
     {
       RemoveLinkTuple (*tuple);
     }
   else if (tuple->symTime < now)
     {
       if (m_linkTupleTimerFirstTime)
         {
           m_linkTupleTimerFirstTime = false;
         }
       else
         {
           NeighborLoss (*tuple);
         }
 
       m_events.Track (Simulator::Schedule (DELAY (tuple->time),
                                            &RoutingProtocol::LinkTupleTimerExpire, this,
                                            neighborIfaceAddr));
     }
   else
     {
       m_events.Track (Simulator::Schedule (DELAY (std::min (tuple->time, tuple->symTime)),
                                            &RoutingProtocol::LinkTupleTimerExpire, this,
                                            neighborIfaceAddr));
     }
 }
 
 void
 RoutingProtocol::Nb2hopTupleTimerExpire (Ipv4Address neighborMainAddr, Ipv4Address twoHopNeighborAddr)
 {
   TwoHopNeighborTuple *tuple;
   tuple = m_state.FindTwoHopNeighborTuple (neighborMainAddr, twoHopNeighborAddr);
   if (tuple == NULL)
     {
       return;
     }
   if (tuple->expirationTime < Simulator::Now ())
     {
       RemoveTwoHopNeighborTuple (*tuple);
     }
   else
     {
       m_events.Track (Simulator::Schedule (DELAY (tuple->expirationTime),
                                            &RoutingProtocol::Nb2hopTupleTimerExpire,
                                            this, neighborMainAddr, twoHopNeighborAddr));
     }
 }
 
 void
 RoutingProtocol::MprSelTupleTimerExpire (Ipv4Address mainAddr)
 {
   MprSelectorTuple *tuple = m_state.FindMprSelectorTuple (mainAddr);
   if (tuple == NULL)
     {
       return;
     }
   if (tuple->expirationTime < Simulator::Now ())
     {
       RemoveMprSelectorTuple (*tuple);
     }
   else
     {
       m_events.Track (Simulator::Schedule (DELAY (tuple->expirationTime),
                                            &RoutingProtocol::MprSelTupleTimerExpire,
                                            this, mainAddr));
     }
 }
 
 void
 RoutingProtocol::TopologyTupleTimerExpire (Ipv4Address destAddr, Ipv4Address lastAddr)
 {
   TopologyTuple *tuple = m_state.FindTopologyTuple (destAddr, lastAddr);
   if (tuple == NULL)
     {
       return;
     }
   if (tuple->expirationTime < Simulator::Now ())
     {
       RemoveTopologyTuple (*tuple);
     }
   else
     {
       m_events.Track (Simulator::Schedule (DELAY (tuple->expirationTime),
                                            &RoutingProtocol::TopologyTupleTimerExpire,
                                            this, tuple->destAddr, tuple->lastAddr));
     }
 }
 
 void
 RoutingProtocol::IfaceAssocTupleTimerExpire (Ipv4Address ifaceAddr)
 {
   IfaceAssocTuple *tuple = m_state.FindIfaceAssocTuple (ifaceAddr);
   if (tuple == NULL)
     {
       return;
     }
   if (tuple->time < Simulator::Now ())
     {
       RemoveIfaceAssocTuple (*tuple);
     }
   else
     {
       m_events.Track (Simulator::Schedule (DELAY (tuple->time),
                                            &RoutingProtocol::IfaceAssocTupleTimerExpire,
                                            this, ifaceAddr));
     }
 }
 
 void
 RoutingProtocol::AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask)
 {
   AssociationTuple *tuple = m_state.FindAssociationTuple (gatewayAddr, networkAddr, netmask);
   if (tuple == NULL)
     {
       return;
     }
   if (tuple->expirationTime < Simulator::Now ())
     {
       RemoveAssociationTuple (*tuple);
     }
   else
     {
       m_events.Track (Simulator::Schedule (DELAY (tuple->expirationTime),
                                            &RoutingProtocol::AssociationTupleTimerExpire,
                                            this, gatewayAddr, networkAddr, netmask));
     }
 }
 
 void
 RoutingProtocol::Clear ()
 {
   NS_LOG_FUNCTION_NOARGS ();
   m_table.clear ();
 }
 
 void
 RoutingProtocol::RemoveEntry (Ipv4Address const &dest)
 {
   m_table.erase (dest);
 }
 
 bool
 RoutingProtocol::Lookup (Ipv4Address const &dest,
                          RoutingTableEntry &outEntry) const
 {
   // Get the iterator at "dest" position
   std::map<Ipv4Address, RoutingTableEntry>::const_iterator it =
     m_table.find (dest);
   // If there is no route to "dest", return NULL
   if (it == m_table.end ())
     {
       return false;
     }
   outEntry = it->second;
   return true;
 }
 
 bool
 RoutingProtocol::FindSendEntry (RoutingTableEntry const &entry,
                                 RoutingTableEntry &outEntry) const
 {
   outEntry = entry;
   while (outEntry.destAddr != outEntry.nextAddr)
     {
       if (not Lookup (outEntry.nextAddr, outEntry))
         {
           return false;
         }
     }
   return true;
 }

 Ptr<Ipv4Route>
 RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
 {
   RoutingTableEntry entry1, entry2; 
   Ptr<Ipv4Route> rt = Create<Ipv4Route>();
//ALL-DTN 
/*
if(true)
{
    ControlTag tag;
  p->PeekPacketTag(tag);
  Ptr<Ipv4Route> rt = Create<Ipv4Route>();
  if(tag.GetTagType() == ControlTag::CONTROL)
  {
    rt->SetSource(m_mainAddress);
    rt->SetGateway(header.GetDestination());
    rt->SetOutputDevice (m_ipv4->GetNetDevice (1));
  }
  else
  {
  rt->SetSource (m_mainAddress);
  rt->SetDestination (m_mainAddress);
  rt->SetGateway (m_mainAddress);
  rt->SetOutputDevice (m_ipv4->GetNetDevice (1));
  }
  return rt;
}*/

 Ipv4Address lastposition_vehicle;
 uint16_t lastposition = 0;

 for(std::map<Ipv4Address, RoutingTableEntry>::const_iterator it = m_table.begin();
     it != m_table.end(); it++)
     {
       if(lastposition < it->second.position)
       {
         lastposition_vehicle = it->first;
         lastposition = it->second.position;
       }
     }
  std::cout<<"lastposition_vehicle = " << lastposition_vehicle << std::endl;
  ControlTag tag;
  p->PeekPacketTag(tag);
  NS_LOG_INFO(header.GetSource() << " " << header.GetDestination() << " " << tag.GetTagType());
  
  
  /*if p is a DREQ message*/
  if(tag.GetTagType() == ControlTag::CONTROL)
  {
    rt->SetSource(m_mainAddress);
    rt->SetGateway(header.GetDestination());
    rt->SetOutputDevice (m_ipv4->GetNetDevice (1));
  }
  else
  {
    QueueEntry newEntry;
    std::cout<<"p->GetUid() = " << p->GetUid() << std::endl;
    newEntry.SetPacketID(p->GetUid());
    newEntry.SetExpireTime(Time(Seconds(5000) + Simulator::Now()));
    m_queue.Enqueue(newEntry);
  /*  Ipv4Header reHeader;
    p->RemoveHeader(reHeader);
    reHeader.SetTtl(header.GetTtl());
    p->AddHeader(reHeader);
    */
    MprPacketQueue(p,JITTER);
    if(Lookup (header.GetDestination (), entry1) != 0)
    {
      bool foundSendEntry = FindSendEntry (entry1, entry2);
      if (!foundSendEntry) NS_FATAL_ERROR ("FindSendEntry failure");
      uint32_t interfaceIdx = entry2.interface;
       if (oif && m_ipv4->GetInterfaceForDevice (oif) != static_cast<int> (interfaceIdx))
         {
           sockerr = Socket::ERROR_NOROUTETOHOST;
           return rt;
         }
         rt = Create<Ipv4Route> ();
         rt->SetDestination (header.GetDestination ());
         NS_ASSERT (m_ipv4);
         uint32_t numOifAddresses = m_ipv4->GetNAddresses (interfaceIdx);
         NS_ASSERT (numOifAddresses > 0);
         Ipv4InterfaceAddress ifAddr;
         if (numOifAddresses == 1)
           {
             ifAddr = m_ipv4->GetAddress (interfaceIdx, 0);
           }
         else
           {
             NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing and OLSR");
           }
         rt->SetSource (ifAddr.GetLocal ());
         rt->SetGateway (entry2.nextAddr);
         rt->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
         sockerr = Socket::ERROR_NOTERROR;
      }
      else
      {
        Lookup(lastposition_vehicle,entry1);
        bool foundSendEntry = FindSendEntry (entry1, entry2);
       if (!foundSendEntry)
         {
           NS_FATAL_ERROR ("FindSendEntry failure");
         }
       uint32_t interfaceIdx = entry2.interface;
       if (oif && m_ipv4->GetInterfaceForDevice (oif) != static_cast<int> (interfaceIdx))
         {
           sockerr = Socket::ERROR_NOROUTETOHOST;
           return rt;
         }
       rt = Create<Ipv4Route> ();
       rt->SetDestination (lastposition_vehicle);
       NS_ASSERT (m_ipv4);
       uint32_t numOifAddresses = m_ipv4->GetNAddresses (interfaceIdx);
       NS_ASSERT (numOifAddresses > 0);
       Ipv4InterfaceAddress ifAddr;
       if (numOifAddresses == 1)
         {
           ifAddr = m_ipv4->GetAddress (interfaceIdx, 0);
         }
       else
         {
           NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing and OLSR");
         }
       rt->SetSource (ifAddr.GetLocal ());
       rt->SetGateway (entry2.nextAddr);
       rt->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
       sockerr = Socket::ERROR_NOTERROR;
      }
  }

  return rt;
 }
 
 bool RoutingProtocol::RouteInput  (Ptr<const Packet> p,
                                    const Ipv4Header &header, Ptr<const NetDevice> idev,
                                    UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                    LocalDeliverCallback lcb, ErrorCallback ecb)
 {
   NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId () << " " << header.GetDestination ());
 Ipv4Address lastposition_vehicle;
 uint16_t lastposition = 0;
 bool forward = false;
  Vector position = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel>()->GetPosition();
 /*自車両より最短後方に位置するACとMANET内最後方の車両を検索*/
 for(std::map<Ipv4Address, RoutingTableEntry>::const_iterator it = m_table.begin();
     it != m_table.end(); it++)
     {
       if(it->second.position > lastposition)
       {
         lastposition = it->second.position;
         lastposition_vehicle = it->first;
       }
         if(position.x > it->second.position)
          {
          forward = true;
          }
     }

  Ptr<Packet> packet = p->Copy();
  ControlTag tag;
  packet->RemovePacketTag(tag);
  p->PeekPacketTag(tag);
  uint32_t iif = m_ipv4->GetInterfaceForDevice (idev);
  uint32_t interfaceIdx;
  uint32_t numOifAddresses;
  Ipv4InterfaceAddress ifAddr;
  Ptr<Ipv4Route> rt;
  RoutingTableEntry entry1, entry2;
  int NodeNumber = m_ipv4->GetObject<Node>()->GetId();
  /*DTN node RouteInput Function*/
  if(m_protcol == 0)
  {
    /*If p is a data packet, Add Queue and Measure time*/
    if(tag.GetTagType() != ControlTag::CONTROL)
    {
      QueueEntry newEntry(packet,header,ucb,ecb);
      newEntry.SetPacketID(packet->GetUid());
      newEntry.SetExpireTime(Time(Seconds(5000) + Simulator::Now()));
      /*If newEntry is New data packet*/
      std::cout<<"packet->GetUid() = " << packet->GetUid() << std::endl;
      if(m_queue.Find(packet->GetUid()).GetPacketID() == 0)
      {
        m_queue.Enqueue(newEntry);
        recvCount[NodeNumber]++;
        NS_LOG_INFO(header.GetSource() << " " << header.GetDestination());
        std::cout <<"Delay: "<<Simulator::Now() - Seconds(20) << " DTNNode " << NodeNumber << " Position: " << 
        m_ipv4->GetObject<Node>()->GetObject<MobilityModel>()->GetPosition().x<<  std::endl;
      }
    }
    /*if p is a control packet, local call back*/
    else
    {
      lcb(packet,header,iif);
    }
    return true;
  }
  /*MANET node RouteInput Function*/
  else
  {
    /*If p is a data packet*/
    if(tag.GetTagType() != ControlTag::CONTROL)
    {
     // std::cout<<"lastpotision_Vehicle = " << lastposition_vehicle << std::endl;
      QueueEntry newEntry(packet,header,ucb,ecb);
      newEntry.SetPacketID(packet->GetUid());
      newEntry.SetExpireTime(Time(Seconds(5000) + Simulator::Now()));

      std::cout<<"packet->GetUid() = " << packet->GetUid() << std::endl;
      if(m_queue.Find(packet->GetUid()).GetPacketID() == 0)
      {
        m_queue.Enqueue(newEntry);
        recvCount[NodeNumber]++;
        
        std::cout<<"Delay: "<<Simulator::Now() - Seconds(20) << " MANETNode " << NodeNumber << " Position: " << 
        m_ipv4->GetObject<Node>()->GetObject<MobilityModel>()->GetPosition().x<<  std::endl;
       // if(m_mainAddress == "10.0.0.42") return true;
      }
      if(tag.GetTagType() == ControlTag::BROADCASTDATA)
        {
        const MprSelectorTuple *mprselTuple = m_state.FindMprSelectorTuple (GetMainAddress (header.GetSource()));
        if(mprselTuple != NULL && !retransmitted[NodeNumber])
          {
          retransmitted[NodeNumber] = true;
          Ipv4Header reHeader;
          packet->RemoveHeader(reHeader);
          reHeader.SetTtl(255);
          reHeader.SetSource(m_ipv4->GetAddress (1, 0).GetLocal());
          packet->AddHeader(reHeader);
          MprPacketQueue(packet,JITTER);
          return true;
          }
        }     
        /*If MANET node receives new data for DTN node, broadcast packet*/
        if(tag.GetTagType() == ControlTag::FIRSTDATA)
        {
          Ipv4Header reHeader;
          packet->RemoveHeader(reHeader);
          reHeader.SetTtl(255);
          reHeader.SetSource(m_ipv4->GetAddress (1, 0).GetLocal());
          packet->AddHeader(reHeader);
          MprPacketQueue(packet,JITTER);
        }
      if(Lookup(header.GetDestination(),entry1) != 0)
      {
        Lookup(header.GetDestination(),entry1);
        bool foundSendEntry = FindSendEntry (entry1, entry2);
            if (!foundSendEntry)
              {
                NS_FATAL_ERROR ("FindSendEntry failure");
              }
            rt = Create<Ipv4Route> ();
            rt->SetDestination (header.GetDestination());
            interfaceIdx = entry2.interface;
            NS_ASSERT (m_ipv4);
            numOifAddresses = m_ipv4->GetNAddresses (interfaceIdx);
            NS_ASSERT (numOifAddresses > 0);
            if (numOifAddresses == 1)
              {
                ifAddr = m_ipv4->GetAddress (interfaceIdx, 0);
              }
            else
              {
                NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing and OLSR");
              }
            rt->SetSource (ifAddr.GetLocal ());
            rt->SetGateway (entry2.nextAddr);
            rt->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
            ucb (rt, p, header); 
            return true;        
      }
      else
      {
        if(!forward)
        {
          return true;
        }
        Lookup(lastposition_vehicle,entry1);
        bool foundSendEntry = FindSendEntry (entry1, entry2);
        if (!foundSendEntry)
          {
            NS_FATAL_ERROR ("FindSendEntry failure");
          }
        rt = Create<Ipv4Route> ();
        rt->SetDestination (lastposition_vehicle);
        interfaceIdx = entry2.interface;
        NS_ASSERT (m_ipv4);
        numOifAddresses = m_ipv4->GetNAddresses (interfaceIdx);
        NS_ASSERT (numOifAddresses > 0);
        if (numOifAddresses == 1)
          {
            ifAddr = m_ipv4->GetAddress (interfaceIdx, 0);
          }
        else
          {
            NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing and OLSR");
          }
        rt->SetSource (ifAddr.GetLocal ());
        rt->SetGateway (entry2.nextAddr);
        rt->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
        ucb (rt, p, header);
        return true;
          }
      }
    /*if p is a control packet*/
    else
    {
      lcb(packet,header,iif);
    }
    return true;
  }
 }

 void
 RoutingProtocol::NotifyInterfaceUp (uint32_t i)
 {
 }
 void
 RoutingProtocol::NotifyInterfaceDown (uint32_t i)
 {
 }
 void
 RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
 {
 }
 void
 RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
 {
 }
 
 
 void
 RoutingProtocol::AddEntry (Ipv4Address const &dest,
                            Ipv4Address const &next,
                            uint32_t interface,
                            uint32_t distance,
                            uint16_t position,
                            uint16_t vType)
 {
   NS_LOG_FUNCTION (this << dest << next << interface << distance << m_mainAddress);
 
   NS_ASSERT (distance > 0);
 
   // Creates a new rt entry with specified values
   RoutingTableEntry &entry = m_table[dest];
 
   entry.destAddr = dest;
   entry.nextAddr = next;
   entry.interface = interface;
   entry.distance = distance;
   entry.position = position;
   entry.vType = vType;
 }
 
 void
 RoutingProtocol::AddEntry (Ipv4Address const &dest,
                            Ipv4Address const &next,
                            Ipv4Address const &interfaceAddress,
                            uint32_t distance,
                            uint16_t position,
                            uint16_t vType)
 {
   NS_LOG_FUNCTION (this << dest << next << interfaceAddress << distance << m_mainAddress);
 
   NS_ASSERT (distance > 0);
   NS_ASSERT (m_ipv4);
 
   RoutingTableEntry entry;
   for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
     {
       for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); j++)
         {
           if (m_ipv4->GetAddress (i,j).GetLocal () == interfaceAddress)
             {
               AddEntry (dest, next, i, distance,position,vType);
               return;
             }
         }
     }
   NS_ASSERT (false); // should not be reached
   AddEntry (dest, next, 0, distance,position,vType);
 }
 
 
 std::vector<RoutingTableEntry>
 RoutingProtocol::GetRoutingTableEntries () const
 {
   std::vector<RoutingTableEntry> retval;
   for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator iter = m_table.begin ();
        iter != m_table.end (); iter++)
     {
       retval.push_back (iter->second);
     }
   return retval;
 }
 
 int64_t
 RoutingProtocol::AssignStreams (int64_t stream)
 {
   NS_LOG_FUNCTION (this << stream);
   m_uniformRandomVariable->SetStream (stream);
   return 1;
 }
 
 bool
 RoutingProtocol::IsMyOwnAddress (const Ipv4Address & a) const
 {
   std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j;
   for (j = m_sendSockets.begin (); j != m_sendSockets.end (); ++j)
     {
       Ipv4InterfaceAddress iface = j->second;
       if (a == iface.GetLocal ())
         {
           return true;
         }
     }
   return false;
 }
 
 void
 RoutingProtocol::Dump (void)
 {
 #ifdef NS3_LOG_ENABLE
   Time now = Simulator::Now ();
   NS_LOG_DEBUG ("Dumping for node with main address " << m_mainAddress);
   NS_LOG_DEBUG (" Neighbor set");
   for (NeighborSet::const_iterator iter = m_state.GetNeighbors ().begin ();
        iter != m_state.GetNeighbors ().end (); iter++)
     {
       NS_LOG_DEBUG ("  " << *iter);
     }
   NS_LOG_DEBUG (" Two-hop neighbor set");
   for (TwoHopNeighborSet::const_iterator iter = m_state.GetTwoHopNeighbors ().begin ();
        iter != m_state.GetTwoHopNeighbors ().end (); iter++)
     {
       if (now < iter->expirationTime)
         {
           NS_LOG_DEBUG ("  " << *iter);
         }
     }
   NS_LOG_DEBUG (" Routing table");
   for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator iter = m_table.begin (); iter != m_table.end (); iter++)
     {
       NS_LOG_DEBUG ("  dest=" << iter->first << " --> next=" << iter->second.nextAddr << " via interface " << iter->second.interface);
     }
   NS_LOG_DEBUG ("");
 #endif  //NS3_LOG_ENABLE
 }
 
 Ptr<const Ipv4StaticRouting>
 RoutingProtocol::GetRoutingTableAssociation () const
 {
   return m_hnaRoutingTable;
 }
 
 } // namespace manet_dtn
 } // namespace ns3
 
 