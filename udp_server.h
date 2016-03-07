#ifndef UDP_SERVER
#define UDP_SERVER

#include <iostream>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFLEN 512 

class udp_server{
	
public:
	int port ;
	struct sockaddr_in si_me, si_other ;
	int sock ;
	socklen_t slen ;
	int recv_len ;
	char buf[BUFLEN] ;

	udp_server();
	udp_server(int p);
	~udp_server();

	void listen();

private: 
	void initSocket();

};

#endif