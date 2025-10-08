#include "pch.h"
#include "Shader/Public/Shader.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Core/Public/VertexTypes.h"

IMPLEMENT_CLASS(UShader, UObject)

UShader::UShader() = default;
UShader::~UShader()
{
    Release();
}

bool UShader::Initialize(const FString& InFilePath, EVertexLayoutType InLayoutType)
{
    if (!GDynamicRHI)
    {
        return false;
    }

    FilePath = InFilePath;
    LayoutType = InLayoutType;

    // Vertex Shader와 Input Layout 생성
    TArray<D3D11_INPUT_ELEMENT_DESC> LayoutDesc;
    if (LayoutType == EVertexLayoutType::PositionColorTextureNormal)
    {
        LayoutDesc = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
    }
    else // PositionColor 등 다른 레이아웃에 대한 처리 추가 가능
    {
        LayoutDesc = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
    }

    std::wstring WidePath(InFilePath.begin(), InFilePath.end());
    if (!GDynamicRHI->CreateVertexShaderAndInputLayout(WidePath, LayoutDesc, &VertexShader, &InputLayout))
    {
        UE_LOG_ERROR("Failed to create vertex shader and input layout for %s", InFilePath.c_str());
        return false;
    }

    // Pixel Shader 생성
    if (!GDynamicRHI->CreatePixelShader(WidePath, &PixelShader))
    {
        UE_LOG_ERROR("Failed to create pixel shader for %s", InFilePath.c_str());
        Release(); // 앞에서 생성된 VS 리소스 해제
        return false;
    }

    bIsValid = true;
    UE_LOG_SUCCESS("Shader initialized successfully: %s", InFilePath.c_str());
    return true;
}

void UShader::Release()
{
    if (GDynamicRHI)
    {
        GDynamicRHI->ReleaseShader(VertexShader);
        GDynamicRHI->ReleaseShader(PixelShader);
        GDynamicRHI->ReleaseShader(InputLayout);
    }
    VertexShader = nullptr;
    PixelShader = nullptr;
    InputLayout = nullptr;
    bIsValid = false;
}

void UShader::Bind()
{
    if (!bIsValid || !GDynamicRHI) return;
    ID3D11DeviceContext* Context = GDynamicRHI->GetDeviceContext();
    if (!Context) return;

    Context->VSSetShader(VertexShader, nullptr, 0);
    Context->PSSetShader(PixelShader, nullptr, 0);
    Context->IASetInputLayout(InputLayout);
}

void UShader::Unbind()
{
    if (!bIsValid || !GDynamicRHI) return;
    ID3D11DeviceContext* Context = GDynamicRHI->GetDeviceContext();
    if (!Context) return;

    Context->VSSetShader(nullptr, nullptr, 0);
    Context->PSSetShader(nullptr, nullptr, 0);
    Context->IASetInputLayout(nullptr);
}
