#pragma once
#include "Module.h"
#include "D3D12Module.h"
#include <ImGuiPass.h>

class ViewportModule : public Module
{
private:
    HWND hWnd = NULL;

    D3D12Module* d3d12 = nullptr;
    ImGuiPass* imGuiPass = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};

    bool visible = true;

public:
    ViewportModule(HWND hWnd, D3D12Module* d3d12);
    ~ViewportModule() {}

    bool init() override;
    void preRender() override;
    void render() override;
    bool cleanUp() override;

    bool isVisible() const { return visible; }
    void setVisible(bool value) { visible = value; }
};

