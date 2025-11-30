#include "Globals.h"
#include "ExerciseModule.h"

ExerciseModule::ExerciseModule(D3D12Module* d3d12)
{
	this->d3d12 = d3d12;

	exe1 = new Exercise1();
	exe2 = new Exercise2();
	exe3 = new Exercise3();
}

ExerciseModule::~ExerciseModule()
{
}

bool ExerciseModule::init()
{
	Logger::Log("Initializing ExerciseModule...");
	Timer t;
	t.Start();

	exe2->init();
	exe3->init();

	t.Stop();
	Logger::Log("ExerciseModule initialized in: " + std::to_string(t.ReadMs()) + " ms.");
	return true;
}

bool ExerciseModule::cleanUp()
{
	delete exe1;
	delete exe2;
	delete exe3;
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

void ExerciseModule::exercise3()
{
	exe3->render();
}



