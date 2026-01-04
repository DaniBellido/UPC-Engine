#include "Globals.h"
#include "ExerciseModule.h"

#include "Exercise1.h"
#include "Exercise2.h"
#include "Exercise3.h"
#include "Exercise4.h"
#include "Exercise5.h"
#include "Exercise6.h"
#include "Exercise7.h"

ExerciseModule::ExerciseModule(D3D12Module* d3d12)
{
	this->d3d12 = d3d12;

	exe1 = new Exercise1();
	exe2 = new Exercise2();
	exe3 = new Exercise3();
	exe4 = new Exercise4();
	exe5 = new Exercise5();
	exe6 = new Exercise6();
	exe7 = new Exercise7();
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
	exe4->init();
	exe5->init();
	exe6->init();
	exe7->init();


	t.Stop();
	Logger::Log("ExerciseModule initialized in: " + std::to_string(t.ReadMs()) + " ms.");
	return true;
}

bool ExerciseModule::cleanUp()
{
	delete exe1;
	delete exe2;
	delete exe3;
	delete exe4;
	delete exe5;
	delete exe6;
	delete exe7;

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

void ExerciseModule::exercise4()
{
	exe4->render();
}

void ExerciseModule::exercise5()
{
	exe5->render();
}

void ExerciseModule::exercise6()
{
	exe6->render();
}

void ExerciseModule::exercise7()
{
	exe7->render();
}



