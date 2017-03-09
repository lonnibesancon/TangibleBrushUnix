#ifndef  INTERACTIONMODE_INC
#define  INTERACTIONMODE_INC


#define ADD 1
#define SUB 2
#define INTER 3

#define nothing						0

// Interaction mode for data
#define dataTangible 				1
#define dataTouch 					2
//#define dataHybrid 					3

//Interaction mode for plane
#define planeTouch 					11
#define planeTangible 				12
//#define planeHybrid 				13

// Interaction mode for plane + data
#define dataPlaneTouch 				21
#define dataPlaneTangible 			22
#define dataPlaneHybrid 			23
#define dataTouchTangible 			24
#define planeTouchTangible 			25
#define dataPlaneTouchTangible		26
#define dataPlaneTangibleTouch		27

//Seeding point interaction
#define seedPointTangible 			31
#define seedPointTouch 				32
#define seedPointHybrid 			33


#define touchInteraction 			1
#define tangibleInteraction 		2


#define thresholdRST 				450

#define ftle						0
#define head						1
#define ironprot					2
#define velocities					3

#endif   /* ----- #ifndef INTERACTIONMODE_INC  ----- */
