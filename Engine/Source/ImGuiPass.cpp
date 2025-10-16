#include "Globals.h"
#include "ImGuiPass.h"

#include "Application.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"


ImGuiPass::ImGuiPass(ID3D12Device2* device, HWND hWnd, D3D12_CPU_DESCRIPTOR_HANDLE cpuTextHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuTextHandle)
{

    // It's not optimal but makes ImGuiPass independent from ModuleDescriptor slides
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    if (!cpuTextHandle.ptr || !gpuTextHandle.ptr)
    {
        device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
        heap->SetName(L"ImGui Descriptor Heap");

        cpuTextHandle = heap->GetCPUDescriptorHandleForHeapStart();
        gpuTextHandle = heap->GetGPUDescriptorHandleForHeapStart();
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls    
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX12_Init(device, FRAMES_IN_FLIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr, cpuTextHandle, gpuTextHandle);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !

    FILE* fp = fopen("c:\\Windows\\Fonts\\segoeui.ttf", "r");
    if (fp) 
    {
        fclose(fp);
        io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    }
    else
    {
        io.Fonts->AddFontDefault();
    }
        
    io.Fonts->Build();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);
}

ImGuiPass::~ImGuiPass()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiPass::startFrame()
{
    // imgui new frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();

    // imgui commands
    //ImGui::ShowDemoWindow();
}

void ImGuiPass::record(ID3D12GraphicsCommandList* commandList)
{
    BEGIN_EVENT(commandList, "ImGui Pass");

    ImGui::Render();

    // It's not optimal but makes ImGuiPass independent from ModuleDescriptor slides

    if (heap)
    {
        ID3D12DescriptorHeap* descriptorHeaps[] = { heap.Get() };
        commandList->SetDescriptorHeaps(1, descriptorHeaps);
    }

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

    END_EVENT(commandList);
}
