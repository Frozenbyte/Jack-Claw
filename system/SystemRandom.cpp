
#include "precompiled.h"

#pragma warning(disable:4103)
#pragma warning(disable:4786)

#include <stdlib.h>
#include <time.h>


#include "SystemRandom.h"
#include "../util/Debug_MemoryManager.h"

SystemRandom *SystemRandom::instance = NULL;


SystemRandom *SystemRandom::getInstance()
{
	if (instance == NULL)
	{
		instance = new SystemRandom();
	}
	return instance;
}

void SystemRandom::cleanInstance()
{
	if (instance != NULL)
	{
		delete instance;
		instance = NULL;
	}
}

SystemRandom::SystemRandom()
{
  srand(static_cast<unsigned int>(time(0)));
}

SystemRandom::~SystemRandom()
{
  // nop
}

int SystemRandom::nextInt()
{
  return rand(); // % SYSTEMRANDOM_MAX_VALUE;
}

