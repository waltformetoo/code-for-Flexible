//This is a toy example of fat-tree.
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ToyExampleFat-tree");

int
main (int argc, char *argv[])
{
	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

	//a star like topology with 1 core and 3 terminals
	NodeContainer nodes;
	nodes.Create (4);

	//install protocol stack
	InternetStackHelper stack;
	stack.Install (nodes);

	//1st p2p connection between nodes 0 and 1
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

	NetDeviceContainer devices;
	devices = p2p.Install (nodes.Get(0),nodes.Get(1));

	Ipv4AddressHelper address;
	address.SetBase ("10.0.1.0", "255.255.255.0");

	Ipv4InterfaceContainer interfaces = address.Assign (devices);

	//1st pair of Apps
	UdpEchoServerHelper echoServer (9000);

	ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
	serverApps.Start (Seconds (0.0));
	serverApps.Stop (Seconds (10.0));

	UdpEchoClientHelper echoClient (interfaces.GetAddress (0), 9000);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	ApplicationContainer clientApps = echoClient.Install (nodes.Get(1));
	clientApps.Start (Seconds (1.0));
	clientApps.Stop (Seconds (10.0));

	//2nd p2p connection between nodes 0 and 2
	devices = p2p.Install (nodes.Get(0),nodes.Get(2));

  	address.SetBase ("10.0.2.0", "255.255.255.0");

	interfaces = address.Assign (devices);

	//2nd client on node 2
	UdpEchoClientHelper echoClient2 (interfaces.GetAddress (0), 9000);
	echoClient2.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

	ApplicationContainer clientApps2 = echoClient2.Install (nodes.Get(2));
	clientApps2.Start (Seconds (2.0));
	clientApps2.Stop (Seconds (10.0));

	//3rd p2p connection between nodes 0 and 3
	devices = p2p.Install (nodes.Get(0),nodes.Get(3));

	//Let me do a little more precise operation here. I'd like to specify the address of each end.
	/*
  	address.SetBase ("10.0.3.0", "255.255.255.0");

	interfaces = address.Assign (devices);
	*/
	address.SetBase ("10.3.4.0", "255.255.255.0", "0.0.0.253");
	address.Assign (devices.Get(0));
	address.SetBase ("10.3.4.0", "255.255.255.0", "0.0.0.3");
	address.Assign (devices.Get(1));
	
	//3rd client on node 3
	//UdpEchoClientHelper echoClient3 (interfaces.GetAddress (0), 9000);
	UdpEchoClientHelper echoClient3 (Ipv4Address ("10.3.4.253"), 9000);
	echoClient3.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient3.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	echoClient3.SetAttribute ("PacketSize", UintegerValue (1024));

	ApplicationContainer clientApps3 = echoClient3.Install (nodes.Get(3));
	clientApps3.Start (Seconds (3.0));
	clientApps3.Stop (Seconds (10.0));

	//run
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}
