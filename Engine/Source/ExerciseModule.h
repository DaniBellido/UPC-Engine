#pragma once
#include "Module.h"
#include "D3D12Module.h"

class Exercise1;
class Exercise2;
class Exercise3;
class Exercise4;
class Exercise5;
class Exercise6;
class Exercise7;
class Exercise8;

class ExerciseModule : public Module
{
private:
	D3D12Module* d3d12 = nullptr;

	Exercise1* exe1 = nullptr;
	Exercise2* exe2 = nullptr;
	Exercise3* exe3 = nullptr;
	Exercise4* exe4 = nullptr;
	Exercise5* exe5 = nullptr;
	Exercise6* exe6 = nullptr;
	Exercise7* exe7 = nullptr;
	Exercise8* exe8 = nullptr;

public:
	ExerciseModule(D3D12Module* d3d12);
	~ExerciseModule();

	bool init() override;
	bool cleanUp() override;
	void render() override;

	void exercise1();
	void exercise2();
	void exercise3();
	void exercise4();
	void exercise5();
	void exercise6();
	void exercise7();
	void exercise8();

};

