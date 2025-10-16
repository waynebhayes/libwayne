// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include "randomSeed.hpp"
#include <cassert>
#include <random>
#include <unistd.h>
#include <ctime>

using namespace std;

//this should be refactored without global variables -Nil
unsigned long int currentSeed;
static bool doneInit = false;

void setSeed(unsigned long int seed) {
	assert(!doneInit);
	currentSeed = seed;
	doneInit = true;
}

void setRandomSeed() {
	assert(!doneInit);
	currentSeed = gethostid() + time(0) + getpid();
	// random_device rd;
	// currentSeed += rd(); //rd() fails on Jenkins.
	doneInit = true;
}

unsigned long int getRandomSeed() {
	if (not doneInit) setRandomSeed();
	return currentSeed;
}
