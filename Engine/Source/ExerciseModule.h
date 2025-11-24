#pragma once
#include "Module.h"
#include "D3D12Module.h"
#include "Exercise1.h"
#include "Exercise2.h"

class ExerciseModule : public Module
{
private:
	D3D12Module* d3d12 = nullptr;

	Exercise1* exe1 = nullptr;
	Exercise2* exe2 = nullptr;

public:
	ExerciseModule(D3D12Module* d3d12);
	~ExerciseModule();

	bool init() override;
	bool cleanUp() override;
	void render() override;

	void exercise1();
	void exercise2();

};

