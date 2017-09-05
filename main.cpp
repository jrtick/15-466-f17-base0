#include "Draw.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>

#include <chrono>
#include <iostream>
#include <stdlib.h> //used for random
#include <time.h> //used for random seed
#include <math.h> //used for sheep positioning

bool close_enough(float x, float y){ //float equality function
	float absdif = x-y;
	if(absdif<0) return absdif >= -0.00001f;
	else         return absdif <=  0.00001f;
}

class Sheep {
public:
	static float speed; //all sheep have same speed
	static float radius; //all sheep are the same size
	static glm::u8vec4 color; //all sheep have the same color

	glm::vec2 pos,vel; //different pos & vel
	bool dogCollide; //true when dog is overlapping sheep so no infinite flipping occurs
	float lastSwitch; //time since last directional switch. Used to add randomness
	Sheep(){}
	Sheep(glm::vec2 pos, glm::vec2 vel){
		this->pos = pos;
		this->vel = vel;
		dogCollide = false;
		lastSwitch = 0;
	}
	bool collision(Sheep other){
		//tl stands for top-left corner
		glm::vec2 tl1 = this->pos - glm::vec2(radius,radius),
		          tl2 = other.pos - glm::vec2(radius,radius);
		return tl1.x < tl2.x + 2*radius && tl1.x + 2*radius > tl2.x &&
		       tl1.y < tl2.y + 2*radius && tl1.y + 2*radius > tl2.y;
	}
	bool collision(glm::vec2 tl2,glm::vec2 bl2){ //used for dog and fence collision
		//tl stands for top-left corner, bl stands for bottom-left corner
		glm::vec2 tl1 = this->pos - glm::vec2(radius,radius);
		return tl1.x < bl2.x && tl1.x + 2*radius > tl2.x &&
		       tl1.y < bl2.y && tl1.y + 2*radius > tl2.y;
	}
};

//GAME PARAMETERS
glm::u8vec4 Sheep::color = glm::u8vec4(0xff,0xff,0xff,0xff); //white
float Sheep::speed = 0.05f;
float Sheep::radius = 0.1f;
#define SHEEP_COUNT 5
#define DOG_SCALE 1.f //as compared to sheep
#define DOG_COLOR glm::u8vec4(0x00,0x00,0x00,0xff) //black
#define FENCE_COLOR glm::u8vec4(0xa7,0x71,0x50,0xff) //brown
#define GROUND_COLOR_HACK 0,1,0,1 //I'm so sorry. Green
#define SPEEDUP 0.001f
#define FENCE_BOUND 0.8f
#define FENCE_RAD 0.025f
#define SHEEP_RESET_TIME 10.f //change sheep directions periodically

int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Game0: Sheep Herder";
		glm::uvec2 size = glm::uvec2(640, 640); //square
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
	bool paused = true; //can pause game with 'p' key
	float total_time = 0; //keep track to tell user their score at the end

	//initialize sheep
	Sheep sheeps[SHEEP_COUNT];
	for(int i=0;i<SHEEP_COUNT;i++){
		//set sheep position
		float angle = 2*3.14159265f/SHEEP_COUNT*i;
		glm::vec2 pos = 0.3f*glm::vec2(sin(angle),cos(angle));
		
		//set random direction
		int randdir = rand() % 4;
		switch(randdir){ //can only go north, south, east, or west
		case 0: //right
			sheeps[i] = Sheep(pos,glm::vec2(1,0));
			break;
		case 1: //left
			sheeps[i] = Sheep(pos,glm::vec2(-1,0));
			break;
		case 2: //up
			sheeps[i] = Sheep(pos,glm::vec2(0,1));
			break;
		default: //down
			sheeps[i] = Sheep(pos,glm::vec2(0,-1));
			break;
		}
	}

	//dog
	glm::vec2 dog = glm::vec2(FENCE_BOUND-FENCE_RAD-DOG_SCALE*Sheep::radius,-FENCE_BOUND+FENCE_RAD+DOG_SCALE*Sheep::radius);
	glm::vec2 mouse = glm::vec2(0.0f, 0.0f);

	//Out of Bounds
	glm::vec2 boundaries[4];
	boundaries[0] = glm::vec2(-FENCE_BOUND,FENCE_BOUND);
	boundaries[1] = glm::vec2(FENCE_BOUND,FENCE_BOUND);
	boundaries[2] = glm::vec2(FENCE_BOUND,-FENCE_BOUND);
	boundaries[3] = glm::vec2(-FENCE_BOUND,-FENCE_BOUND);
	glm::vec2 fence_pad = glm::vec2(FENCE_RAD,FENCE_RAD);

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
			}else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) { //quit game
				should_quit = true;
			}else if (evt.type == SDL_QUIT) {
				should_quit = true;
				break;
			}else if(evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_p){ //pause game
				paused = !paused;
				break;
			}
		}
		if (should_quit) break;

		auto current_time = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration< float >(current_time - previous_time).count();
		previous_time = current_time;

		{ //update game state:
			if(!paused){
				dog = mouse; //update dog pose
				total_time += elapsed;

				for(int i=0;i<SHEEP_COUNT;i++){
					sheeps[i].lastSwitch += elapsed;
					sheeps[i].pos += sheeps[i].vel * Sheep::speed * elapsed; //update sheep pos

					//sheep/OOB collision
					if(sheeps[i].collision(boundaries[0]-fence_pad,boundaries[1]+fence_pad) ||
					   sheeps[i].collision(boundaries[2]-fence_pad,boundaries[1]+fence_pad) ||
					   sheeps[i].collision(boundaries[3]-fence_pad,boundaries[2]+fence_pad) ||
					   sheeps[i].collision(boundaries[3]-fence_pad,boundaries[0]+fence_pad)){
						printf("Game over! You lasted %.2f seconds\n",total_time);
						should_quit = true;
					}

					//sheep/dog collision
					glm::vec2 dogTL = dog - DOG_SCALE*glm::vec2(Sheep::radius,Sheep::radius),
					          dogBL = dog + DOG_SCALE*glm::vec2(Sheep::radius,Sheep::radius);
					if(sheeps[i].collision(dogTL,dogBL)){
						if(!sheeps[i].dogCollide){ //only flip velocity if just collided
							sheeps[i].vel *= -1;
							sheeps[i].dogCollide = true;
						}
					}else if(sheeps[i].dogCollide) sheeps[i].dogCollide = false;

					//sheep/sheep collision
					for(int j=0;j<i;j++){
						if(sheeps[i].collision(sheeps[j])){
							//get pos and vel before collision
							glm::vec2 vel1 = sheeps[i].vel*Sheep::speed,
							          vel2 = sheeps[j].vel*Sheep::speed;
							glm::vec2 prev1 = sheeps[i].pos - vel1*elapsed,
							          prev2 = sheeps[j].pos - vel2*elapsed;

							//reset to pre-collision so collision only happens once
							sheeps[i].pos = prev1;
							sheeps[j].pos = prev2;

							if(close_enough(vel1.x,-vel2.x) &&
							   close_enough(vel1.y,-vel2.y)){
								//direct collision
								sheeps[i].vel *= -1;
								sheeps[j].vel *= -1;
							}else{ //t-bone collision
								//calculate who gets to the collision first
								glm::vec2 dist = prev2-prev1;
								float time1,time2;
								if(close_enough(vel1.x,0.f)){ //i moves in y, j moves in x
									time1 = abs(dist.y/vel1.y);
									time2 = abs(dist.x/vel2.x);
								}else{ //i moves in x, j moves in y
									time1 = abs(dist.x/vel1.x);
									time2 = abs(dist.y/vel2.y);
								}
								
								//based on collision, adjust velocities
								if(time1 <= time2){ //sheep j 'rams' sheep i
									sheeps[i].vel = sheeps[j].vel;
									sheeps[j].vel *= -1;
								}else{ //sheep i 'rams' sheep j
									sheeps[j].vel = sheeps[i].vel;
									sheeps[i].vel *= -1;
								}
							}
						}
					}

					//INTRODUCE MOVEMENT RANDOMNESS PERIODICALLY
					if(sheeps[i].lastSwitch > SHEEP_RESET_TIME){
						//set random direction
						int randdir = rand() % 4;
						switch(randdir){ //can only go north, south, east, or west
						case 0: //right
							sheeps[i].vel = glm::vec2(1,0);
							break;
						case 1: //left
							sheeps[i].vel = glm::vec2(-1,0);
							break;
						case 2: //up
							sheeps[i].vel = glm::vec2(0,1);
							break;
						default: //down
							sheeps[i].vel = glm::vec2(0,-1);
							break;
						}
						sheeps[i].lastSwitch = 0;
					}
				}
				
				Sheep::speed += elapsed*SPEEDUP; //sheep speed increases over time
			}
		}

		//draw output:
		glClearColor(GROUND_COLOR_HACK); //make background green
		glClear(GL_COLOR_BUFFER_BIT);


		{ //draw game state:
			Draw draw;

			//draw out of bounds
			draw.add_rectangle(boundaries[0]-fence_pad,boundaries[1]+fence_pad,FENCE_COLOR);
			draw.add_rectangle(boundaries[1]-fence_pad,boundaries[2]+fence_pad,FENCE_COLOR);
			draw.add_rectangle(boundaries[3]-fence_pad,boundaries[2]+fence_pad,FENCE_COLOR);
			draw.add_rectangle(boundaries[0]-fence_pad,boundaries[3]+fence_pad,FENCE_COLOR);

			//draw sheep
			glm::vec2 rad2 = glm::vec2(Sheep::radius,Sheep::radius);
			for(int i=0;i<SHEEP_COUNT;i++)
				draw.add_rectangle(sheeps[i].pos-rad2,sheeps[i].pos+rad2,Sheep::color);
			//draw dog
			draw.add_rectangle(dog-DOG_SCALE*rad2,dog+DOG_SCALE*rad2,DOG_COLOR);

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
