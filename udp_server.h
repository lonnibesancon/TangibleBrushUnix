#ifndef UDP_SERVER
#define UDP_SERVER

#include <iostream>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sstream>
#include "util/linear_math.h"
#include <cstring>
#include "global.h"

#define BUFLEN 512
#define NUMBEROFITEMSINMESSAGE 35

class udp_server{
	
public:
	
	int port ;
	struct sockaddr_in si_me, si_other ;
	int sock ;
	socklen_t slen ;
	int recv_len ;
	char buf[BUFLEN] ;
	bool hasDataChanged 	= false ;
	bool hasDataSetChanged 	= false ;
	bool hasSelectionSet    = false;
	bool hasSelectionClear  = true;
	

	

	std::string previousMessage ;


	udp_server();
	udp_server(int p);
	~udp_server();

	//static void* launch_listen(void* args);
	void listen(void);
	Matrix4 getDataMatrix();
	Matrix4 getSliceMatrix();
	Vector3 getSeedPoint();

	int getDataSet();
	float getZoomFactor();

	bool getShowVolume();
	bool getShowSurface();
	bool getShowStylus();
	bool getShowSlice();
	bool getShowOutline();
	
	short getConsiderX();
	short getConsiderY();
	short getConsiderZ();

	Synchronized<Vector3>     selectionStartPoint;
	Synchronized<std::vector<Matrix4>> selectionMatrix;
	Synchronized<std::vector<Vector3>> selectionPoint;
private:

	void initSocket();
	Synchronized<Matrix4> dataMatrix ;
	Synchronized<Matrix4> sliceMatrix ;
	Synchronized<Vector3> seedPoint ;

	int dataset 		= 1 ;
	float zoomingFactor 	= 1 ;

	bool showVolume 	= true ;
	bool showSurface 	= true ;
	bool showStylus 	= true ;
	bool showSlice 		= true ;
	bool showOutline 	= true ;

	short considerX = 1 ;
	short considerY = 1 ;
	short considerZ = 1 ;


};

#endif
