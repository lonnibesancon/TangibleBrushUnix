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

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	std::unique_ptr<FluidMechanics> app(new FluidMechanics("data"));
	app->rebind();

	app->loadDataSet("data/ftlelog.vtk");
	app->setMatrices(Matrix4::makeTransform(Vector3(0, 0, 400), Quaternion(Vector3::unitX(), -M_PI/4)),
	                 // Matrix4::identity()
	                 Matrix4::makeTransform(Vector3(0, 0, 400))
					 );

	float t = 0;
	float t2 = 0;

	//Thread creation
	std::thread th(&udp_server::listen,&server);
	//th.join();

	Matrix4 dataMatrix = Matrix4::makeTransform(Vector3(0, 0, 400), Quaternion(Vector3::unitX(), -M_PI/4)) ;
	Matrix4 sliceMatrix = Matrix4::makeTransform(Vector3(0, 0, 400)) ;
	Vector3 seedPoint ;


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
	                quit = true ;
	            }
	            break;
	        case SDL_KEYUP: 
	            if ( event.key.keysym.sym == SDLK_ESCAPE ) 
	            {
	                quit = true ;
	            }
	            break;
	    }
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if(server.hasDataSetChanged ){
			int dataset = server.getDataSet();
			if(dataset == ftle){
				app->loadDataSet("data/ftlelog.vtk");
			}
			else if(dataset == ironProt){
				app->loadDataSet("data/ironProt.vtk");
			}
			else if(dataset == head){
				app->loadDataSet("data/head.vti");
			}
			else if(dataset == velocity){
				app->loadDataSet("data/Velocities.vtk");
			}
		}
		dataMatrix = server.getDataMatrix();
		sliceMatrix = server.getSliceMatrix();
		//sliceMatrix = dataMatrix * sliceMatrix ;
		seedPoint = server.getSeedPoint();
		app->setMatrices(dataMatrix,sliceMatrix);
		/*app->setMatrices(Matrix4::makeTransform(Vector3(0, 0, 380), Quaternion(Vector3::unitX(), -M_PI/4)*Quaternion(Vector3::unitZ(), t)),
		                 // Matrix4::identity()
		                 Matrix4::makeTransform(Vector3(0, 0, 400)));*/
		
		t += 0.005;
		// t2 = 400+std::cos(t)*200;
		t2 = 360;
		app->getSettings()->sliceType = SLICE_STYLUS;
		//app->getSettings()->sliceType = SLICE_CAMERA;
		app->getSettings()->showSlice = true;
		app->getSettings()->clipDist = t2;
		app->getSettings()->zoomFactor = server.getZoomFactor();
		
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
