#pragma once

#include "Globals.h"

#include <array>
#include <vector>
#include <chrono>

class Module;
class D3D12Module;
class ResourcesModule;
class ShaderDescriptorsModule;
class SamplersModule;
class CameraModule;

class DebugDrawPass;

class Application
{
public:

	Application(int argc, wchar_t** argv, void* hWnd);
	~Application();

	bool         init();
	void         update();
	bool         cleanUp();

    D3D12Module* getD3D12() { return d3d12; }
    ResourcesModule* getResources() { return resources; }
    ShaderDescriptorsModule* getShaderDescriptors() { return shaderDescriptors; }
    SamplersModule* getSamplers() { return samplers; }
    CameraModule* getCamera() { return camera; }

    DebugDrawPass* getDebugDrawPass() { return debugDrawPass.get(); }

    float                       getFPS() const { return 1000.0f * float(MAX_FPS_TICKS) / tickSum; }
    float                       getAvgElapsedMs() const { return tickSum / float(MAX_FPS_TICKS); }
    uint64_t                    getElapsedMilis() const { return elapsedMilis; }

    bool                        isPaused() const { return paused; }
    bool                        setPaused(bool p) { paused = p; return paused; }

    float                       getUpdateMs() const { return updateMs; }
    float                       getPreRenderMs() const { return preRenderMs; }
    float                       getRenderMs() const { return renderMs; }
    float                       getPostRenderMs() const { return postRenderMs; }

private:
    enum { MAX_FPS_TICKS = 30 };
    typedef std::array<uint64_t, MAX_FPS_TICKS> TickList;

    std::vector<Module*> modules;

    D3D12Module* d3d12 = nullptr;
    ResourcesModule* resources = nullptr;
    ShaderDescriptorsModule* shaderDescriptors = nullptr;
    SamplersModule* samplers = nullptr;
    CameraModule* camera = nullptr;

    std::unique_ptr<DebugDrawPass> debugDrawPass;

    uint64_t  lastMilis = 0;
    TickList  tickList;
    uint64_t  tickIndex;
    uint64_t  tickSum = 0;
    uint64_t  elapsedMilis = 0;
    bool      paused = false;


    float updateMs = 0.0f;
    float preRenderMs = 0.0f;
    float renderMs = 0.0f;
    float postRenderMs = 0.0f;

};

extern Application* app;
