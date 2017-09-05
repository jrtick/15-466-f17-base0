## Shepherd Dog Game
Forked from Base0, which was written by Jim McCann for 15-466-f17 course.
Modified by Jordan Tick


## Requirements
 - modern C++ compiler
 - glm
 - libSDL2

## Build instructions
I used linux:
 - get libraries by apt-get install libsdl2-dev libglm-dev
 - modified makefile to require the 'std=c++11'
 - no cmd line args, just run make and game is ./main
 - There are some game parameters you can tweak in main.cpp (speed, sizes, etc)

## Game Description
Sheperd Dog Game:
 - Initial state: five sheep in a pentagon around center, dog in bottom-right
 - Sheep get random NSEW direction
 - if sheep collide, move opposite their collision
 - if dog collides with sheep, flips its direction
 - collision with fence and sheep results in game over
 - sheep speed gradually increases over time
 - sheperd dog position is determined by user's mouse position

To see original game design doc (by hungyuc), go to this link:
 - http://graphics.cs.cmu.edu/courses/15-466-f17/game0-designs/hungyuc/
