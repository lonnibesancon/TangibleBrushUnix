#include "global.h"

#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#include "fluids_app.h"
#include "udp_server.h"

#include <pthread.h>
#include <thread>
#include "definitions.h"

#include <signal.h>


void loadDataSet(std::unique_ptr<FluidMechanics> app, int dataset){
	return ;
}


int main()
{
	bool realFullScreen  = false ;
	int32_t selectionID  = -1;
	udp_server server(8500);
	//server.listen();
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		LOGE("Unable to initialize SDL: %s", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Request an OpenGL ES 2.0 context
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// Request double buffering and a depth buffer
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// // Request a stencil buffer of at least 1 bit per pixel
	// SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

	SDL_Window* window = SDL_CreateWindow(
		"Fluids App",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
	);
	if (!window) {
		LOGE("Unable to create window: %s", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}



	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (!context) {
		LOGE("Unable to create context: %s", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Synchronize the swap buffer with vsync
	SDL_GL_SetSwapInterval(1);

	LOGD("OpenGL version: %s", glGetString(GL_VERSION));
	LOGD("GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	// LOGD("OpenGL extensions: %s", glGetString(GL_EXTENSIONS));

	std::unique_ptr<FluidMechanics> app(new FluidMechanics("data"));
	app->rebind();

	app->loadDataSet("data/data/1");
	app->setMatrices(Matrix4::makeTransform(Vector3(0, 0, 120), Quaternion(Vector3::unitX(), -M_PI/4)),
	                 // Matrix4::identity()
	                 Matrix4::makeTransform(Vector3(0, 0, 120))
					 );
  
	float t = 0;
	float t2 = 0;

	//Thread creation
	std::thread th(&udp_server::listen,&server);
	th.detach();
	//th.join();

	Matrix4 dataMatrix = Matrix4::makeTransform(Vector3(0, 0, 400), Quaternion(Vector3::unitX(), -M_PI/4)) ;
	Matrix4 sliceMatrix = Matrix4::makeTransform(Vector3(0, 0, 400)) ;
	Vector3 seedPoint(-10000.0,-10000.0,-10000.0);
	Vector3 prevSeedPoint(-10000.0,-10000.0,-10000.0);


	struct sigaction action;
	sigaction(SIGINT, NULL, &action);
	SDL_Init(SDL_INIT_EVERYTHING);
	sigaction(SIGINT, &action, NULL);
	SDL_Event event;
	bool quit = false ;
	while (!quit) {
		SDL_PollEvent(&event);
		switch(event.type)
	    {
	        case SDL_WINDOWEVENT: // Événement de la fenêtre
	            if ( event.window.event == SDL_WINDOWEVENT_CLOSE ) 
	            {
//					app->saveFinalFile();
	                quit = true ;
	            }
	            break;
	        case SDL_KEYUP: 
	            if ( event.key.keysym.sym == SDLK_ESCAPE ) 
	            {
	                quit = true ;
	            }
				if(event.key.keysym.sym == SDLK_f){
					realFullScreen = !realFullScreen ;
					if(realFullScreen)
						SDL_SetWindowBordered(window, SDL_FALSE);
					else
						SDL_SetWindowBordered(window, SDL_TRUE);
				}
	            break;
	    }

/*		if(server.hasDataSetChanged ){
			int dataset = server.getDataSet();
			if(dataset == ftle){
				app->loadDataSet("data/hemisphere");
			}
			else if(dataset == ironProt){
//				app->loadDataSet("data/ironProt.vtk");
			}
			else if(dataset == head){
				app->loadDataSet("data/galaxy");
			}
			else if(dataset == velocity){
//				app->loadDataSet("data/FTLE7.vtk");
//				app->loadVelocityDataSet("data/Velocities7.vtk");
			}

			server.hasDataChanged = false;
		}
		*/

		if(server.hasSelectionClear)
		{
			app->getSettings()->showSelection = false;
			app->clearSelection();
			server.hasSelectionClear = false;
			selectionID = -1;
		}

		if(server.hasChangeInTraining)
		{
			app->changeInTraining();
			server.hasChangeInTraining = false;
		}

	/*  if(server.hasSelectionSet)
		{
			synchronized(server.selectionMatrix)
			{
				app->setSelectionMatrix(server.selectionMatrix);
			}

		//	synchronized(server.selectionPoint)
		//	{
		//		app->pushBackSelection(server.dataSelected.rbegin()->getSelectionMode(), server.dataSelected.rbegin()->getSelectionPoint());
		//	}

			synchronized(server.selectionStartPoint)
			{
				app->setFirstPoint(server.selectionStartPoint);
			}

			app->getSettings()->showSelection = true;
			server.hasSelectionSet = false;
		}
		*/

		if(server.hasPostTreatmentSet)
		{
			app->setPostTreatment(server.postTreatmentTrans, server.postTreatmentRot);
			server.hasPostTreatmentSet = false;
		}

		if(server.hasSetTabletMatrix)
		{
			app->setTabletMatrix(server.tabletMatrix, server.modelTrans, server.modelRot);
			server.hasSetTabletMatrix = false;
		}

/*  	if(server.hasSubDataChanged)
		{
			synchronized(server.dataTrans)
			{
				synchronized(server.dataRot)
				{
					app->setSubData(server.dataTrans, server.dataRot);
				}
			}
			server.hasSubDataChanged = false;
		}
*/

		if(server.hasInit)
		{
			app->initFromClient();
			server.hasInit = false;
		}

		if(server.hasUpdateNextTrial)
		{
			app->nextTrial();
			server.hasUpdateNextTrial = false;
		}

		if(server.hasUpdateTangoMove)
		{
			app->setTangoMove(server.tangoMove, server.interactionMode);
			server.hasUpdateTangoMove = false;
		}
		
		if(server.hasSetToSelection)
		{
			if(selectionID == -1)
			{
				selectionID=0;
				SelectionMode s;
				s = server.dataSelected[selectionID].getSelectionMode();
				const std::vector<Vector2_f>* points = &(server.dataSelected[selectionID].getSelectionPoint());
				app->pushBackSelection(s, *points);
			}

			int32_t dataSelectedSize=0;
			dataSelectedSize = server.dataSelected.size();

			for(int32_t i=selectionID; i < dataSelectedSize; i++)
			{
				const Matrix4_f* m=NULL;

				do
				{
					int i = server.dataSelected[selectionID].nextIndice();
					m = server.dataSelected[selectionID].getMatrix(i);

					if(m == NULL && dataSelectedSize > selectionID+1)
					{
						selectionID++;
						SelectionMode s;
						s = server.dataSelected[selectionID].getSelectionMode();
						const std::vector<Vector2_f>* points = &(server.dataSelected[selectionID].getSelectionPoint());
						app->pushBackSelection(s, *points);
						continue;
					}

					else if(m != NULL)
					{
						app->updateCurrentSelection(m);
					}
					std::cout << "m coming" << std::endl;
				}while(m != NULL);
			}
			app->updateVolumetricRendering();
			server.hasSetToSelection = false;
		}

		if(server.hasDataChanged)
		{
			server.hasDataChanged = false;

			t2 = 360;
			app->getSettings()->sliceType = SLICE_STYLUS;
			//app->getSettings()->sliceType = SLICE_CAMERA;
			//app->getSettings()->showSlice = true;
	//		app->getSettings()->showSlice = server.getShowSlice();
			app->getSettings()->showSlice = true;
			app->getSettings()->clipDist = t2;

			dataMatrix = server.getDataMatrix();
			sliceMatrix = server.getSliceMatrix();
			//LOGD("dataMatrix = %s", Utility::toString(dataMatrix).c_str());
			//LOGD("sliceMatrix = %s", Utility::toString(sliceMatrix).c_str());
			//LOGD("server.getZoomFactor() = %f", server.getZoomFactor());

			app->getSettings()->zoomFactor = server.getZoomFactor();
			//sliceMatrix = dataMatrix * sliceMatrix ;
	//			seedPoint = server.getSeedPoint();
			app->setMatrices(dataMatrix,sliceMatrix);

		/*  if(prevSeedPoint != seedPoint){
				if(seedPoint == Vector3(-1000000,-1000000,-1000000) || seedPoint == Vector3(-1,-1,-1)){
					std::cout << "Reset Particles" << std::endl ;
					app->resetParticles();
				}
				app->setSeedPoint(seedPoint.x, seedPoint.y, seedPoint.z);
				prevSeedPoint = seedPoint ;
				app->releaseParticles();
			}
		*/
			
			app->getSettings()->considerX = server.getConsiderX();
			app->getSettings()->considerY = server.getConsiderY();
			app->getSettings()->considerZ = server.getConsiderZ();
		}


		/*app->setMatrices(Matrix4::makeTransform(Vector3(0, 0, 380), Quaternion(Vector3::unitX(), -M_PI/4)*Quaternion(Vector3::unitZ(), t)),
		                 // Matrix4::identity()
		                 Matrix4::makeTransform(Vector3(0, 0, 400)));*/
		
		t += 0.005;
		// t2 = 400+std::cos(t)*200;
		
		//LOGD("%f", t2);

		app->render();

		SDL_GL_SwapWindow(window);
		// usleep(16*1000);
	}

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
