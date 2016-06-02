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
	zoomingFactor = 1 ;
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
	zoomingFactor = 1 ;
}

udp_server::~udp_server(){
	//close(sock);
}

Matrix4 udp_server::getDataMatrix(){
	//std::cout << "Get Data Matrix " << std::endl ;

	hasDataChanged = false ;
	
	Matrix4 m ;
	synchronized(dataMatrix){
		m = dataMatrix ;
	}
	return m ;
}

Matrix4 udp_server::getSliceMatrix(){
	Matrix4 m ;
	synchronized(sliceMatrix){
		m = sliceMatrix ;
	}
	return sliceMatrix;
}

Vector3 udp_server::getSeedPoint(){
	Vector3 v ;
	synchronized(seedPoint){
		v = seedPoint ;
	}
	return udp_server::seedPoint ;
}

int udp_server::getDataSet(){
	hasDataSetChanged = false ;
	return dataset ;
}

float udp_server::getZoomFactor(){
	return zoomingFactor ;
}

bool udp_server::getShowVolume(){
	return this->showVolume ;
}

bool udp_server::getShowSurface(){
	return this->showSurface ;
}

bool udp_server::getShowStylus(){
	return this->showStylus ;
}

bool udp_server::getShowSlice(){
	return this->showSlice ;
}

bool udp_server::getShowOutline(){
	return this->showOutline ;
}

short udp_server::getConsiderX(){
	return this->considerX;
}

short udp_server::getConsiderY(){
	return this->considerY;
}

short udp_server::getConsiderZ(){
	return this->considerZ;
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
		std::cout << "Message = " << msg << std::endl ;

		if(msg[0] == ' ')
		{}

		else if(msg[0] == '3' && msg.size() == 1)
		{
			if(!hasSelectionClear)
			{
				synchronized(selectionMatrix)
				{
					selectionMatrix.clear();
				}

				synchronized(selectionPoint)
				{
					selectionPoint.clear();
				}
				hasSelectionClear = true;
			}
		}

		else if(msg[0] == '2' && msg[1] == ';')
		{
			if(!hasSelectionSet)
			{
				std::string tok;
				float matrix[16];
				float firstPoint[3];
				float lastPoint[3];

				//Useless one
				getline(ss, tok, ';');

				//get first point;
				for(uint32_t i=0; i < 3; i++)
				{
					getline(ss, tok, ';');
					firstPoint[i] = std::stof(tok.c_str());
				}

				//Then get the matrix
				for(uint32_t i=0; i < 16; i++)
				{
					getline(ss, tok, ';');
					matrix[i] = std::stof(tok.c_str());
				}

				//Then the last point
				for(uint32_t i=0; i < 3; i++)
				{
					getline(ss, tok, ';');
					lastPoint[i] = std::stof(tok);
				}

				synchronized(selectionMatrix)
				{
					selectionMatrix.push_back(Matrix4(matrix));
				}

				synchronized(selectionStartPoint)
				{
					selectionStartPoint = Vector3(firstPoint[0], firstPoint[1], firstPoint[2]);
				}

				synchronized(selectionPoint)
				{
					selectionPoint.push_back(Vector3(lastPoint[0], lastPoint[1], lastPoint[2]));
				}
				hasSelectionSet = true;
			}
		}

		else
		{
			std::string tok;
			int nbOfElementsToParseFirst = 7 ;
			int i = nbOfElementsToParseFirst ;	
			//double allvalues[NUMBEROFITEMSINMESSAGE];
			float oMatrix[16];
			float pMatrix[16];
			float seed[3];
			int datast = -1 ;

			int tmpbool = -1 ;
			
			//First we set the dataset
			getline(ss, tok, ';');
			datast = std::stoi(tok.c_str());

			//Then the zooming Factor
			getline(ss, tok, ';');
			zoomingFactor = std::stod(tok.c_str());

			//Get the booleans
			getline(ss, tok, ';');
			tmpbool = std::stoi(tok.c_str());
			showVolume = (tmpbool == 0 )? false : true ;
			getline(ss, tok, ';');
			tmpbool = std::stoi(tok.c_str());
			showSurface = (tmpbool == 0 )? false : true ;
			getline(ss, tok, ';');
			tmpbool = std::stoi(tok.c_str());
			showStylus = (tmpbool == 0 )? false : true ;
			getline(ss, tok, ';');
			tmpbool = std::stoi(tok.c_str());
			showSlice = (tmpbool == 0 )? false : true ;
			getline(ss, tok, ';');
			tmpbool = std::stoi(tok.c_str());
			showOutline = (tmpbool == 0 )? false : true ;

			//Finally the matrices and seeding point
			getline(ss, tok, ';');
			do{
				//std::cout << "i = " << i << " -> " << tok << std::endl ;
				oMatrix[i-nbOfElementsToParseFirst] = std::stod(tok.c_str());
				i++ ;
			}while(getline(ss, tok, ';') && i < 16+nbOfElementsToParseFirst );
			//std::cout << "Object Matrix Constructed " << std::endl ;
			//std::cout << "Remaining Line = " << ss << std::endl ;
			//sleep(2);


			do{
			//	std::cout << "i = " << i << " -> " << tok << std::endl ;
				pMatrix[i-nbOfElementsToParseFirst-16] = std::stod(tok.c_str());
				i++ ;
			}while(getline(ss, tok, ';') && i < 32+nbOfElementsToParseFirst );
			//std::cout << "Plane Matrix Constructed " << std::endl ;
			//sleep(2);
			

			do{
				//std::cout << "i = " << i << " -> " << tok << std::endl ;
				seed[i-32-nbOfElementsToParseFirst] = std::stod(tok.c_str());
				i++ ;
			}while(getline(ss, tok, ';') && i < 35+nbOfElementsToParseFirst );
			//sleep(2);
			//std::cout << "Seed Point Constructed " << std::endl ;


			synchronized(dataMatrix){
				dataMatrix = Matrix4(oMatrix) ;
				//printAny(dataMatrix, "dataMatrix = ");
			}
			synchronized(sliceMatrix){
				sliceMatrix = Matrix4(pMatrix) ;
				//printAny(sliceMatrix, "sliceMatrix = ");
			}
			synchronized(seedPoint){
				seedPoint = Vector3(seed) ;
			}
			if(dataset!= datast){
				dataset = datast ;
				hasDataSetChanged = true ;
			}
			
			//Now for the display only part: constrains on axis + what to show
			int consider = -1 ;
			//We don't need to use getline again, it was stored in the last do-while loop		getline(ss, tok, ';');
			std::cout << "SS ======" << tok.c_str() << std::endl ;
			consider = std::stoi(tok.c_str());
			this->considerX = consider ;
			getline(ss, tok, ';');
			std::cout << "SS ======" << tok.c_str() << std::endl ;
			consider = std::stoi(tok.c_str());
			this->considerY = consider ;
			getline(ss, tok, ';');
			std::cout << "SS ======" << tok.c_str() << std::endl ;
			consider = std::stoi(tok.c_str());
			this->considerZ = consider ;
			
			
			hasDataChanged = true ;
		}
	}
	return NULL ;
}
