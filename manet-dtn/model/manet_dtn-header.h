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
 
 #ifndef manet_dtn_HEADER_H
 #define manet_dtn_HEADER_H
 
 #include <stdint.h>
 #include <vector>
 #include "ns3/header.h"
 #include "ns3/ipv4-address.h"
 #include "ns3/nstime.h"
 
 
 namespace ns3 {
 namespace manet_dtn {
 
 double EmfToSeconds (uint8_t emf);
 uint8_t SecondsToEmf (double seconds);
 
 class PacketHeader : public Header
 {
 public:
   PacketHeader ();
   virtual ~PacketHeader ();
 
   void SetPacketLength (uint16_t length)
   {
     m_packetLength = length;
   }
 
   uint16_t GetPacketLength () const
   {
     return m_packetLength;
   }
 
   void SetPacketSequenceNumber (uint16_t seqnum)
   {
     m_packetSequenceNumber = seqnum;
   }
 
   uint16_t GetPacketSequenceNumber () const
   {
     return m_packetSequenceNumber;
   }
 
 private:
   uint16_t m_packetLength;          
   uint16_t m_packetSequenceNumber;  
 
 public:
   static TypeId GetTypeId (void);
   virtual TypeId GetInstanceTypeId (void) const;
   virtual void Print (std::ostream &os) const;
   virtual uint32_t GetSerializedSize (void) const;
   virtual void Serialize (Buffer::Iterator start) const;
   virtual uint32_t Deserialize (Buffer::Iterator start);
 };
 
 class MessageHeader : public Header
 {
 public:
   enum MessageType
   {
     HELLO_MESSAGE = 1,
     TC_MESSAGE    = 2,
     MID_MESSAGE   = 3,
     HNA_MESSAGE   = 4,
     SV_MESSAGE = 5,
     DREQ_MESSAGE = 6,
     CONTROL_MESSAGE = 7
   };
 
   MessageHeader ();
   virtual ~MessageHeader ();
 
   void SetMessageType (MessageType messageType)
   {
     m_messageType = messageType;
   }
   MessageType GetMessageType () const
   {
     return m_messageType;
   }
 
   void SetVTime (Time time)
   {
     m_vTime = SecondsToEmf (time.GetSeconds ());
   }
   Time GetVTime () const
   {
     return Seconds (EmfToSeconds (m_vTime));
   }
 
   void SetOriginatorAddress (Ipv4Address originatorAddress)
   {
     m_originatorAddress = originatorAddress;
   }
   Ipv4Address GetOriginatorAddress () const
   {
     return m_originatorAddress;
   }
 
   void SetTimeToLive (uint8_t timeToLive)
   {
     m_timeToLive = timeToLive;
   }
   uint8_t GetTimeToLive () const
   {
     return m_timeToLive;
   }
 
   void SetHopCount (uint8_t hopCount)
   {
     m_hopCount = hopCount;
   }
   uint8_t GetHopCount () const
   {
     return m_hopCount;
   }
 
   void SetMessageSequenceNumber (uint16_t messageSequenceNumber)
   {
     m_messageSequenceNumber = messageSequenceNumber;
   }
   uint16_t GetMessageSequenceNumber () const
   {
     return m_messageSequenceNumber;
   }
 
 private:
   MessageType m_messageType;        
   uint8_t m_vTime;                  
   Ipv4Address m_originatorAddress;  
   uint8_t m_timeToLive;             
   uint8_t m_hopCount;               
   uint16_t m_messageSequenceNumber; 
   uint16_t m_messageSize;           
 
 public:
   static TypeId GetTypeId (void);
   virtual TypeId GetInstanceTypeId (void) const;
   virtual void Print (std::ostream &os) const;
   virtual uint32_t GetSerializedSize (void) const;
   virtual void Serialize (Buffer::Iterator start) const;
   virtual uint32_t Deserialize (Buffer::Iterator start);
 
   struct Mid
   {
     std::vector<Ipv4Address> interfaceAddresses;  
 
     void Print (std::ostream &os) const;
     uint32_t GetSerializedSize (void) const;
     void Serialize (Buffer::Iterator start) const;
     uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
   };
 
   struct Hello
   {
     struct LinkMessage
     {
       uint8_t linkCode;       
       std::vector<Ipv4Address> neighborInterfaceAddresses;  
       std::vector<uint16_t> neighborPositions;
       std::vector<uint16_t> neighborVTypes;
     };
 
     uint8_t hTime;  
     uint16_t position;
     uint16_t vType;
     void SetPosition (uint16_t position)
     {
       this->position = position;
     }
 
     uint16_t GetPosition () const
     {
       return this->position;
     }
     void SetVtype (uint16_t vType)
     {
       this->vType = vType;
     }
 
     uint16_t GetVtype () const
     {
       return this->vType;
     }
     void SetHTime (Time time)
     {
       this->hTime = SecondsToEmf (time.GetSeconds ());
     }
 
     Time GetHTime () const
     {
       return Seconds (EmfToSeconds (this->hTime));
     }
 
     uint8_t willingness; 
     std::vector<LinkMessage> linkMessages; 
 
     void Print (std::ostream &os) const;
     uint32_t GetSerializedSize (void) const;
     void Serialize (Buffer::Iterator start) const;
     uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
   };
 
   struct Tc
   {
     std::vector<Ipv4Address> neighborAddresses;
     std::vector<uint16_t> neighborPositions;
     std::vector<uint16_t> neighborVTypes; 
     uint16_t ansn;  
 
     void Print (std::ostream &os) const;
     uint32_t GetSerializedSize (void) const;
     void Serialize (Buffer::Iterator start) const;
     uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
   };
 
 
   struct Hna
   {
     struct Association
     {
       Ipv4Address address; 
       Ipv4Mask mask;       
     };
 
     std::vector<Association> associations; 
 
     void Print (std::ostream &os) const;
     uint32_t GetSerializedSize (void) const;
     void Serialize (Buffer::Iterator start) const;
     uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
   };
 
 struct CONTROL
 {
    Ipv4Address acAddress;
    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator start) const;
    uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
 };
 
 struct SV
 {
  std::vector<uint32_t> m_packets;
  void Add (const uint32_t pkt_ID);
  bool Contains (const uint32_t pkt_ID) const;
  size_t Size (void) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
 };

 struct DREQ
 {
  std::vector<uint32_t> m_packets;
  void Add (const uint32_t pkt_ID);
  bool Contains (const uint32_t pkt_ID) const;
  size_t Size (void) const; 
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
 };

 
  private:
   struct
   {
     Mid mid;      
     Hello hello;  
     Tc tc;        
     Hna hna;    
     SV sv;
     DREQ dreq;
     CONTROL control;
   } m_message; 
 
 public:
   Mid& GetMid ()
   {
     if (m_messageType == 0)
       {
         m_messageType = MID_MESSAGE;
       }
     else
       {
         NS_ASSERT (m_messageType == MID_MESSAGE);
       }
     return m_message.mid;
   }
 
   Hello& GetHello ()
   {
     if (m_messageType == 0)
       {
         m_messageType = HELLO_MESSAGE;
       }
     else
       {
         NS_ASSERT (m_messageType == HELLO_MESSAGE);
       }
     return m_message.hello;
   }
 
   Tc& GetTc ()
   {
     if (m_messageType == 0)
       {
         m_messageType = TC_MESSAGE;
       }
     else
       {
         NS_ASSERT (m_messageType == TC_MESSAGE);
       }
     return m_message.tc;
   }
 
   Hna& GetHna ()
   {
     if (m_messageType == 0)
       {
         m_messageType = HNA_MESSAGE;
       }
     else
       {
         NS_ASSERT (m_messageType == HNA_MESSAGE);
       }
     return m_message.hna;
   }
   SV& GetSv()
   {
       if (m_messageType == 0)
       {
         m_messageType = SV_MESSAGE;
       }
       else
       {
         NS_ASSERT (m_messageType == SV_MESSAGE);
       }
     return m_message.sv;
   }
    CONTROL& GetControl()
   {
       if (m_messageType == 0)
       {
         m_messageType = CONTROL_MESSAGE;
       }
       else
       {
         NS_ASSERT (m_messageType == CONTROL_MESSAGE);
       }
     return m_message.control;
   }
   DREQ& GetDreq()
   {
       if (m_messageType == 0)
       {
         m_messageType = DREQ_MESSAGE;
       }
       else
       {
         NS_ASSERT (m_messageType == DREQ_MESSAGE);
       }
     return m_message.dreq;
   }
   const SV& GetSv () const
   {
     NS_ASSERT (m_messageType == SV_MESSAGE);
     return m_message.sv;
   }
   const CONTROL& GetControl () const
   {
     NS_ASSERT (m_messageType == CONTROL_MESSAGE);
     return m_message.control;
   }
   const DREQ& GetDreq () const
   {
     NS_ASSERT (m_messageType == DREQ_MESSAGE);
     return m_message.dreq;
   }

   const Mid& GetMid () const
   {
     NS_ASSERT (m_messageType == MID_MESSAGE);
     return m_message.mid;
   }
 
   const Hello& GetHello () const
   {
     NS_ASSERT (m_messageType == HELLO_MESSAGE);
     return m_message.hello;
   }
 
   const Tc& GetTc () const
   {
     NS_ASSERT (m_messageType == TC_MESSAGE);
     return m_message.tc;
   }
 
   const Hna& GetHna () const
   {
     NS_ASSERT (m_messageType == HNA_MESSAGE);
     return m_message.hna;
   }
 
 
 };
 
 
 static inline std::ostream& operator<< (std::ostream& os, const PacketHeader & packet)
 {
   packet.Print (os);
   return os;
 }
 
 static inline std::ostream& operator<< (std::ostream& os, const MessageHeader & message)
 {
   message.Print (os);
   return os;
 }
 
 typedef std::vector<MessageHeader> MessageList;
 
 static inline std::ostream& operator<< (std::ostream& os, const MessageList & messages)
 {
   os << "[";
   for (std::vector<MessageHeader>::const_iterator messageIter = messages.begin ();
        messageIter != messages.end (); messageIter++)
     {
       messageIter->Print (os);
       if (messageIter + 1 != messages.end ())
         {
           os << ", ";
         }
     }
   os << "]";
   return os;
 }
 
 
 }
 }  // namespace manet_dtn, ns3
 
 #endif /* manet_dtn_HEADER_H */