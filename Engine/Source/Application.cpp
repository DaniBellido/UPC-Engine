#include "Globals.h"
#include "Application.h"
#include "DebugDrawPass.h"

#include "ModuleInput.h"
#include "D3D12Module.h"

#include "EditorModule.h"
#include "ResourcesModule.h"
#include "ShaderDescriptorsModule.h"
#include "SamplersModule.h"
#include "ExerciseModule.h"
#include "CameraModule.h"



Application::Application(int argc, wchar_t** argv, void* hWnd)
{
    Timer t;
    t.Start();

    modules.push_back(new ModuleInput((HWND)hWnd));
    modules.push_back(d3d12 = new D3D12Module((HWND)hWnd));
    modules.push_back(resources = new ResourcesModule());
    modules.push_back(shaderDescriptors = new ShaderDescriptorsModule());
    modules.push_back(samplers = new SamplersModule());
    modules.push_back(camera = new CameraModule());


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
    static Timer updateTimer, preRenderTimer, renderTimer, postRenderTimer;
    static float updateMs = 0.0f, preRenderMs = 0.0f, renderMs = 0.0f, postRenderMs = 0.0f;

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
        updateTimer.Start();
        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->update();
        updateTimer.Stop();
        updateMs = updateTimer.ReadMs();

        preRenderTimer.Start();
        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->preRender();
        preRenderTimer.Stop();
        preRenderMs = preRenderTimer.ReadMs();

        renderTimer.Start();
        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->render();
        renderTimer.Stop();
        renderMs = renderTimer.ReadMs();

        postRenderTimer.Start();
        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->postRender();
        postRenderTimer.Stop();
        postRenderMs = postRenderTimer.ReadMs();

        app->updateMs = updateMs;
        app->preRenderMs = preRenderMs;
        app->renderMs = renderMs;
        app->postRenderMs = postRenderMs;

    }
}

bool Application::cleanUp()
{
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
