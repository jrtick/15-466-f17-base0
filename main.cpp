#include "Draw.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>

#include <chrono>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>

class Sheep {
public:
	static float speed; //all sheep have same speed
	static float radius; //all sheep are the same size
	static glm::vec3 color;
	glm::vec2 pos,vel; //different pos & vel
	Sheep(){}
	Sheep(glm::vec2 pos, glm::vec2 vel){
		this->pos = pos;
		this->vel = vel;
	}
	bool collision(Sheep other){
		//tl stands for top-left corner
		glm::vec2 tl1 = this->pos - glm::vec2(radius,radius),
		          tl2 = other.pos - glm::vec2(radius,radius);
		return tl1.x < tl2.x + 2*radius && tl1.x + 2*radius > tl2.x &&
		       tl1.y < tl2.y + 2*radius && tl1.y + 2*radius > tl2.y;
	}
	bool collision(glm::vec2 tl2,glm::vec2 bl2){
		//tl stands for top-left corner, bl stands for bottom-left corner
		glm::vec2 tl1 = this->pos - glm::vec2(radius,radius);
		return tl1.x < bl2.x && tl1.x + 2*radius > tl2.x &&
		       tl1.y < bl2.y && tl1.y + 2*radius > tl2.y;
	}
};
//starting sheep params
float Sheep::speed = 0.1;
float Sheep::radius = 0.1;
glm::vec3 Sheep::color = glm::vec3(0.5,0.5,0.5);
#define SHEEP_COUNT 5


int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Game0: Tennis For One";
		glm::uvec2 size = glm::uvec2(640, 480);
	} config;

	//------------  initialization ------------

	//Initialize SDL library:
	SDL_Init(SDL_INIT_VIDEO);

	//Ask for an OpenGL context version 3.3, core profile, enable debug:
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	//create window:
	SDL_Window *window = SDL_CreateWindow(
		config.title.c_str(),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		config.size.x, config.size.y,
		SDL_WINDOW_OPENGL /*| SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI*/
	);

	if (!window) {
		std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
		return 1;
	}

	//Create OpenGL context:
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!context) {
		SDL_DestroyWindow(window);
		std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
		return 1;
	}

	#ifdef _WIN32
	//On windows, load OpenGL extensions:
	if (!init_gl_shims()) {
		std::cerr << "ERROR: failed to initialize shims." << std::endl;
		return 1;
	}
	#endif

	//Set VSYNC + Late Swap (prevents crazy FPS):
	if (SDL_GL_SetSwapInterval(-1) != 0) {
		std::cerr << "NOTE: couldn't set vsync + late swap tearing (" << SDL_GetError() << ")." << std::endl;
		if (SDL_GL_SetSwapInterval(1) != 0) {
			std::cerr << "NOTE: couldn't set vsync (" << SDL_GetError() << ")." << std::endl;
		}
	}

	//Hide mouse cursor (note: showing can be useful for debugging):
	SDL_ShowCursor(SDL_DISABLE);

	//------------  game state ------------
	srand(time(NULL)); //random seed

	//SHEEP
	Sheep sheeps[SHEEP_COUNT];
	for(int i=0;i<SHEEP_COUNT;i++){
		int randnum = rand() % 4;
		float angle = 2*3.14159265/SHEEP_COUNT*i;
		glm::vec2 pos = glm::vec2(0.2*sin(angle),0.2*cos(angle));
		switch(randnum){
		case 0:
			sheeps[i] = Sheep(pos,glm::vec2(Sheep::speed,0));
			break;
		case 1:
			sheeps[i] = Sheep(pos,glm::vec2(-Sheep::speed,0));
			break;
		case 2:
			sheeps[i] = Sheep(pos,glm::vec2(0,Sheep::speed));
			break;
		default:
			sheeps[i] = Sheep(pos,glm::vec2(0,-Sheep::speed));
			break;
		}
	}
	float total_time = 0;

	//dog
	glm::vec2 dog = glm::vec2(1,-1);
	glm::vec2 mouse = glm::vec2(0.0f, 0.0f);

	//Out of Bounds
	glm::vec2 boundaries[4];
	boundaries[0] = glm::vec2(-0.8,0.8);
	boundaries[1] = glm::vec2(0.8,0.8);
	boundaries[2] = glm::vec2(0.8,-0.8);
	boundaries[3] = glm::vec2(-0.8,-0.8);

	//------------  game loop ------------

	auto previous_time = std::chrono::high_resolution_clock::now();
	bool should_quit = false;
	while (true) {
		static SDL_Event evt;
		while (SDL_PollEvent(&evt) == 1) {
			//handle input:
			if (evt.type == SDL_MOUSEMOTION) {
				mouse.x = (evt.motion.x + 0.5f) / float(config.size.x) * 2.0f - 1.0f;
				mouse.y = (evt.motion.y + 0.5f) / float(config.size.y) *-2.0f + 1.0f;
			}/* else if (evt.type == SDL_MOUSEBUTTONDOWN) {
				ball = mouse;
				ball_velocity = glm::vec2(0.5f, 0.5f);
			}*/ else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
				should_quit = true;
			} else if (evt.type == SDL_QUIT) {
				should_quit = true;
				break;
			}
		}
		if (should_quit) break;

		auto current_time = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration< float >(current_time - previous_time).count();
		previous_time = current_time;

		{ //update game state:
			dog = mouse;
			glm::vec2 pad = glm::vec2(0.1,0.1);
			for(int i=0;i<SHEEP_COUNT;i++){
				sheeps[i].pos += sheeps[i].vel * elapsed;
				for(int j=0;j<i;j++){
					if(sheeps[i].collision(sheeps[j]))
						sheeps[i].vel *= -1;
				}
				if(sheeps[i].collision(dog-glm::vec2(Sheep::radius,Sheep::radius),dog+glm::vec2(Sheep::radius,Sheep::radius))) sheeps[i].vel *= -1;
				
				if(sheeps[i].collision(boundaries[0]-pad,boundaries[1]+pad) ||
				   sheeps[i].collision(boundaries[1]-pad,boundaries[2]+pad) ||
				   sheeps[i].collision(boundaries[2]-pad,boundaries[3]+pad) ||
				   sheeps[i].collision(boundaries[3]-pad,boundaries[0]+pad))
					should_quit = true;

			}
			
			total_time += elapsed;
			Sheep::speed += elapsed*0.1f;
		}

		//draw output:
		glClearColor(0.0, 1.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);


		{ //draw game state:
			Draw draw;
			//draw out of bounds
			glm::vec2 pad = glm::vec2(0.05,0.05);
			glm::u8vec4 col = glm::u8vec4(0.5,0,0.5,1);
			draw.add_rectangle(boundaries[0]-pad,boundaries[1]+pad,col);
			draw.add_rectangle(boundaries[1]-pad,boundaries[2]+pad,col);
			draw.add_rectangle(boundaries[2]-pad,boundaries[3]+pad,col);
			draw.add_rectangle(boundaries[3]-pad,boundaries[0]+pad,col);

			//draw sheep and dog
			for(int i=0;i<SHEEP_COUNT;i++)
				draw.add_rectangle(sheeps[i].pos-glm::vec2(Sheep::radius,Sheep::radius),sheeps[i].pos+glm::vec2(Sheep::radius,Sheep::radius),glm::u8vec4(0xff,0xff,0xff,0xff));
			draw.add_rectangle(dog-glm::vec2(Sheep::radius,Sheep::radius),dog+glm::vec2(Sheep::radius,Sheep::radius),glm::u8vec4(0xff,0x00,0x00,0xff));
			draw.draw();
		}


		SDL_GL_SwapWindow(window);
	}


	//------------  teardown ------------

	SDL_GL_DeleteContext(context);
	context = 0;

	SDL_DestroyWindow(window);
	window = NULL;

	return 0;
}
