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

int datasetorder[4] = {0, 0, 0, 0};

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
	hasSetToSelection = false;
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
	hasSetToSelection = false;
}

udp_server::~udp_server(){
	//close(sock);
}

Matrix4 udp_server::getDataMatrix(){
	//std::cout << "Get Data Matrix " << std::endl ;

	hasDataChanged = false ;
	
	Matrix4 m ;
	m = dataMatrix ;
	return m ;
}

Matrix4 udp_server::getSliceMatrix(){
	Matrix4 m ;
	m = sliceMatrix ;
	return sliceMatrix;
}

Vector3 udp_server::getSeedPoint(){
	Vector3 v ;
	v = seedPoint ;
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

		if(msg[0] == '3' && msg.size() == 1)
		{
			if(!hasSelectionClear)
			{
				selectionMatrix.clear();
				selectionPoint.clear();
				dataSelected.clear();
				hasSelectionClear = true;
			}
		}

	/*  else if(msg[0] == '2' && msg[1] == ';')
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
	*/

		//The postTreatment matrix
		else if(msg[0] == '4')
		{
			std::string tok;
			SelectionMode s;
			float data[7];

			double scaleFactorX, scaleFactorY;

			//Useless one
			getline(ss, tok, ';');

			getline(ss, tok, ';');
			scaleFactorX = std::stof(tok.c_str());
			getline(ss, tok, ';');
			scaleFactorY = std::stof(tok.c_str());

			//Then get the matrix
			for(uint32_t i=0; i < 7; i++)
			{
				getline(ss, tok, ';');
				data[i] = std::stof(tok.c_str());
			}

			postTreatmentTrans = Vector3_f(data[0], data[1], data[2]);
			postTreatmentRot = Quaternion_f(data[3], data[4], data[5], data[6]);

			//Need to position the tablet position on the screen
			postTreatmentMat = Matrix4_f::makeTransform(postTreatmentTrans, postTreatmentRot, Vector3_f(1.0, 1.0, 1.0));

			if(tangoMove && hasSelectionSet)
			{
				if(dataSelected.size() > 0)
					dataSelected.rbegin()->addPostTreatmentMatrix(s, scaleFactorX, scaleFactorY, postTreatmentMat);
				hasSetToSelection = true;
			}

			hasPostTreatmentSet = true;
		}

		//The difference between the postTreatment matrix and the model matrix (for the tablet).
		//It is now useless
		/* 
		else if(msg[0] == '5')
		{
			if(!hasSubDataChanged)
			{
				std::string tok;
				float data[7];

				//Useless one
				getline(ss, tok, ';');

				//Then get the matrix
				for(uint32_t i=0; i < 7; i++)
				{
					getline(ss, tok, ';');
					data[i] = std::stof(tok.c_str());
				}

				synchronized(dataTrans)
				{
					dataTrans = Vector3(data[0], data[1], data[2]);
				}

				synchronized(dataRot)
				{
					dataRot = Quaternion(data[3], data[4], data[5], data[6]);
				}

				synchronized(postTreatmentMat)
				{
					postTreatmentMat.push_back(Matrix4(matrix));
				}

				hasSubDataChanged = true;
			}
		}
		*/
		
		else if(msg[0] == '6')
		{
			//Useless one
			std::string tok;
			getline(ss, tok, ';');
			getline(ss, tok, ';');
			uint32_t mode = std::stoi(tok.c_str());

			std::vector<Vector2_f> points;
			std::vector<float> datas;
			while(getline(ss, tok, ';'))
			{
				datas.push_back(std::stof(tok.c_str()));
			}

			for(uint32_t i=0; i < datas.size(); i+= 2)
				points.push_back(Vector2_f(datas[i], datas[i+1]));

			dataSelected.push_back(Selection((SelectionMode) mode, points));

			hasSelectionSet = true;
		}

		//Tablet matrix
		else if(msg[0]=='7')
		{
			std::string tok;
			SelectionMode s;
			float data[16];
			//Useless one
			getline(ss, tok, ';');

			//Then get the matrix
			for(uint32_t i=0; i < 16; i++)
			{
				getline(ss, tok, ';');
				data[i] = std::stof(tok.c_str());
			}

			tabletMatrix = Matrix4(data);

			float mTrans[3];
			float mRot[4];

			for(uint32_t i=0; i < 3; i++)
			{
				getline(ss, tok, ';');
				mTrans[i] = std::stof(tok.c_str());
			}

			for(uint32_t i=0; i < 4; i++)
			{
				getline(ss, tok, ';');
				mRot[i] = std::stof(tok.c_str());
			}

			modelTrans = Vector3_f(mTrans);
			modelRot = Quaternion(mRot[0], mRot[1], mRot[2], mRot[3]);
			hasSetTabletMatrix=true;
		}

		else if(msg[0] == '9')
		{
			std::string tok;
			getline(ss, tok, ';');

			getline(ss, tok, ';');
			userID = std::stoi(tok.c_str());
			hasInit=true;

			if(userID <= 2){
				datasetorder[0] = 1 ;
				datasetorder[1] = 2 ;
				datasetorder[2] = 4 ;
				datasetorder[3] = 3 ;
			}
			else if(userID <= 4){
				datasetorder[0] = 2 ;
				datasetorder[1] = 3 ;
				datasetorder[2] = 1 ;
				datasetorder[3] = 4 ;
			}
			else if(userID <= 6){
				datasetorder[0] = 3 ;
				datasetorder[1] = 4 ;
				datasetorder[2] = 2 ;
				datasetorder[3] = 1 ;
			}
			else if(userID <= 8){
				datasetorder[0] = 4 ;
				datasetorder[1] = 1 ;
				datasetorder[2] = 3 ;
				datasetorder[3] = 2 ;
			}
			else if(userID <= 10){
				datasetorder[0] = 1 ;
				datasetorder[1] = 4 ;
				datasetorder[2] = 2 ;
				datasetorder[3] = 3 ;
			}
			else if(userID <= 12){
				datasetorder[0] = 2 ;
				datasetorder[1] = 1 ;
				datasetorder[2] = 3 ;
				datasetorder[3] = 4 ;
			}
			else if(userID <= 14){
				datasetorder[0] = 3 ;
				datasetorder[1] = 2 ;
				datasetorder[2] = 4 ;
				datasetorder[3] = 1 ;
			}
			else if(userID <= 16){
				datasetorder[0] = 4 ;
				datasetorder[1] = 3 ;
				datasetorder[2] = 1 ;
				datasetorder[3] = 2 ;
			}
		}

		else if(msg[0] == '8')
		{
			std::string tok;
			getline(ss, tok, ';');

			getline(ss, tok, ';');
			tangoMove = std::stoi(tok.c_str());

			getline(ss, tok, ';');
			interactionMode = std::stoi(tok.c_str());
			hasUpdateTangoMove = true;
		}

		else if(msg[0] == 'a')
		{
			hasUpdateNextTrial = true;
		}

		else if(msg[0] == '0')
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

			//Useless one
			getline(ss, tok, ';');
			
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


			dataMatrix = Matrix4(oMatrix) ;
			//printAny(dataMatrix, "dataMatrix = ");
			sliceMatrix = Matrix4(pMatrix) ;
			//printAny(sliceMatrix, "sliceMatrix = ");
			seedPoint = Vector3(seed) ;
//			if(dataset!= datast){
//				dataset = datast ;
//				hasDataSetChanged = true ;
//			}
			
			//Now for the display only part: constrains on axis + what to show
			int consider = -1 ;
			//We don't need to use getline again, it was stored in the last do-while loop		getline(ss, tok, ';');
			std::cout << "SS ======" << tok.c_str() << std::endl ;
			printf("%s \n", tok.c_str());
			consider = std::stoi(tok.c_str());
			this->considerX = consider ;
			getline(ss, tok, ';');

			std::cout << "SS ======" << tok.c_str() << std::endl ;
			printf("%s \n", tok.c_str());
			consider = std::stoi(tok.c_str());
			this->considerY = consider ;
			getline(ss, tok, ';');

			std::cout << "SS ======" << tok.c_str() << std::endl ;
			printf("%si \n", tok.c_str());
			consider = std::stoi(tok.c_str());
			this->considerZ = consider ;
			
			hasDataChanged = true ;
		}
	}
	return NULL ;
}
