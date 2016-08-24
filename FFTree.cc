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

// Build a FFTree.
// Current problem is routing. Looks like a routing table entry does not support wild card.

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

//topology
int i_edge_ofst = 0;
int j_aggr_ofst = 1;
int k_port_count = 6; //must be even, can not be 10
//connect an aggr sw
int aggr_down_pc = k_port_count/2 - j_aggr_ofst;
int aggr_up_begin = aggr_down_pc;
int aggr_up_pc = k_port_count/2 + j_aggr_ofst;
//connect an edge sw
int edge_down_pc = k_port_count/2 - i_edge_ofst;
int edge_up_begin = edge_down_pc;
int edge_up_pc = k_port_count/2 + i_edge_ofst;
//index fpods in the FFTree
int f_begin = 0;
int f_end = k_port_count-1;
int fpod_count = k_port_count;
//index aggr sw in a fpod
int a_begin = k_port_count/2 - j_aggr_ofst;
int a_end = k_port_count + i_edge_ofst - j_aggr_ofst - 1;
int aggr_count_in_fpod = k_port_count/2 + i_edge_ofst;
//index edge sw in a fpod
int e_begin = 0;
int e_end = k_port_count/2 - j_aggr_ofst - 1;
int edge_count_in_fpod = k_port_count/2 - j_aggr_ofst;
//index core sw in the FFTree
int core_count = aggr_up_pc * aggr_count_in_fpod;

Ipv4Address
corePortAddress (int coreIndex, int portIndex) {
	std::stringstream strAddr;
	strAddr<<k_port_count<<'.';//byte3
	strAddr<<coreIndex / aggr_up_pc + 1<<'.';//byte2
	strAddr<<coreIndex % aggr_up_pc + 1<<'.';//byte1
	strAddr<<254 - portIndex;//byte0
	return Ipv4Address(strAddr.str().c_str());
}

Ipv4Address
aggrPortAddress (int f, int a, int portIndex) {
	std::stringstream strAddr;
	if (portIndex < aggr_up_begin) {
		strAddr<<"10."<<f<<'.'<<a<<'.'<<254 - portIndex;
	}//downward ports
	else {
		strAddr<<k_port_count<<'.';//byte3
		strAddr<<a - a_begin + 1<<'.';//byte2
		strAddr<<portIndex - aggr_up_begin + 1<<'.';//byte1
		strAddr<<f + 2;//byte0
	}//upward ports
	return Ipv4Address(strAddr.str().c_str());
}

Ipv4Address
edgePortAddress (int f, int e, int portIndex) {
	std::stringstream strAddr;
	if (portIndex < edge_up_begin) {
		strAddr<<"10."<<f<<'.'<<e<<'.'<<254 - portIndex;
	}//downward ports
	else {
		strAddr<<"10.";//byte3
		strAddr<<f<<'.';//byte2
		strAddr<<portIndex - edge_up_begin + a_begin<<'.';//byte1
		strAddr<<e+2;//byte0
	}//upward ports
	return  Ipv4Address(strAddr.str().c_str());
}

Ipv4Address
serverAddress (int f, int e, int portIndex) {
	std::stringstream strAddr;
	strAddr<<"10.";//byte3
	strAddr<<f<<'.';//byte2
	strAddr<<e<<'.';//byte1
	strAddr<<portIndex+2;//byte0
	return  Ipv4Address(strAddr.str().c_str());
}

bool
addressCheck (Ipv4Address addr1, Ipv4Address addr2) {
	Ipv4Address prefix1 = addr1.CombineMask("255.255.255.0");
	Ipv4Address prefix2 = addr2.CombineMask("255.255.255.0");
	if (!prefix1.IsEqual(prefix2)){
		NS_LOG_UNCOND("p2p address prefix not match");
		return false;
	}
	uint8_t buff1[4];
	addr1.Serialize(buff1);
	uint8_t buff2[4];
	addr2.Serialize(buff2);
	if ((int)buff1[3] + (int)buff2[3] != 256){
		NS_LOG_UNCOND("p2p address suffix not match");
		return false;
	}
	return true;
}

//show all ipv4 interface addresses of a switch
void
showAddresses (Ptr<Node> sw){
	Ptr<Ipv4> edgeIpv4 = sw->GetObject<Ipv4> ();
	uint32_t nInterface = edgeIpv4->GetNInterfaces();
	for (uint32_t i=0; i<nInterface; i++) {
		NS_LOG_UNCOND("interface "<<i<<": ");
		uint32_t nAddress = edgeIpv4->GetNAddresses (i);
		for (uint32_t a=0; a<nAddress; a++) {
			NS_LOG_UNCOND(edgeIpv4->GetAddress (i, a));
		}
	}
}

int
main (int argc, char *argv[])
{
	//create groups of servers
	NodeContainer serverGroup[fpod_count];
	for (int f=f_begin; f<=f_end; f++) {
		serverGroup[f].Create(edge_down_pc*edge_count_in_fpod);
	}
	//create fpods
	NodeContainer fpod[fpod_count];
	for (int f=f_begin; f<=f_end; f++) {
		fpod[f].Create(aggr_count_in_fpod + edge_count_in_fpod);
	}
	//create core switches
	NodeContainer cores;
	cores.Create(core_count);

	//channel helper
	PointToPointHelper p2pHelper;
	p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
	p2pHelper.SetChannelAttribute ("Delay", StringValue ("2ms"));
	//node pair container
	NodeContainer nodePair;
	//dev pair container
	NetDeviceContainer devicePair;
	//internet helper
	InternetStackHelper internetHelper;
	//address helper
	Ipv4AddressHelper addrHelper;

	//edge switches connect servers
	//Could be more concise if allowing ip stack to be installed before the p2p devices been installed
	for (int f=f_begin; f<=f_end; f++) {
		for (int e=e_begin; e<=e_end; e++) {
			Ptr<Node> edgeSwitch = fpod[f].Get(e);
			NetDeviceContainer devPair[edge_down_pc];//device container for one edge switch
			//install p2p devices, install internet stack on servers
			for (int portIndex=0; portIndex<edge_down_pc; portIndex++) {
				int serverIndex = edge_down_pc * e + portIndex;
				Ptr<Node> server = serverGroup[f].Get(serverIndex);
				nodePair = NodeContainer (edgeSwitch, server);
				devPair[portIndex] = p2pHelper.Install(nodePair);
				internetHelper.Install (server);
			}
			//install internet stack on the edge switch
			internetHelper.Install (edgeSwitch);
			//assign addresses
			for (int portIndex=0; portIndex<edge_down_pc; portIndex++) {
				Ipv4Address edgePortAddr = edgePortAddress(f,e,portIndex);
				Ipv4Address serverAddr = serverAddress(f,e,portIndex);
				if (!addressCheck(edgePortAddr, serverAddr))
					NS_LOG_UNCOND("p2p addresses not match between edge and servers");
				addrHelper.SetBase (edgePortAddr.CombineMask("255.255.255.0"), "255.255.255.0", edgePortAddr.CombineMask("0.0.0.255"));
				addrHelper.Assign (devPair[portIndex].Get(0));
				addrHelper.SetBase (serverAddr.CombineMask("255.255.255.0"), "255.255.255.0", serverAddr.CombineMask("0.0.0.255"));
				addrHelper.Assign (devPair[portIndex].Get(1));
			}
		}//for each edge switch e in fpod f
	}//for each fpod f

	//aggr switches connect edge switches
	for (int f=f_begin; f<=f_end; f++) {
		for (int a=a_begin; a<=a_end; a++) {
			Ptr<Node> aggrSwitch = fpod[f].Get(a);
			internetHelper.Install(aggrSwitch);
			for (int portIndex = 0; portIndex <aggr_down_pc; portIndex++) {
				int e = portIndex;
				Ptr<Node> edgeSwitch = fpod[f].Get(e);
				nodePair = NodeContainer (aggrSwitch, edgeSwitch);
				devicePair = p2pHelper.Install(nodePair);
				Ipv4Address aggrPortAddr = aggrPortAddress (f, a, portIndex);
				Ipv4Address edgePortAddr = edgePortAddress (f, e, edge_up_begin + a - a_begin);
				if (!addressCheck(aggrPortAddr, edgePortAddr))
					NS_LOG_UNCOND("p2p addresses not match between aggregation and edge");
				addrHelper.SetBase (aggrPortAddr.CombineMask("255.255.255.0"), "255.255.255.0", aggrPortAddr.CombineMask("0.0.0.255"));
				addrHelper.Assign (devicePair.Get(0));
				addrHelper.SetBase (edgePortAddr.CombineMask("255.255.255.0"), "255.255.255.0", edgePortAddr.CombineMask("0.0.0.255"));
				addrHelper.Assign (devicePair.Get(1));
			}
		}
	}//for each fpod f

	//core switches connect aggr switches
	for (int a = a_begin; a<=a_end; a++) {
		for (int aPortIndex =  aggr_up_begin; aPortIndex < k_port_count; aPortIndex++) {
			int coreIndex = (a - a_begin)*aggr_up_pc + (aPortIndex - aggr_up_begin);
			Ptr<Node> coreSwitch = cores.Get(coreIndex);
			internetHelper.Install(coreSwitch);
			for (int cPortIndex = 0; cPortIndex < k_port_count; cPortIndex++) {
				int f = cPortIndex;
				nodePair = NodeContainer(coreSwitch, fpod[f].Get(a));
				devicePair = p2pHelper.Install(nodePair);
				Ipv4Address corePortAddr = corePortAddress(coreIndex, cPortIndex);
				Ipv4Address aggrPortAddr = aggrPortAddress(f, a, aPortIndex);
				if (!addressCheck(corePortAddr, aggrPortAddr))
					NS_LOG_UNCOND("p2p addresses not match between core and aggragation");
				addrHelper.SetBase (corePortAddr.CombineMask("255.255.255.0"), "255.255.255.0", corePortAddr.CombineMask("0.0.0.255"));
				addrHelper.Assign (devicePair.Get(0));
				addrHelper.SetBase (aggrPortAddr.CombineMask("255.255.255.0"), "255.255.255.0", aggrPortAddr.CombineMask("0.0.0.255"));
				addrHelper.Assign (devicePair.Get(1));
			} //each core switch port
		} //for each aggragation up port
	} //each aggregation switch

	Simulator::Run ();
	Simulator::Destroy ();

	NS_LOG_UNCOND("~FIN~");

	return 0;
}
