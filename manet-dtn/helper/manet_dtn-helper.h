/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright (c) 2008 INRIA
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
  * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
  */
 #ifndef OLSR_HELPER_H
 #define OLSR_HELPER_H
 
 #include "ns3/object-factory.h"
 #include "ns3/node.h"
 #include "ns3/node-container.h"
 #include "ns3/ipv4-routing-helper.h"
 #include <map>
 #include <set>
 
 namespace ns3 {
 
 class ManetDtnHelper : public Ipv4RoutingHelper
 {
 public:
   ManetDtnHelper ();
 
   ManetDtnHelper (const ManetDtnHelper &);
 
   ManetDtnHelper* Copy (void) const;
 
   void ExcludeInterface (Ptr<Node> node, uint32_t interface);
 
   virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;
 
   void Set (std::string name, const AttributeValue &value);
 
   int64_t AssignStreams (NodeContainer c, int64_t stream);
 
 private:
   ManetDtnHelper &operator = (const ManetDtnHelper &);
   ObjectFactory m_agentFactory; 
 
   std::map< Ptr<Node>, std::set<uint32_t> > m_interfaceExclusions; 
 };
 
 } // namespace ns3
 
 #endif /* OLSR_HELPER_H */