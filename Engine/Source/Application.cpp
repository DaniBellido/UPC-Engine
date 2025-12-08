#include "Globals.h"
#include "Application.h"
#include "ModuleInput.h"
#include "D3D12Module.h"
#include "EditorModule.h"
#include "ResourcesModule.h"
#include "ShaderDescriptorsModule.h"
#include "ExerciseModule.h"



Application::Application(int argc, wchar_t** argv, void* hWnd)
{
    Timer t;
    t.Start();

    modules.push_back(new ModuleInput((HWND)hWnd));
    modules.push_back(d3d12 = new D3D12Module((HWND)hWnd));
    modules.push_back(resources = new ResourcesModule());
    modules.push_back(shaderDescriptors = new ShaderDescriptorsModule());


    // Rendering exercises
    modules.push_back(new ExerciseModule(d3d12));
    // Last Module to be pushed must be the Editor Module
    modules.push_back(new EditorModule((HWND)hWnd, d3d12));

    t.Stop();
    Logger::Log("Modules pushed in: " + std::to_string(t.ReadMs()) + " ms");
}

Application::~Application()
{
    cleanUp();

	for(auto it = modules.rbegin(); it != modules.rend(); ++it)
    {
        delete *it;
    }
}
 
bool Application::init()
{
    Logger::Log("Initializing Application...");
    Timer t;
    t.Start();

	bool ret = true;

	for(auto it = modules.begin(); it != modules.end() && ret; ++it)
		ret = (*it)->init();

    debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getCommandQueue());

    lastMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    t.Stop();
    Logger::Log("Applicaion initialized in: " + std::to_string(t.ReadMs()) + " ms");

	return ret;
}

void Application::update()
{
    using namespace std::chrono_literals;

    // Update milis
    uint64_t currentMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    elapsedMilis = currentMilis - lastMilis;
    lastMilis = currentMilis;
    tickSum -= tickList[tickIndex];
    tickSum += elapsedMilis;
    tickList[tickIndex] = elapsedMilis;
    tickIndex = (tickIndex + 1) % MAX_FPS_TICKS;

    if (!app->paused)
    {
        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->update();

        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->preRender();

        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->render();

        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->postRender();
    }
}

bool Application::cleanUp()
{
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
