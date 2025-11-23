#include "Globals.h"
#include "ExerciseModule.h"

ExerciseModule::ExerciseModule(D3D12Module* d3d12)
{
	this->d3d12 = d3d12;

	exe1 = new Exercise1();
}

ExerciseModule::~ExerciseModule()
{
}

bool ExerciseModule::init()
{

	return true;
}

bool ExerciseModule::cleanUp()
{
	delete exe1;
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

}



