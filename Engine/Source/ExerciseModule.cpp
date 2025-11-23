#include "Globals.h"
#include "ExerciseModule.h"

ExerciseModule::ExerciseModule(D3D12Module* d3d12)
{
	this->d3d12 = d3d12;
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
	/*d3d12->cleanUp();
	delete d3d12;*/
	return true;
}

void ExerciseModule::render()
{
	
}

void ExerciseModule::Exercise1()
{
	// Get render descriptor
	auto rtvHandle = d3d12->getRenderTargetDescriptor();

	// RGBA (1.0f, 0.0f, 0.0f, 1.0f)
	const float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };

	// Clear the current RTV
	d3d12->getCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}
