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
 
 #ifndef OLSR_AGENT_IMPL_H
 #define OLSR_AGENT_IMPL_H
 
 #include "manet_dtn-header.h"
 #include "manet_dtn-packet-queue.h"
 #include "manet_dtn-packet.h"
 #include "manet_dtn-tag.h"
 #include "ns3/test.h"
 #include "manet_dtn-state.h"
 #include "manet_dtn-repositories.h"
 
 #include "ns3/object.h"
 #include "ns3/packet.h"
 #include "ns3/node.h"
 #include "ns3/socket.h"
 #include "ns3/event-garbage-collector.h"
 #include "ns3/random-variable-stream.h"
 #include "ns3/timer.h"
 #include "ns3/traced-callback.h"
 #include "ns3/ipv4.h"
 #include "ns3/ipv4-routing-protocol.h"
 #include "ns3/ipv4-static-routing.h"
 
 #include <vector>
 #include <map>
 
 class Manet_DtnMprTestCase;
 
 namespace ns3 {
 namespace manet_dtn {
 
 
 struct RoutingTableEntry
 {
   Ipv4Address destAddr; 
   Ipv4Address nextAddr; 
   uint32_t interface; 
   uint32_t distance; 
   uint16_t position;
   uint16_t vType; 
   RoutingTableEntry () : // default values
     destAddr (), nextAddr (),
     interface (0), distance (0),
     position (0), vType (0)
   {
   }
 };
 
 class RoutingProtocol;
 
 class RoutingProtocol : public Ipv4RoutingProtocol
 {
 public:
   friend class ::Manet_DtnMprTestCase;
 
   static TypeId GetTypeId (void);
 
   RoutingProtocol ();
   virtual ~RoutingProtocol ();
 
   void SetMainInterface (uint32_t interface);
 
   void Dump (void);
 
   std::vector<RoutingTableEntry> GetRoutingTableEntries () const;
 
   int64_t AssignStreams (int64_t stream);
 
   typedef void (* PacketTxRxTracedCallback)(const PacketHeader & header, const MessageList & messages);
 
   typedef void (* TableChangeTracedCallback) (uint32_t size);
 
 private:
   std::set<uint32_t> m_interfaceExclusions; 
   Ptr<Ipv4StaticRouting> m_routingTableAssociation; 
 
 public:
   std::set<uint32_t> GetInterfaceExclusions () const
   {
     return m_interfaceExclusions;
   }
   void MprBroadcast();

   void SetInterfaceExclusions (std::set<uint32_t> exceptions);
 
   void AddHostNetworkAssociation (Ipv4Address networkAddr, Ipv4Mask netmask);
 
   void RemoveHostNetworkAssociation (Ipv4Address networkAddr, Ipv4Mask netmask);
 
   void SetRoutingTableAssociation (Ptr<Ipv4StaticRouting> routingTable);
 
   Ptr<const Ipv4StaticRouting> GetRoutingTableAssociation () const;
 
 protected:
   virtual void DoInitialize (void);
 private:
   std::map<Ipv4Address, RoutingTableEntry> m_table; 
 
   Ptr<Ipv4StaticRouting> m_hnaRoutingTable; 
 
   EventGarbageCollector m_events; 
   uint16_t m_dataPacketCounter;
   uint16_t m_messageSequenceNumber;   
   uint16_t m_ansn;
   Time m_helloInterval;
   Time m_controlInterval;
   Time m_manetInterval;
   Time m_svInterval;
   Time m_tcInterval;      
   Time m_midInterval;     
   Time m_hnaInterval;     
   uint8_t m_willingness;  
   Ptr<Ipv4> m_ipv4;   
   uint32_t m_maxQueueLen;
   uint16_t m_packetSequenceNumber;    
   PacketQueue m_queue;
   Ptr<Packet> MprPacket;
   bool m_protcol; /*MANET = 1, DTN = 0*/
   bool m_vType;
   Time m_queueEntryExpireTime;

   Manet_DtnState m_state;  
  
 
   void Clear ();
 
   uint32_t GetSize () const
   {
     return m_table.size ();
   }
 
   void RemoveEntry (const Ipv4Address &dest);
    void AddEntry (const Ipv4Address &dest,
                  const Ipv4Address &next,
                  uint32_t interface,
                  uint32_t distance,
                  uint16_t position,
                  uint16_t vType);
   void AddEntry (const Ipv4Address &dest,
                  const Ipv4Address &next,
                  const Ipv4Address &interfaceAddress,
                  uint32_t distance,
                  uint16_t position,
                  uint16_t vType);
 
   bool Lookup (const Ipv4Address &dest,
                RoutingTableEntry &outEntry) const;
 
   bool FindSendEntry (const RoutingTableEntry &entry,
                       RoutingTableEntry &outEntry) const;
 
   // From Ipv4RoutingProtocol
   virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p,
                                       const Ipv4Header &header,
                                       Ptr<NetDevice> oif,
                                       Socket::SocketErrno &sockerr);
   virtual bool RouteInput (Ptr<const Packet> p,
                            const Ipv4Header &header,
                            Ptr<const NetDevice> idev,
                            UnicastForwardCallback ucb,
                            MulticastForwardCallback mcb,
                            LocalDeliverCallback lcb,
                            ErrorCallback ecb);
   virtual void NotifyInterfaceUp (uint32_t interface);
   virtual void NotifyInterfaceDown (uint32_t interface);
   virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
   virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
   virtual void SetIpv4 (Ptr<Ipv4> ipv4);
   virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;
 
   void DoDispose ();
 
   void SendPacket (Ptr<Packet> packet, const MessageList &containedMessages);
   
   inline uint16_t GetPacketSequenceNumber ();
 
   inline uint16_t GetMessageSequenceNumber ();
 
   void RecvPacket (Ptr<Socket> socket);
 
   void RoutingTableComputation ();
 
   Ipv4Address GetMainAddress (Ipv4Address iface_addr) const;
 
   bool UsesNonManetDtnOutgoingInterface (const Ipv4RoutingTableEntry &route);
 
   // Timer handlers
   Timer m_helloTimer; 
 
   void HelloTimerExpire ();
   
   Timer m_svTimer;
   
   void ManetTimeExpire ();
   Timer m_manetTimer;

   void SVTimerExpire ();

   Timer m_tcTimer; 
   
   void ACMessageTimeExpire ();
   Timer m_acTimer;

   void TcTimerExpire ();
 
   Timer m_midTimer; 
 
   void MidTimerExpire ();
 
   Timer m_hnaTimer; 
 
   void HnaTimerExpire ();
 
   void DupTupleTimerExpire (Ipv4Address address, uint16_t sequenceNumber);
 
   bool m_linkTupleTimerFirstTime; 
 
   void LinkTupleTimerExpire (Ipv4Address neighborIfaceAddr);
 
   void Nb2hopTupleTimerExpire (Ipv4Address neighborMainAddr, Ipv4Address twoHopNeighborAddr);
 
   void MprSelTupleTimerExpire (Ipv4Address mainAddr);
 
   void TopologyTupleTimerExpire (Ipv4Address destAddr, Ipv4Address lastAddr);
 
   void IfaceAssocTupleTimerExpire (Ipv4Address ifaceAddr);
 
   void AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask);
 
   void IncrementAnsn ();
   Timer m_queuedPacketTimer;
   manet_dtn::MessageList m_queuedMessages;
   Timer m_queuedMessagesTimer;
   
   void MprComputation ();

 
   void ForwardDefault (manet_dtn::MessageHeader manet_dtnMessage,
                        DuplicateTuple *duplicated,
                        const Ipv4Address &localIface,
                        const Ipv4Address &senderAddress);
 
   void QueueMessage (const manet_dtn::MessageHeader &message, Time delay);
   void MprPacketQueue (const Ptr<Packet> packet, Time delay);
   void SendQueuedMessages ();
 
   void SendHello ();
   
   void SendControlMessage ();

   void SendSummaryVector ();
   
   void SendDREQ (Ipv4Address dest,SummaryVectorHeader sm);
   void SendTc ();
 
   void SendMid ();
 
   void SendHna ();
   
   SummaryVectorHeader GetDisjointSummary(const SummaryVectorHeader listA,
                                          const SummaryVectorHeader listB);
   void NeighborLoss (const LinkTuple &tuple);
 
   void AddDuplicateTuple (const DuplicateTuple &tuple);
 
   void RemoveDuplicateTuple (const DuplicateTuple &tuple);
 
   void LinkTupleAdded (const LinkTuple &tuple, uint8_t willingness);
 
   void RemoveLinkTuple (const LinkTuple &tuple);
 
   void LinkTupleUpdated (const LinkTuple &tuple, uint8_t willingness);
 
   void AddNeighborTuple (const NeighborTuple &tuple);
 
   void RemoveNeighborTuple (const NeighborTuple &tuple);
 
   void AddTwoHopNeighborTuple (const TwoHopNeighborTuple &tuple);
 
   void RemoveTwoHopNeighborTuple (const TwoHopNeighborTuple &tuple);
 
   void AddMprSelectorTuple (const MprSelectorTuple  &tuple);
 
   void RemoveMprSelectorTuple (const MprSelectorTuple &tuple);
 
   void AddTopologyTuple (const TopologyTuple &tuple);
 
   void RemoveTopologyTuple (const TopologyTuple &tuple);
 
   void AddIfaceAssocTuple (const IfaceAssocTuple &tuple);
 
   void RemoveIfaceAssocTuple (const IfaceAssocTuple &tuple);
 
   void AddAssociationTuple (const AssociationTuple &tuple);
 
   void RemoveAssociationTuple (const AssociationTuple &tuple);
 
   void ProcessHello (const manet_dtn::MessageHeader &msg,
                      const Ipv4Address &receiverIface,
                      const Ipv4Address &senderIface);
 
   void ProcessTc (const manet_dtn::MessageHeader &msg,
                   const Ipv4Address &senderIface);

   void ProcessControl (const manet_dtn::MessageHeader &msg,
                   const Ipv4Address &senderIface);

   void ProcessMid (const manet_dtn::MessageHeader &msg,
                    const Ipv4Address &senderIface);
 
   void ProcessHna (const manet_dtn::MessageHeader &msg,
                    const Ipv4Address &senderIface);
   void ProcessSV (const manet_dtn::MessageHeader &msg,
                    const Ipv4Address &senderIface);
   void ProcessDREQ (const manet_dtn::MessageHeader &msg,
                    const Ipv4Address &senderIface);                    
   void LinkSensing (const manet_dtn::MessageHeader &msg,
                     const manet_dtn::MessageHeader::Hello &hello,
                     const Ipv4Address &receiverIface,
                     const Ipv4Address &senderIface);
 
   void PopulateNeighborSet (const manet_dtn::MessageHeader &msg,
                             const manet_dtn::MessageHeader::Hello &hello);
 
   void PopulateTwoHopNeighborSet (const manet_dtn::MessageHeader &msg,
                                   const manet_dtn::MessageHeader::Hello &hello);
 
   void PopulateMprSelectorSet (const manet_dtn::MessageHeader &msg,
                                const manet_dtn::MessageHeader::Hello &hello);
 
   int Degree (NeighborTuple const &tuple);
 
   bool IsMyOwnAddress (const Ipv4Address & a) const;
 
   Ipv4Address m_mainAddress; 
 
   // One socket per interface, each bound to that interface's address
   // (reason: for OLSR Link Sensing we need to know on which interface
   // HELLO messages arrive)
   std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_sendSockets; 
   Ptr<Socket> m_recvSocket; 
 
 
   TracedCallback <const PacketHeader &, const MessageList &> m_rxPacketTrace;
 
   TracedCallback <const PacketHeader &, const MessageList &> m_txPacketTrace;
 
   TracedCallback <uint32_t> m_routingTableChanged;
 
   Ptr<UniformRandomVariable> m_uniformRandomVariable;
 
 };
 
 }
 }  // namespace ns3
 
 #endif /* OLSR_AGENT_IMPL_H */
