/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

// Test program for my routing before actually building the FFTree
// A star like toplogy with node 0 at the center as the router
// The csma address can be used as a "to address", iff it matches the packet. But I don't know how to use it as the "to address".

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-static-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("StaticRoutingSlash32Test");

int 
main (int argc, char *argv[])
{
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  //create the nodes with n0 as the center
  Ptr<Node> n0 = CreateObject<Node> ();
  Ptr<Node> n1 = CreateObject<Node> ();
  Ptr<Node> n2 = CreateObject<Node> ();
  Ptr<Node> n3 = CreateObject<Node> ();
  
  NodeContainer allNodes = NodeContainer (n0, n1, n2, n3);

  //install internet stack before p2p devices so that the loopback interface is indexed as 0
  InternetStackHelper internet;
  internet.Install (allNodes);

  //configure the topology
  NodeContainer nodePair1 = NodeContainer (n0, n1);
  NodeContainer nodePair2 = NodeContainer (n0, n2);
  NodeContainer nodePair3 = NodeContainer (n0, n3);

  //channels
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  
  //devices
  NetDeviceContainer devPair1 = p2p.Install (nodePair1);
  NetDeviceContainer devPair2 = p2p.Install (nodePair2);
  NetDeviceContainer devPair3 = p2p.Install (nodePair3);
  
  Ptr<CsmaNetDevice> csmaDev0 = CreateObject<CsmaNetDevice> ();
  csmaDev0->SetAddress (Mac48Address::Allocate());
  n0->AddDevice (csmaDev0);
  Ptr<CsmaNetDevice> csmaDev1 = CreateObject<CsmaNetDevice> ();
  csmaDev1->SetAddress (Mac48Address::Allocate());
  n1->AddDevice (csmaDev1);
  Ptr<CsmaNetDevice> csmaDev2 = CreateObject<CsmaNetDevice> ();
  csmaDev2->SetAddress (Mac48Address::Allocate());
  n2->AddDevice (csmaDev2);
  Ptr<CsmaNetDevice> csmaDev3 = CreateObject<CsmaNetDevice> ();
  csmaDev3->SetAddress (Mac48Address::Allocate());
  n3->AddDevice (csmaDev3);
  
  //intefaces
  Ipv4AddressHelper ipv4;
  
  ipv4.SetBase ("10.0.1.0", "255.255.255.252");
  Ipv4InterfaceContainer ifPair1 = ipv4.Assign (devPair1);
  
  ipv4.SetBase ("10.0.2.0", "255.255.255.252");
  Ipv4InterfaceContainer ifPair2 = ipv4.Assign (devPair2);
  
  ipv4.SetBase ("10.0.3.0", "255.255.255.252");
  Ipv4InterfaceContainer ifPair3 = ipv4.Assign (devPair3);

  
  Ptr<Ipv4> ipv4_0 = n0->GetObject<Ipv4> ();
  Ptr<Ipv4> ipv4_1 = n1->GetObject<Ipv4> ();
  Ptr<Ipv4> ipv4_2 = n2->GetObject<Ipv4> ();
  Ptr<Ipv4> ipv4_3 = n3->GetObject<Ipv4> ();

  int32_t ifIndex0 = ipv4_0->AddInterface (csmaDev0);
  Ipv4InterfaceAddress add0 = Ipv4InterfaceAddress (Ipv4Address ("192.168.0.1"), Ipv4Mask ("/32"));
  ipv4_0->AddAddress (ifIndex0, add0);
  ipv4_0->SetMetric (ifIndex0, 1);
  ipv4_0->SetUp (ifIndex0);
  
  int32_t ifIndex1 = ipv4_1->AddInterface (csmaDev1);
  Ipv4InterfaceAddress add1 = Ipv4InterfaceAddress (Ipv4Address ("192.168.1.2"), Ipv4Mask ("/32"));
  ipv4_1->AddAddress (ifIndex1, add1);
  ipv4_1->SetMetric (ifIndex1, 1);
  ipv4_1->SetUp (ifIndex1);
  
  int32_t ifIndex2 = ipv4_2->AddInterface (csmaDev2);
  Ipv4InterfaceAddress add2 = Ipv4InterfaceAddress (Ipv4Address ("192.168.2.2"), Ipv4Mask ("/32"));
  ipv4_2->AddAddress (ifIndex2, add2);
  ipv4_2->SetMetric (ifIndex2, 1);
  ipv4_2->SetUp (ifIndex2);
  
  int32_t ifIndex3 = ipv4_3->AddInterface (csmaDev3);
  Ipv4InterfaceAddress add3 = Ipv4InterfaceAddress (Ipv4Address ("192.168.3.2"), Ipv4Mask ("/32"));
  //Ipv4InterfaceAddress add3 = Ipv4InterfaceAddress (Ipv4Address ("192.168.3.254"), Ipv4Mask ("/32"));  //not received if the "to address" do not match
  ipv4_3->AddAddress (ifIndex3, add3);
  ipv4_3->SetMetric (ifIndex3, 1);
  ipv4_3->SetUp (ifIndex3);
  
  //Here I will try to find the indices according to the Ipv4 addresses, on node 0. Succeeded.
  /*
  int32_t ifNumber;
  ifNumber = ipv4_0->GetInterfaceForAddress (Ipv4Address ("10.0.1.1"));
  NS_LOG_UNCOND (ifNumber); //should be 1
  ifNumber = ipv4_0->GetInterfaceForAddress (Ipv4Address ("10.0.2.1"));
  NS_LOG_UNCOND (ifNumber); //should be 1
  ifNumber = ipv4_0->GetInterfaceForAddress (Ipv4Address ("10.0.3.1"));
  NS_LOG_UNCOND (ifNumber); //should be 1
  ifNumber = ipv4_0->GetInterfaceForAddress (Ipv4Address ("192.168.0.1"));
  NS_LOG_UNCOND (ifNumber); //should be 1
  ifNumber = ipv4_0->GetInterfaceForAddress (Ipv4Address ("0.0.0.0"));
  NS_LOG_UNCOND (ifNumber); //should be -1
  */

  //routing
  Ipv4StaticRoutingHelper rtHelper;
  Ptr<Ipv4StaticRouting> staticRouting0 = rtHelper.GetStaticRouting (ipv4_0);
  //staticRouting0->AddHostRouteTo (Ipv4Address ("10.0.1.2"), 1); //not really necessary but can be overwritten
  //staticRouting0->AddHostRouteTo (Ipv4Address ("10.0.2.2"), 2); //not really necessary but can be overwritten
  //staticRouting0->AddHostRouteTo (Ipv4Address ("10.0.3.2"), 3); //not really necessary but can be overwritten
  staticRouting0->AddHostRouteTo (Ipv4Address ("192.168.3.2"), 3); //can use the csma interface as the "to address"

  Ptr<Ipv4StaticRouting> staticRouting1 = rtHelper.GetStaticRouting (ipv4_1);
  staticRouting1->AddHostRouteTo (Ipv4Address ("10.0.2.2"), 1);
  staticRouting1->AddHostRouteTo (Ipv4Address ("10.0.3.2"), 1);
  staticRouting1->AddHostRouteTo (Ipv4Address ("192.168.3.2"), 1); //can use the csma interface as the "to address"
        
  Ptr<Ipv4StaticRouting> staticRouting2 = rtHelper.GetStaticRouting (ipv4_2);
  staticRouting2->AddHostRouteTo (Ipv4Address ("10.0.1.2"), 1);
  staticRouting2->AddHostRouteTo (Ipv4Address ("10.0.3.2"), 1);
  
  Ptr<Ipv4StaticRouting> staticRouting3 = rtHelper.GetStaticRouting (ipv4_3);
  staticRouting3->AddHostRouteTo (Ipv4Address ("10.0.1.2"), 1);
  staticRouting3->AddHostRouteTo (Ipv4Address ("10.0.2.2"), 1);
  
  //apps
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (NodeContainer (n1, n2,n3)); //servers on all nodes
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient1 (Ipv4Address ("10.0.1.2"), 9);
  echoClient1.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps1 = echoClient1.Install (n3); // client on n3, to n1
  clientApps1.Start (Seconds (1.0));
  clientApps1.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient2 (Ipv4Address ("10.0.2.2"), 9);
  echoClient2.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps2 = echoClient2.Install (n1); // client on n1, to n2
  clientApps2.Start (Seconds (2.0));
  clientApps2.Stop (Seconds (10.0));

  //UdpEchoClientHelper echoClient3 (Ipv4Address ("10.0.3.2"), 9);
  UdpEchoClientHelper echoClient3 (Ipv4Address ("192.168.3.2"), 9); //can use the csma interface as the "to address"
  echoClient3.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient3.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient3.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps3 = echoClient3.Install (n1); // client on n1, to n3
  clientApps3.Start (Seconds (3.0));
  clientApps3.Stop (Seconds (10.0));

  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("toy-routing.tr"));
  p2p.EnablePcapAll ("toy-routing");

  Simulator::Run ();
  Simulator::Destroy ();

  //signal the end
  NS_LOG_UNCOND ("~FIN~");

  return 0;
}