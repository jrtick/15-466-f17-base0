##Forked from Base0, which was written by Jim McCann for 15-466-f17 course.
Modified by Jordan Tick and released into public domain.

## Requirements
 - modern C++ compiler
 - glm
 - libSDL2

On Linux or OSX these requirements should be available from your package manager without too much hassle.
I used linux, which first required
	apt-get install libsdl2-dev libglm-dev
Then I modified the makefile to required the std=c++11 and now it runs!

Sheperd Dog Game:
*Initial state: five sheep in a pentagon around center, dog in bottom-right
*Sheep get random NSEW direction

*if sheep collide, move opposite their collision face
*if hunter collides with sheep, changes its direction
*collision with OOB and sheep results in game over
*sheep speed gradually increases

*sheperd dog moves by mouse


This game is based off of the following design document:
	http://graphics.cs.cmu.edu/courses/15-466-f17/game0-designs/hungyuc/
