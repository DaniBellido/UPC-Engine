#pragma once
#include "Module.h"
#include "D3D12Module.h"
class ExerciseModule : public Module
{
private:
	D3D12Module* d3d12 = nullptr;

public:
	ExerciseModule(D3D12Module* d3d12);
	~ExerciseModule();

	bool init() override;
	bool cleanUp() override;
	void render() override;

	void Exercise1();

};

