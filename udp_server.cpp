#include "udp_server.h"
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h> 


udp_server::udp_server(){
	int i ;
	port = 8888 ;
	slen = sizeof(si_other) ;
	initSocket();
}

udp_server::udp_server(int p){
	int i ;
	port = p ;
	slen = sizeof(si_other) ;
	initSocket();
}

udp_server::~udp_server(){
	close(sock);
}


void udp_server::initSocket(){
	//create a UDP socket
	if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		std::cerr << "Socket" << std::endl ;
	}

	int flags = fcntl(sock, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(sock, F_SETFL, flags);

	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind socket to port
	if( bind(sock , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
	{
		std::cerr << "Bind" << std::endl ;
	}
}

void udp_server::listen(){
	while(1)
    {
        printf("Waiting for data...");
        fflush(stdout);
         
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
        	std::cerr << "Received" << std::endl ;
        }
         
        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Data: %s\n" , buf);
         
        //now reply the client with the same data
        if (sendto(sock, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
        {
            std::cerr << "Send" << std::endl ;
        }

        std::cout << "test" << std::endl ;
    }
}