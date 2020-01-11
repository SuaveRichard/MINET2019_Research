/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 /*
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
  * Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt>
  */
 
 #include <cmath>
 
 #include "ns3/assert.h"
 #include "ns3/log.h"
 
 #include "manet_dtn-header.h"
 
 #define IPV4_ADDRESS_SIZE 4
 #define manet_dtn_MSG_HEADER_SIZE 12
 #define manet_dtn_PKT_HEADER_SIZE 4
 
 namespace ns3 {
 
 NS_LOG_COMPONENT_DEFINE ("manet_dtnHeader");
 
 namespace manet_dtn {
 
 
 #define manet_dtn_C 0.0625
 
 uint8_t
 SecondsToEmf (double seconds)
 {
   int a, b = 0;
 
   NS_ASSERT_MSG (seconds >= manet_dtn_C, "SecondsToEmf - Can not convert a value less than manet_dtn_C");
 
   // find the largest integer 'b' such that: T/C >= 2^b
   for (b = 1; (seconds / manet_dtn_C) >= (1 << b); ++b)
     {
     }
   NS_ASSERT ((seconds / manet_dtn_C) < (1 << b));
   b--;
   NS_ASSERT ((seconds / manet_dtn_C) >= (1 << b));
 
   // compute the expression 16*(T/(C*(2^b))-1), which may not be a integer
   double tmp = 16 * (seconds / (manet_dtn_C * (1 << b)) - 1);
 
   // round it up.  This results in the value for 'a'
   a = (int) std::ceil (tmp - 0.5);
 
   // if 'a' is equal to 16: increment 'b' by one, and set 'a' to 0
   if (a == 16)
     {
       b += 1;
       a = 0;
     }
 
   // now, 'a' and 'b' should be integers between 0 and 15,
   NS_ASSERT (a >= 0 && a < 16);
   NS_ASSERT (b >= 0 && b < 16);
 
   // the field will be a byte holding the value a*16+b
   return (uint8_t)((a << 4) | b);
 }
 
 double
 EmfToSeconds (uint8_t manet_dtnFormat)
 {
   int a = (manet_dtnFormat >> 4);
   int b = (manet_dtnFormat & 0xf);
   // value = C*(1+a/16)*2^b [in seconds]
   return manet_dtn_C * (1 + a / 16.0) * (1 << b);
 }
 
 
 
 // ---------------- manet_dtn Packet -------------------------------
 
 NS_OBJECT_ENSURE_REGISTERED (PacketHeader);
 
 PacketHeader::PacketHeader ()
 {
 }
 
 PacketHeader::~PacketHeader ()
 {
 }
 
 TypeId
 PacketHeader::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::manet_dtn::PacketHeader")
     .SetParent<Header> ()
     .SetGroupName ("manet_dtn")
     .AddConstructor<PacketHeader> ()
   ;
   return tid;
 }
 TypeId
 PacketHeader::GetInstanceTypeId (void) const
 {
   return GetTypeId ();
 }
 
 uint32_t
 PacketHeader::GetSerializedSize (void) const
 {
   return manet_dtn_PKT_HEADER_SIZE;
 }
 
 void
 PacketHeader::Print (std::ostream &os) const
 {
 }
 
 void
 PacketHeader::Serialize (Buffer::Iterator start) const
 {
   Buffer::Iterator i = start;
   i.WriteHtonU16 (m_packetLength);
   i.WriteHtonU16 (m_packetSequenceNumber);
 }
 
 uint32_t
 PacketHeader::Deserialize (Buffer::Iterator start)
 {
   Buffer::Iterator i = start;
   m_packetLength  = i.ReadNtohU16 ();
   m_packetSequenceNumber = i.ReadNtohU16 ();
   return GetSerializedSize ();
 }
 
 
 // ---------------- manet_dtn Message -------------------------------
 
 NS_OBJECT_ENSURE_REGISTERED (MessageHeader);
 
 MessageHeader::MessageHeader ()
   : m_messageType (MessageHeader::MessageType (0))
 {
 }
 
 MessageHeader::~MessageHeader ()
 {
 }
 
 TypeId
 MessageHeader::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::manet_dtn::MessageHeader")
     .SetParent<Header> ()
     .SetGroupName ("manet_dtn")
     .AddConstructor<MessageHeader> ()
   ;
   return tid;
 }
 TypeId
 MessageHeader::GetInstanceTypeId (void) const
 {
   return GetTypeId ();
 }
 
 uint32_t
 MessageHeader::GetSerializedSize (void) const
 {
   uint32_t size = manet_dtn_MSG_HEADER_SIZE;
   switch (m_messageType)
     {
     case MID_MESSAGE:
       size += m_message.mid.GetSerializedSize ();
       break;
     case HELLO_MESSAGE:
       NS_LOG_DEBUG ("Hello Message Size: " << size << " + " << m_message.hello.GetSerializedSize ());
       size += m_message.hello.GetSerializedSize ();
       break;
     case TC_MESSAGE:
       size += m_message.tc.GetSerializedSize ();
       break;
     case HNA_MESSAGE:
       size += m_message.hna.GetSerializedSize ();
       break;
     case SV_MESSAGE:
       size += m_message.sv.GetSerializedSize ();
       break; 
     case DREQ_MESSAGE:
       size += m_message.dreq.GetSerializedSize ();
       break;
    case CONTROL_MESSAGE:
       size += m_message.control.GetSerializedSize();
       break;
     default:
       NS_ASSERT (false);
     }
   return size;
 }
 
 void
 MessageHeader::Print (std::ostream &os) const
 {
 }
 
 void
 MessageHeader::Serialize (Buffer::Iterator start) const
 {
   Buffer::Iterator i = start;
   i.WriteU8 (m_messageType);
   i.WriteU8 (m_vTime);
   i.WriteHtonU16 (GetSerializedSize ());
   i.WriteHtonU32 (m_originatorAddress.Get ());
   i.WriteU8 (m_timeToLive);
   i.WriteU8 (m_hopCount);
   i.WriteHtonU16 (m_messageSequenceNumber);
 
   switch (m_messageType)
     {
     case MID_MESSAGE:
       m_message.mid.Serialize (i);
       break;
     case HELLO_MESSAGE:
       m_message.hello.Serialize (i);
       break;
     case TC_MESSAGE:
       m_message.tc.Serialize (i);
       break;
     case HNA_MESSAGE:
       m_message.hna.Serialize (i);
       break;
     case SV_MESSAGE:
       m_message.sv.Serialize(i);
       break;
     case DREQ_MESSAGE:
       m_message.dreq.Serialize(i);
       break;
     case CONTROL_MESSAGE:
       m_message.control.Serialize(i);
       break;
     default:
       NS_ASSERT (false);
     }
 
 }
 
 uint32_t
 MessageHeader::Deserialize (Buffer::Iterator start)
 {
   uint32_t size;
   Buffer::Iterator i = start;
   m_messageType  = (MessageType) i.ReadU8 ();
   NS_ASSERT (m_messageType >= HELLO_MESSAGE && m_messageType <= CONTROL_MESSAGE);
   m_vTime  = i.ReadU8 ();
   m_messageSize  = i.ReadNtohU16 ();
   m_originatorAddress = Ipv4Address (i.ReadNtohU32 ());
   m_timeToLive  = i.ReadU8 ();
   m_hopCount  = i.ReadU8 ();
   m_messageSequenceNumber = i.ReadNtohU16 ();
   size = manet_dtn_MSG_HEADER_SIZE;
   switch (m_messageType)
     {
     case MID_MESSAGE:
       size += m_message.mid.Deserialize (i, m_messageSize - manet_dtn_MSG_HEADER_SIZE);
       break;
     case HELLO_MESSAGE:
       size += m_message.hello.Deserialize (i, m_messageSize - manet_dtn_MSG_HEADER_SIZE);
       break;
     case TC_MESSAGE:
       size += m_message.tc.Deserialize (i, m_messageSize - manet_dtn_MSG_HEADER_SIZE);
       break;
     case HNA_MESSAGE:
       size += m_message.hna.Deserialize (i, m_messageSize - manet_dtn_MSG_HEADER_SIZE);
       break;
     case SV_MESSAGE:
       size += m_message.sv.Deserialize (i, m_messageSize - manet_dtn_MSG_HEADER_SIZE);
       break;
     case DREQ_MESSAGE:
       size += m_message.dreq.Deserialize (i, m_messageSize - manet_dtn_MSG_HEADER_SIZE);
       break;
     case CONTROL_MESSAGE:
       size += m_message.control.Deserialize(i, m_messageSize - manet_dtn_MSG_HEADER_SIZE);
       break;
     default:
       NS_ASSERT (false);
     }
   return size;
 }
 
  // ---------------- manet_dtn DREQ Message -------------------------------
 
 uint32_t
 MessageHeader::DREQ::GetSerializedSize (void) const
 {
   return  this->m_packets.size()*sizeof(u_int32_t);
 }
 void
 MessageHeader::DREQ::Serialize (Buffer::Iterator start) const
 {
   Buffer::Iterator i = start;
     for (std::vector<uint32_t>::const_iterator j = this->m_packets.begin ();
       j != this->m_packets.end (); ++j)
    {
      i.WriteHtonU32 (*j);
    }
 }
 uint32_t
 MessageHeader::DREQ::Deserialize (Buffer::Iterator start, uint32_t messageSize)
 {
    Buffer::Iterator i = start;
    this->m_packets.clear ();
    int num = messageSize/4;
   for (int j = 0 ; j < num; j++)
    {
      this->m_packets.push_back (uint32_t(i.ReadNtohU32 ()));
    }
    return messageSize;
 }
  // ---------------- manet_dtn CONTROL Message -------------------------------
 uint32_t
 MessageHeader::CONTROL::GetSerializedSize (void) const
 {
   return IPV4_ADDRESS_SIZE;
 }
 void
 MessageHeader::CONTROL::Serialize (Buffer::Iterator start) const
 {
   Buffer::Iterator i = start;
   i.WriteHtonU32 (this->acAddress.Get());
 }
 uint32_t
 MessageHeader::CONTROL::Deserialize (Buffer::Iterator start, uint32_t messageSize)
 {
   Buffer::Iterator i = start;
   this->acAddress = Ipv4Address (i.ReadNtohU32 ());
   return messageSize;
 }

  // ---------------- manet_dtn SV Message -------------------------------
  uint32_t
 MessageHeader::SV::GetSerializedSize (void) const
 {
   return this->m_packets.size() * sizeof(u_int32_t);
 }
 void
 MessageHeader::SV::Serialize (Buffer::Iterator start) const
 {
    Buffer::Iterator i = start;
     for (std::vector<uint32_t>::const_iterator j = this->m_packets.begin ();
       j != this->m_packets.end (); ++j)
    {
      i.WriteHtonU32 (*j);
    }
 }
 uint32_t
 MessageHeader::SV::Deserialize (Buffer::Iterator start, uint32_t messageSize)
 {
    Buffer::Iterator i = start;
    this->m_packets.clear ();
    int num = messageSize/4;
   for (int j = 0 ; j < num; j++)
    {
      this->m_packets.push_back (uint32_t(i.ReadNtohU32 ()));
    }
    return messageSize;
 }
 // ---------------- manet_dtn MID Message -------------------------------
 
 uint32_t
 MessageHeader::Mid::GetSerializedSize (void) const
 {
   return this->interfaceAddresses.size () * IPV4_ADDRESS_SIZE;
 }
 
 void
 MessageHeader::Mid::Print (std::ostream &os) const
 {
 }
 
 void
 MessageHeader::Mid::Serialize (Buffer::Iterator start) const
 {
   Buffer::Iterator i = start;
 
   for (std::vector<Ipv4Address>::const_iterator iter = this->interfaceAddresses.begin ();
        iter != this->interfaceAddresses.end (); iter++)
     {
       i.WriteHtonU32 (iter->Get ());
     }
 }
 
 uint32_t
 MessageHeader::Mid::Deserialize (Buffer::Iterator start, uint32_t messageSize)
 {
   Buffer::Iterator i = start;
 
   this->interfaceAddresses.clear ();
   NS_ASSERT (messageSize % IPV4_ADDRESS_SIZE == 0);
 
   int numAddresses = messageSize / IPV4_ADDRESS_SIZE;
   this->interfaceAddresses.erase (this->interfaceAddresses.begin (),
                                   this->interfaceAddresses.end ());
   for (int n = 0; n < numAddresses; ++n)
     {
       this->interfaceAddresses.push_back (Ipv4Address (i.ReadNtohU32 ()));
     }
   return GetSerializedSize ();
 }
 
 
 
 // ---------------- manet_dtn HELLO Message -------------------------------
 
 uint32_t
 MessageHeader::Hello::GetSerializedSize (void) const
 {
  uint32_t size = 8;
   for (std::vector<LinkMessage>::const_iterator iter = this->linkMessages.begin ();
        iter != this->linkMessages.end (); iter++)
     {
       const LinkMessage &lm = *iter;
       size += 4;
       size += IPV4_ADDRESS_SIZE * lm.neighborInterfaceAddresses.size ();
       size += 2*lm.neighborPositions.size();
       size += 2*lm.neighborVTypes.size();
     }
   return size;
 }
 
 void
 MessageHeader::Hello::Print (std::ostream &os) const
 {
 }
 
 void
 MessageHeader::Hello::Serialize (Buffer::Iterator start) const
 {
    Buffer::Iterator i = start;
   i.WriteU16 (0); // Reserved
   i.WriteU8 (this->hTime);
   i.WriteU8 (this->willingness);
   i.WriteU16 (this->position);
   i.WriteU16 (this->vType);
   for (std::vector<LinkMessage>::const_iterator iter = this->linkMessages.begin ();
        iter != this->linkMessages.end (); iter++)
     {
       const LinkMessage &lm = *iter;
       i.WriteU8 (lm.linkCode);
       i.WriteU8 (0); // Reserved

       // The size of the link message, counted in bytes and measured
       // from the beginning of the "Link Code" field and until the
       // next "Link Code" field (or - if there are no more link types
       // - the end of the message).

       i.WriteHtonU16 (4 + lm.neighborInterfaceAddresses.size () * IPV4_ADDRESS_SIZE+2*lm.neighborPositions.size()+2*lm.neighborVTypes.size());
 
       for (std::vector<Ipv4Address>::const_iterator neigh_iter = lm.neighborInterfaceAddresses.begin ();
            neigh_iter != lm.neighborInterfaceAddresses.end (); neigh_iter++)
         {
           i.WriteHtonU32 (neigh_iter->Get ());
         }
         for(std::vector<uint16_t>::const_iterator neigh_iter = lm.neighborPositions.begin();
         neigh_iter != lm.neighborPositions.end();neigh_iter++)
         {
           i.WriteU16 (*neigh_iter);
         }
         for(std::vector<uint16_t>::const_iterator neigh_iter = lm.neighborVTypes.begin();
             neigh_iter != lm.neighborVTypes.end();neigh_iter++)
         {
           i.WriteU16 (*neigh_iter);
         }
     }
 }
 
 uint32_t
 MessageHeader::Hello::Deserialize (Buffer::Iterator start, uint32_t messageSize)
 {
   Buffer::Iterator i = start;
   NS_ASSERT (messageSize >= 4);
 
   this->linkMessages.clear ();
 
   uint16_t helloSizeLeft = messageSize;
   i.ReadNtohU16 (); // Reserved
   this->hTime = i.ReadU8 ();
   this->willingness = i.ReadU8 ();
   this->position = i.ReadU16();
   this->vType = i.ReadU16();
   helloSizeLeft -= 8;
 
   while (helloSizeLeft)
     {
       LinkMessage lm;
       NS_ASSERT (helloSizeLeft >= 8);
       lm.linkCode = i.ReadU8 ();
       i.ReadU8 (); // Reserved       
       uint16_t lmSize = i.ReadNtohU16 ();
       NS_ASSERT ((lmSize - 4) % (IPV4_ADDRESS_SIZE+2+2) == 0);
       int lmNum = (lmSize - 8) / IPV4_ADDRESS_SIZE;
       for (int n = lmNum; n; --n)
         {
           lm.neighborInterfaceAddresses.push_back (Ipv4Address (i.ReadNtohU32 ()));
         }
         for (int n = lmNum; n; --n)
         {
           lm.neighborPositions.push_back (i.ReadU16 ());
         }
         for (int n = lmNum; n; --n)
         {
           lm.neighborVTypes.push_back (i.ReadU16 ());
         }
       helloSizeLeft -= lmSize;
       this->linkMessages.push_back (lm);
     }
   return messageSize;
 }
 
 
 
 // ---------------- manet_dtn TC Message -------------------------------
 
 uint32_t
 MessageHeader::Tc::GetSerializedSize (void) const
 {
  return 4 + this->neighborAddresses.size () * IPV4_ADDRESS_SIZE + this->neighborPositions.size() * 2 + this->neighborVTypes.size() * 2;
 }
 
 void
 MessageHeader::Tc::Print (std::ostream &os) const
 {
 }
 
 void
 MessageHeader::Tc::Serialize (Buffer::Iterator start) const
 {
   Buffer::Iterator i = start;
 
   i.WriteHtonU16 (this->ansn);
   i.WriteHtonU16 (0); // Reserved
 
   for (std::vector<Ipv4Address>::const_iterator iter = this->neighborAddresses.begin ();
        iter != this->neighborAddresses.end (); iter++)
     {
       i.WriteHtonU32 (iter->Get ());
     }
    for(std::vector<uint16_t>::const_iterator iter = this->neighborPositions.begin();
        iter != this->neighborPositions.end();iter++)
     {
       i.WriteU16 (*iter);
     }
    for(std::vector<uint16_t>::const_iterator iter = this->neighborVTypes.begin();
        iter != this->neighborVTypes.end();iter++)
     {
       i.WriteU16 (*iter);
     }
 }
 
 uint32_t
 MessageHeader::Tc::Deserialize (Buffer::Iterator start, uint32_t messageSize)
 {
    Buffer::Iterator i = start;
 
   this->neighborAddresses.clear ();
   NS_ASSERT (messageSize >= 4);
 
   this->ansn = i.ReadNtohU16 ();
   i.ReadNtohU16 (); // Reserved
 
   NS_ASSERT ((messageSize - 4) % IPV4_ADDRESS_SIZE == 0);
   int numAddresses = (messageSize - 4) / 8;
   this->neighborAddresses.clear ();
   for (int n = 0; n < numAddresses; ++n)
     {
       this->neighborAddresses.push_back (Ipv4Address (i.ReadNtohU32 ()));
     }
     for (int n = 0; n < numAddresses; ++n)
     {
       this->neighborPositions.push_back (i.ReadU16 ());
     }
     for (int n = 0; n < numAddresses; ++n)
     {
       this->neighborVTypes.push_back (i.ReadU16 ());
     }

   return messageSize;
 }
 
 
 // ---------------- manet_dtn HNA Message -------------------------------
 
 uint32_t
 MessageHeader::Hna::GetSerializedSize (void) const
 {
   return 2 * this->associations.size () * IPV4_ADDRESS_SIZE;
 }
 
 void
 MessageHeader::Hna::Print (std::ostream &os) const
 {
 }
 
 void
 MessageHeader::Hna::Serialize (Buffer::Iterator start) const
 {
   Buffer::Iterator i = start;
 
   for (size_t n = 0; n < this->associations.size (); ++n)
     {
       i.WriteHtonU32 (this->associations[n].address.Get ());
       i.WriteHtonU32 (this->associations[n].mask.Get ());
     }
 }
 
 uint32_t
 MessageHeader::Hna::Deserialize (Buffer::Iterator start, uint32_t messageSize)
 {
   Buffer::Iterator i = start;
 
   NS_ASSERT (messageSize % (IPV4_ADDRESS_SIZE * 2) == 0);
   int numAddresses = messageSize / IPV4_ADDRESS_SIZE / 2;
   this->associations.clear ();
   for (int n = 0; n < numAddresses; ++n)
     {
       Ipv4Address address (i.ReadNtohU32 ());
       Ipv4Mask mask (i.ReadNtohU32 ());
       this->associations.push_back ((Association) { address, mask});
     }
   return messageSize;
 }
 
 }
 }  // namespace manet_dtn, ns3