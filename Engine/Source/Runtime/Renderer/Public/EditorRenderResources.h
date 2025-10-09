#pragma once

#include "Runtime/Core/Public/Object.h"
#include "Source/Global/Enum.h"
#include <d3d11.h>

class FRHIDevice;
class FEditorGrid;
class FLineBatcher;

/**
 * @brief Editor에 특화된 렌더링 리소스들을 관리하는 클래스
 * Grid, Gizmo, BoundingBox 등의 Editor 전용 렌더링 리소스를 담당
 */
class FEditorRenderResources
{
public:
    FEditorRenderResources();
    ~FEditorRenderResources();

    void Initialize(FRHIDevice* InRHIDevice);
    void Shutdown();

    // Shader 접근자들
    ID3D11VertexShader* GetEditorVertexShader(EShaderType ShaderType) const;
    ID3D11PixelShader* GetEditorPixelShader(EShaderType ShaderType) const;
    ID3D11InputLayout* GetEditorInputLayout(EShaderType ShaderType) const;

    // Editor 전용 버퍼 생성/해제
    ID3D11Buffer* CreateVertexBuffer(const void* Data, uint32 SizeInBytes, bool bCpuAccess = false);
    ID3D11Buffer* CreateIndexBuffer(const void* Data, uint32 SizeInBytes);
    void ReleaseVertexBuffer(ID3D11Buffer* Buffer);
    void ReleaseIndexBuffer(ID3D11Buffer* Buffer);

    // Editor 전용 렌더링
    void RenderPrimitiveIndexed(const struct FEditorPrimitive& Primitive, const struct FRenderState& RenderState,
                               bool bIsBlendMode, uint32 VertexStride, uint32 IndexStride);

    // 버퍼 업데이트
    void UpdateVertexBuffer(ID3D11Buffer* Buffer, const TArray<FVector>& Vertices);

    // LineBatcher 접근자
    FLineBatcher* GetLineBatcher() const { return LineBatcher.Get(); }

    // Gizmo 관련
    ID3D11Buffer* GetGizmoVertexBuffer(EPrimitiveType PrimitiveType) const;
    uint32 GetGizmoVertexCount(EPrimitiveType PrimitiveType) const;

    // EditorGrid 접근자
    FEditorGrid* GetEditorGrid() const { return EditorGrid.Get(); }

private:
    FRHIDevice* RHIDevice = nullptr;
    bool bIsInitialized = false;

    // Grid 관리
    TUniquePtr<FEditorGrid> EditorGrid;
    ID3D11Buffer* GridVertexBuffer = nullptr;
    ID3D11Buffer* GridIndexBuffer = nullptr;

    // Line Batching
    TUniquePtr<FLineBatcher> LineBatcher;

    // Gizmo 리소스
    TMap<EPrimitiveType, ID3D11Buffer*> GizmoVertexBuffers;
    TMap<EPrimitiveType, uint32> GizmoVertexCounts;

    // Editor 전용 셰이더들
    struct FEditorShaders
    {
        ID3D11VertexShader* BatchLineVS = nullptr;
        ID3D11PixelShader* BatchLinePS = nullptr;
        ID3D11InputLayout* BatchLineIL = nullptr;

        ID3D11VertexShader* StaticMeshVS = nullptr;
        ID3D11PixelShader* StaticMeshPS = nullptr;
        ID3D11InputLayout* StaticMeshIL = nullptr;
    } EditorShaders;

    void LoadEditorShaders();
    void ReleaseEditorShaders();

    void InitializeGrid();
    void ReleaseGrid();

    void InitializeGizmoResources();
    void ReleaseGizmoResources();
};
