#define DEBUG_DRAW_IMPLEMENTATION

#include "Globals.h"
#include "DebugDrawPass.h"

#include "SimpleMath.h"

#include <d3dcompiler.h>
#include "d3dx12.h"


static const char linePointSource[] = R"(
    cbuffer Transforms : register(b0)
    {
        float4x4 mvp;
    };

    struct VertexInput
    {
        float3 position : POSITION;
        float3 color    : COLOR;
    };

    struct VertexOutput
    {
        float4 position : SV_POSITION;
        float3 color    : COLOR;
    };

    VertexOutput linePointVS(VertexInput input) 
    {
        VertexOutput output;
        output.position = mul(float4(input.position, 1.0), mvp);
        output.color    = input.color;

        return output;
    }

    float4 linePointPS(VertexOutput input) : SV_TARGET
    {
        return float4(input.color, 1.0);
    }
)";

static const char textSource[] = R"(
    cbuffer ScreenDimensions : register(b0)
    {
        float2 screenDimensions;
    };

    struct VertexInput
    {
        float2 position : POSITION;
        float2 texCoord : TEXCOORD;
        float3 color    : COLOR;
    };

    struct VertexOutput
    {
        float4 position : SV_POSITION;
        float2 texCoord : TEXCOORD;
        float3 color : COLOR;
    };
    
    VertexOutput textVS(VertexInput input) 
    {
        VertexOutput output;

        float x = ((2.0 * (input.position.x - 0.5)) / screenDimensions.x) - 1.0;
        float y = 2.0*(1.0-((input.position.y-0.5)/screenDimensions.y))-1.0; 
        
        output.position = float4(x, y, 0.0, 1.0);
        output.texCoord = input.texCoord;
        output.color    = input.color;

        return output;
    }

    Texture2D glyphTexture : register(t0);
    SamplerState glyphSampler : register(s0);

    float4 textPS(VertexOutput input) : SV_TARGET
    {
        float alpha = glyphTexture.Sample(glyphSampler, input.texCoord).r;
        return float4(1.0, 1.0, 1.0, alpha); 
    }
    
)";

using namespace DirectX;

class DDRenderInterfaceCoreD3D12 final : public dd::RenderInterface
{
public:
    friend class DebugDrawPass;

    DDRenderInterfaceCoreD3D12(ID3D12Device4* _device, ID3D12CommandQueue* _uploadQueue, D3D12_CPU_DESCRIPTOR_HANDLE cpuText, D3D12_GPU_DESCRIPTOR_HANDLE gpuText)
    {
        device = _device;
        uploadQueue = _uploadQueue;
        cpuTextHandle = cpuText;
        gpuTextHandle = gpuText;

        setupLinePointVertexBuffers();
        setupUploadCommandBuffer();
        setupLinePointPipeline();
        setupTextPipeline();
        setupTextVertexBuffers();
    }

    ~DDRenderInterfaceCoreD3D12()
    {
        if (uploadEvent) CloseHandle(uploadEvent);
    }

    void setupUploadCommandBuffer()
    {
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
        device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList));

        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence));
        uploadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    void setupTextPipeline()
    {
        ComPtr<ID3DBlob> errorBuff;

        unsigned flags = D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ALL_RESOURCES_BOUND;

        D3DCompile(textSource, sizeof(textSource), "TextVS", nullptr, nullptr, "textVS", "vs_5_0", flags, 0, &textVS, nullptr);
        D3DCompile(textSource, sizeof(textSource), "TextPS", nullptr, nullptr, "textPS", "ps_5_0", flags, 0, &textPS, nullptr);

        CD3DX12_ROOT_PARAMETER textRootParams[2];
        D3D12_DESCRIPTOR_RANGE tableRange{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0 };

        CD3DX12_ROOT_PARAMETER::InitAsConstants(textRootParams[0], sizeof(Vector2) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(textRootParams[1], 1, &tableRange, D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_STATIC_SAMPLER_DESC sampler = { D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP , D3D12_TEXTURE_ADDRESS_MODE_CLAMP ,
                                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0, 0, D3D12_COMPARISON_FUNC_NEVER, D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
                                              0.0f, D3D12_FLOAT32_MAX , 0, 0, D3D12_SHADER_VISIBILITY_PIXEL };

        CD3DX12_ROOT_SIGNATURE_DESC textRootDesc;
        textRootDesc.Init(2, &textRootParams[0], 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                                              D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | 
                                                              D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                                                              D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS | 
                                                              D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS);
        ComPtr<ID3DBlob> rootSignatureBlob;

        D3D12SerializeRootSignature(&textRootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr);
        device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&textSignature));

        D3D12_INPUT_ELEMENT_DESC inputLayout[] = { {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                   {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}, 
                                                   {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC textPSODesc = {};
        textPSODesc.InputLayout = { inputLayout, sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC) };
        textPSODesc.pRootSignature = textSignature.Get();
        textPSODesc.VS = { textVS->GetBufferPointer(),  textVS->GetBufferSize() };
        textPSODesc.PS = { textPS->GetBufferPointer(), textPS->GetBufferSize() };
        textPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        textPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        textPSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        textPSODesc.SampleDesc = { 1, 0 };
        textPSODesc.SampleMask = 0xffffffff;
        textPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        textPSODesc.NumRenderTargets = 1;

        textPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
        textPSODesc.BlendState.IndependentBlendEnable = FALSE;
        textPSODesc.BlendState.RenderTarget[0] = { TRUE, FALSE, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD, 
                                                   D3D12_BLEND_ZERO, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD, 
                                                   D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };

        textPSODesc.DepthStencilState = { FALSE, D3D12_DEPTH_WRITE_MASK_ALL, D3D12_COMPARISON_FUNC_LESS, FALSE,
                                          D3D12_DEFAULT_STENCIL_READ_MASK, D3D12_DEFAULT_STENCIL_WRITE_MASK,
                                          { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS }, 
                                          { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS } };

        textPSODesc.RasterizerState.FrontCounterClockwise = TRUE;

        device->CreateGraphicsPipelineState(&textPSODesc, IID_PPV_ARGS(&textPSO));
    }

    void setupLinePointPipeline()
    {
        ComPtr<ID3DBlob> errorBuff;

        unsigned flags = D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ALL_RESOURCES_BOUND;

        D3DCompile(linePointSource, sizeof(linePointSource), "LinePointVS", nullptr, nullptr, "linePointVS", "vs_5_0", flags, 0, &linePointVS, &errorBuff);
        D3DCompile(linePointSource, sizeof(linePointSource), "LinePointPS", nullptr, nullptr, "linePointPS", "ps_5_0", flags, 0, &linePointPS, &errorBuff);

        CD3DX12_ROOT_PARAMETER linePointRootParams[1];

        CD3DX12_ROOT_PARAMETER::InitAsConstants(linePointRootParams[0], sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_ROOT_SIGNATURE_DESC linePointRootDesc;
        linePointRootDesc.Init(1, &linePointRootParams[0], 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                                                       D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                                                       D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                                                       D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                                                                       D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
                                                                       D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS);
        ComPtr<ID3DBlob> rootSignatureBlob;

        D3D12SerializeRootSignature(&linePointRootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr);
        device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pointLineSignature));

        D3D12_INPUT_ELEMENT_DESC inputLayout[] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                   {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pointPSODesc = {};
        pointPSODesc.InputLayout = { inputLayout, UINT(std::size(inputLayout)) };
        pointPSODesc.pRootSignature = pointLineSignature.Get();
        pointPSODesc.VS = { linePointVS->GetBufferPointer(),  linePointVS->GetBufferSize() };
        pointPSODesc.PS = { linePointPS->GetBufferPointer(), linePointPS->GetBufferSize() };
        pointPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        pointPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pointPSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pointPSODesc.SampleDesc = { 1, 0 };
        pointPSODesc.SampleMask = 0xffffffff;
        pointPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        pointPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        pointPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        pointPSODesc.NumRenderTargets = 1;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pointPSODescNoDepth = pointPSODesc;
        pointPSODescNoDepth.DepthStencilState.DepthEnable = FALSE;

        device->CreateGraphicsPipelineState(&pointPSODesc, IID_PPV_ARGS(&pointPSO));
        device->CreateGraphicsPipelineState(&pointPSODescNoDepth, IID_PPV_ARGS(&pointPSONoDepth));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC linePSODesc = pointPSODesc;
        linePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC linePSODescNoDepth = linePSODesc;
        linePSODescNoDepth.DepthStencilState.DepthEnable = FALSE;

        device->CreateGraphicsPipelineState(&linePSODesc, IID_PPV_ARGS(&linePSO));
        device->CreateGraphicsPipelineState(&linePSODescNoDepth, IID_PPV_ARGS(&linePSONoDepth));
    }
     
    void createBuffer(unsigned bufferSize, ComPtr<ID3D12Resource>& buffer, D3D12_VERTEX_BUFFER_VIEW& view)
    {
        // TODO : Test two-step loading in the Graphics queue and UMA NUMA
        CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

        device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,IID_PPV_ARGS(&buffer));

        view.BufferLocation = buffer->GetGPUVirtualAddress();
        view.StrideInBytes = sizeof(dd::DrawVertex);
        view.SizeInBytes = bufferSize;
    }

    void setupTextVertexBuffers()
    {
        createBuffer(DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex), textBuffer, textBufferView);
    }

    void setupLinePointVertexBuffers()
    {
        createBuffer(DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex), lineBuffer, lineBufferView);
        createBuffer(DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex), pointBuffer, pointBufferView);
        lineBuffer->SetName(L"DebugDraw LineBuffer");
        pointBuffer->SetName(L"DebugDraw PointBuffer");
    }

    void beginDraw() override { }    
    void endDraw()   override { }

    void recordCommands(const dd::DrawVertex* vertices, int count, ID3D12Resource* vertexBuffer, const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, 
                        ID3D12PipelineState* pso, ID3D12RootSignature* signature, void* rootConstants, uint32_t rootConstantsSize,
                        D3D_PRIMITIVE_TOPOLOGY topology, size_t& memoryOffset, bool isText)
    {
        size_t freeSpace = DEBUG_DRAW_VERTEX_BUFFER_SIZE - memoryOffset;
        if (freeSpace < count)
        {
            memoryOffset = 0;
            freeSpace = DEBUG_DRAW_VERTEX_BUFFER_SIZE;
        }

        if (freeSpace > count)
        {
            BYTE* uploadData = nullptr;
            D3D12_RANGE range = { memoryOffset * sizeof(dd::DrawVertex), memoryOffset * sizeof(dd::DrawVertex) + sizeof(dd::DrawVertex) * count };
            vertexBuffer->Map(0, &range, (void**)&uploadData);
            memcpy(uploadData, vertices, sizeof(dd::DrawVertex) * count);
            vertexBuffer->Unmap(0, nullptr);

            memoryOffset += count;

            D3D12_VIEWPORT viewport;
            viewport.TopLeftX = viewport.TopLeftY = 0;
            viewport.MinDepth = 0.0f; 
            viewport.MaxDepth = 1.0f;
            viewport.Width    = float(width);
            viewport.Height   = float(height);

            D3D12_RECT scissor;
            scissor.left = 0;
            scissor.top = 0;
            scissor.right = width;
            scissor.bottom = height;

            commandList->SetPipelineState(pso);
            commandList->SetGraphicsRootSignature(signature);
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissor);
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            commandList->IASetPrimitiveTopology(topology);
            
            commandList->SetGraphicsRoot32BitConstants(0, rootConstantsSize, rootConstants, 0);
            if (isText)
            {
                commandList->SetGraphicsRootDescriptorTable(1, gpuTextHandle);
            }
            commandList->DrawInstanced(count, 1, 0, 0);
        }
    }

    void drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled) override
    {
        ID3D12PipelineState* pso = depthEnabled ? pointPSO.Get() : pointPSONoDepth.Get();

        Matrix mvp = mvpMatrix.Transpose();
        recordCommands(points, count, pointBuffer.Get(), pointBufferView, pso, pointLineSignature.Get(), &mvp, sizeof(Matrix) / sizeof(UINT32), D3D_PRIMITIVE_TOPOLOGY_POINTLIST, pointOffset, false);
    }

    void drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled) override
    {
        ID3D12PipelineState* pso = depthEnabled ? linePSO.Get() : linePSONoDepth.Get();

        Matrix mvp = mvpMatrix.Transpose();
        recordCommands(lines, count, lineBuffer.Get(), lineBufferView, pso, pointLineSignature.Get(), &mvp, sizeof(Matrix) / sizeof(UINT32), D3D_PRIMITIVE_TOPOLOGY_LINELIST, lineOffset, false);
    }

    void drawGlyphList(const dd::DrawVertex * glyphs, int count, dd::GlyphTextureHandle glyphTex) override
    {
        Vector2 dim = Vector2(float(width), float(height));

        if (cpuTextHandle.ptr)
        {
            recordCommands(glyphs, count, textBuffer.Get(), textBufferView, textPSO.Get(), textSignature.Get(), &dim, sizeof(Vector2) / sizeof(UINT32), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, textOffset, true);
        }
    }

    dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void * pixels) override
    {
        // Create and upload texture

        if (cpuTextHandle.ptr != 0)
        {
            D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, UINT64(width), UINT(height), 1, 1, DXGI_FORMAT_R8_UNORM, {1, 0}, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE };

            CD3DX12_HEAP_PROPERTIES defaultProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            device->CreateCommittedResource(&defaultProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&glyphTexture));
            glyphTexture->SetName(L"Debug Draw glyph texture");
            
            UINT64 requiredSize = 0;
            UINT64 rowSize = 0;

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrint;
            device->GetCopyableFootprints(&desc, 0, 1, 0, &footPrint, nullptr, &rowSize, &requiredSize);

            ComPtr<ID3D12Resource> staging;
            CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
            CD3DX12_HEAP_PROPERTIES uploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            device->CreateCommittedResource(&uploadProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&staging));

            BYTE* uploadData = nullptr;
            staging->Map(0, nullptr, reinterpret_cast<void**>(&uploadData));

            for (size_t i = 0; i < height; ++i)
            {
                memcpy(uploadData + i * footPrint.Footprint.RowPitch, reinterpret_cast<const uint8_t*>(pixels) + i * rowSize, rowSize);
            }
            staging->Unmap(0, nullptr);

            CD3DX12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(glyphTexture.Get());
            CD3DX12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(staging.Get(), footPrint);
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(glyphTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, 
                                                                                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            commandList->Reset(commandAllocator.Get(), nullptr);
            commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
            commandList->ResourceBarrier(1, &barrier);
            commandList->Close();
        
            ID3D12CommandList* const gfxCommandList = commandList.Get();
            uploadQueue->ExecuteCommandLists(1, &gfxCommandList);

            ++uploadFenceValue;
            uploadQueue->Signal(uploadFence.Get(), uploadFenceValue);

            uploadFence->SetEventOnCompletion(uploadFenceValue, uploadEvent);
            WaitForSingleObject(uploadEvent, INFINITE);

            // Create descriptors 
            device->CreateShaderResourceView(glyphTexture.Get(), nullptr, cpuTextHandle);
        }

        return dd::GlyphTextureHandle(0xFFFFFF);
    }


private:

    Matrix                  mvpMatrix;
    uint32_t                width = 1;
    uint32_t                height = 1;
    ComPtr<ID3D12Device4>   device;
    ComPtr<ID3DBlob>        linePointVS;
    ComPtr<ID3DBlob>        linePointPS;
    ComPtr<ID3DBlob>        textVS;
    ComPtr<ID3DBlob>        textPS;

    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12CommandAllocator>    commandAllocator;
    ComPtr<ID3D12CommandQueue>        uploadQueue;
    ComPtr<ID3D12CommandAllocator>    uploadCommandAllocator;
    ComPtr<ID3D12Fence1>              uploadFence;
    HANDLE                            uploadEvent = NULL;
    uint32_t                          uploadFenceValue = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE       cpuTextHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE       gpuTextHandle;

private:

    ComPtr<ID3D12Resource>       lineBuffer;
    D3D12_VERTEX_BUFFER_VIEW     lineBufferView;
    void*                        linePtr    = nullptr;
    size_t                       lineOffset = 0;

    ComPtr<ID3D12Resource>       pointBuffer;
    D3D12_VERTEX_BUFFER_VIEW     pointBufferView;
    void*                        pointPtr    = nullptr;
    size_t                       pointOffset = 0;

    ComPtr<ID3D12Resource>       textBuffer;
    D3D12_VERTEX_BUFFER_VIEW     textBufferView;
    void*                        textPtr    = nullptr;
    size_t                       textOffset = 0;

    ComPtr<ID3D12RootSignature>  pointLineSignature;
    ComPtr<ID3D12PipelineState>  pointPSO;
    ComPtr<ID3D12PipelineState>  pointPSONoDepth;
    ComPtr<ID3D12PipelineState>  linePSO;
    ComPtr<ID3D12PipelineState>  linePSONoDepth;

    ComPtr<ID3D12RootSignature>  textSignature;
    ComPtr<ID3D12PipelineState>  textPSO;

    ComPtr<ID3D12Resource>       glyphTexture;

}; // class DDRenderInterfaceCoreD3D12

DDRenderInterfaceCoreD3D12* DebugDrawPass::implementation = 0;

DebugDrawPass::DebugDrawPass(ID3D12Device4* device, ID3D12CommandQueue* uploadQueue, 
                             D3D12_CPU_DESCRIPTOR_HANDLE cpuText, D3D12_GPU_DESCRIPTOR_HANDLE gpuText)
{    
    implementation = new DDRenderInterfaceCoreD3D12(device, uploadQueue, cpuText, gpuText);
    dd::initialize(implementation);
}

DebugDrawPass::~DebugDrawPass()
{
    dd::shutdown();

    delete implementation;
    implementation = 0;
}

void DebugDrawPass::record(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height, const Matrix& view, const Matrix& proj)
{
    BEGIN_EVENT(commandList, "DebugDraw Pass");

    implementation->mvpMatrix     = view * proj;
    implementation->commandList   = commandList;

    implementation->width         = width;
    implementation->height        = height;

    dd::flush();

    END_EVENT(commandList);
}
