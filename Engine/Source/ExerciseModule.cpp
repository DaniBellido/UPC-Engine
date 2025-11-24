#include "Globals.h"
#include "ExerciseModule.h"

ExerciseModule::ExerciseModule(D3D12Module* d3d12)
{
	this->d3d12 = d3d12;

	exe1 = new Exercise1();
	exe2 = new Exercise2();
}

ExerciseModule::~ExerciseModule()
{
}

bool ExerciseModule::init()
{
	exe2->init();
	return true;
}

bool ExerciseModule::cleanUp()
{
	delete exe1;
	delete exe2;
	return true;
}

void ExerciseModule::render()
{
	
}

void ExerciseModule::exercise1()
{
	exe1->render();
}

void ExerciseModule::exercise2()
{
	exe2->render();
}



