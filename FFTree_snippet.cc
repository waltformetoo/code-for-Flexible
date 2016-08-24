//test: list port addresses for all edge switches
for (int f = f_begin; f<=f_end; f++){
	for (int e = e_begin; e<=e_end; e++) {
		for (int portIndex=0; portIndex<k_port_count; portIndex++)
			NS_LOG_UNCOND(edgePortAddress(f,e,portIndex));
		NS_LOG_UNCOND("");
	}
}


//test: show server addresses of a /24 subnet
int f = 3; int e = 0;
for (int portIndex=0; portIndex<edge_down_pc; portIndex++) {
	int serverIndex = edge_down_pc * e + portIndex;
	std::cout<<"server "<<serverIndex<<"----------------"<<std::endl;
	Ptr<Node> server = serverGroup[f].Get(serverIndex);
	Ptr<Ipv4> serverIpv4 = server->GetObject<Ipv4> ();
	uint32_t nInterface = serverIpv4->GetNInterfaces();
	for (int i=1; i<nInterface; i++) { //i=1: ignore the loopback
		std::cout<<"interface "<<i<<':'<<std::endl;
		uint32_t nAddress = serverIpv4->GetNAddresses (i);
		for (int a=0; a<nAddress; a++) {
			NS_LOG_UNCOND(serverIpv4->GetAddress (i, a));
		}
	}
}	