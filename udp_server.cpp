#include "udp_server.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sstream>
#include "util/linear_math.h"
#include <iostream>
#include <stdio.h>
#include <string>
#include <stdlib.h>


udp_server::udp_server(){
	int i ;
	port = 8888 ;
	slen = sizeof(si_other) ;
	initSocket();
	previousMessage = "" ;
	hasDataChanged = false ;
	//dataMatrix = Matrix4::makeTransform(Vector3(0, 0, 380));
	dataMatrix = Matrix4::makeTransform(Vector3(0, 0, 400), Quaternion(Vector3::unitX(), -M_PI/4)*Quaternion(Vector3::unitZ(), 0));//,Matrix4::makeTransform(Vector3(0, 0, 400)));
	sliceMatrix = Matrix4::makeTransform(Vector3(0, 0, 400));
	seedPoint = Vector3(-1,-1,-1);
}

udp_server::udp_server(int p){
	int i ;
	port = p ;
	slen = sizeof(si_other) ;
	initSocket();
	previousMessage = "" ;
	hasDataChanged = false ;
	//dataMatrix = Matrix4::makeTransform(Vector3(0, 0, 380));
	dataMatrix = Matrix4::makeTransform(Vector3(0, 0, 400), Quaternion(Vector3::unitX(), -M_PI/4)*Quaternion(Vector3::unitZ(), 0));//,Matrix4::makeTransform(Vector3(0, 0, 400)));
	sliceMatrix = Matrix4::makeTransform(Vector3(0, 0, 400));
	seedPoint = Vector3(-1,-1,-1);
}

udp_server::~udp_server(){
	//close(sock);
}

Matrix4 udp_server::getDataMatrix(){
	//std::cout << "Get Data Matrix " << std::endl ;
	hasDataChanged = false ;
	return dataMatrix ;
}

Matrix4 udp_server::getSliceMatrix(){
	return sliceMatrix;
}

Vector3 udp_server::getSeedPoint(){
	return udp_server::seedPoint ;
}


void udp_server::initSocket(){
	//create a UDP socket
	if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		std::cerr << "Socket" << std::endl ;
	}


	//To make non-blocking ;) 
	//int flags = fcntl(sock, F_GETFL);
	//flags |= O_NONBLOCK;
	//fcntl(sock, F_SETFL, flags);

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

/*void* udp_server::launch_listen(void* args){
	listen((udp_server) args);
}*/

void udp_server::listen(){
	std::cout << "--> Server Start Listening" << std::endl ;
	while(1)
    	{
		//rintf("Waiting for data...");
		fflush(stdout);
		memset(buf,'\0', BUFLEN);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
		{
			//std::cerr << "Received" << std::endl ;
		}
		 
		//print details of the client/peer and the data received
		std::string msg = buf ;
		std::stringstream ss(msg);
		int i = 0 ;
		std::string tok;
		//double allvalues[NUMBEROFITEMSINMESSAGE];
		float oMatrix[16];
		float pMatrix[16];
		float seed[3];
		
		std::cout << "Message = " << msg << std::endl ;
		getline(ss, tok, ';');
		do{
			//std::cout << "i = " << i << " -> " << tok << std::endl ;
			oMatrix[i] = std::stod(tok.c_str());
			i++ ;
		}while(getline(ss, tok, ';') && i < 16);
		std::cout << "Object Matrix Constructed " << std::endl ;
		std::cout << "Remaining Line = " << ss << std::endl ;
		sleep(2);
		do{
		//	std::cout << "i = " << i << " -> " << tok << std::endl ;
			pMatrix[i] = std::stod(tok.c_str());
			i++ ;
		}while(getline(ss, tok, ';') && i < 32);

		std::cout << "Plane Matrix Constructed " << std::endl ;
		sleep(2);
		do{
			//std::cout << "i = " << i << " -> " << tok << std::endl ;
			seed[i] = std::stod(tok.c_str());
			i++ ;
		}while(getline(ss, tok, ';') && i < 35);
		sleep(2);
		std::cout << "Seed Point Constructed " << std::endl ;

		dataMatrix = Matrix4(oMatrix) ;
		sliceMatrix = Matrix4(pMatrix) ;
		seedPoint = Vector3(seed) ;
		hasDataChanged = true ;

    	}
	return NULL ;
	
}
