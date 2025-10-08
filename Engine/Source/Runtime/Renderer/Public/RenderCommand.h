#pragma once

enum class ERHICommandType : uint8
{
    AddPrimitive, // Scene에 Primitive 추가
    RemovePrimitive, // Scene에서 Primitive 정보 제거

    CreateTexture, // GPU에서 사용할 새로운 Texture Resource 생성 명령
    ReleaseResource, // 더 이상 사용하지 않는 GPU Resource 해제

    SetRenderTarget, // 렌더링 결과를 출력할 대상 설정
    SetBoundShaderState, // 사용할 Shade, Vertex Declaration 등, 렌더링 파이프라인 상태 설정

    DrawIndexedPrimitives, // Primitive를 그리는 Draw Call (점, 선, 삼각형)
    DispatchComputeShader, // Compute Shader를 실행하여 병렬 계산 작업을 시작하는 명령

    // Present 및 BackBuffer 관련
    Present, // SwapChain Present 명령
    GetBackBufferSurface, // BackBuffer Surface 접근
    GetBackBufferRTV, // BackBuffer RenderTargetView 접근

    SubmitCommandList, // GPU 처리 시작 명령
    WaitOnFence, // Batch 처리 등을 위한 Blocking
    
    Num
};

// Unreal Engine 스타일 Sorting Key 비트 마스크 정의
namespace SortingKeyBits
{
    // 64-bit Sorting Key 구성 (MSB -> LSB)
    static constexpr uint64 DepthShift = 32;        // [63:32] Depth 정보 (32 bits)
    static constexpr uint64 MaterialShift = 16;     // [31:16] Material ID (16 bits)
    static constexpr uint64 MeshShift = 8;          // [15:8]  Mesh ID (8 bits)
    static constexpr uint64 PriorityShift = 0;      // [7:0]   Priority (8 bits)
    
    static constexpr uint64 DepthMask = 0xFFFFFFFFULL << DepthShift;
    static constexpr uint64 MaterialMask = 0xFFFFULL << MaterialShift;
    static constexpr uint64 MeshMask = 0xFFULL << MeshShift;
    static constexpr uint64 PriorityMask = 0xFFULL << PriorityShift;
};

// Sorting Key 생성 헬퍼 함수
inline uint64 CreateSortingKey(uint32 DepthInt, uint16 MaterialID, uint8 MeshID, uint8 Priority)
{
    return (static_cast<uint64>(DepthInt) << SortingKeyBits::DepthShift) |
           (static_cast<uint64>(MaterialID) << SortingKeyBits::MaterialShift) |
           (static_cast<uint64>(MeshID) << SortingKeyBits::MeshShift) |
           (static_cast<uint64>(Priority) << SortingKeyBits::PriorityShift);
}

/**
 * @brief Renderer한테 보내야 하는 정보를 담는 클래스
 * Unreal Engine 스타일 Sorting Key 지원
 */
class IRHICommand
{
protected:
    uint64 SortingKey = 0; // Unreal-style 64-bit Sorting Key
    
public:
    IRHICommand() = default;
    virtual ~IRHICommand() = default;

    virtual void Execute() = 0;
    virtual ERHICommandType GetCommandType() const = 0;
    
    // Sorting Key 접근자
    uint64 GetSortingKey() const { return SortingKey; }
    void SetSortingKey(uint64 InSortingKey) { SortingKey = InSortingKey; }
    
    // Sorting Key 컴포넌트 추출 헬퍼
    uint32 GetDepthInt() const { return static_cast<uint32>((SortingKey & SortingKeyBits::DepthMask) >> SortingKeyBits::DepthShift); }
    uint16 GetMaterialID() const { return static_cast<uint16>((SortingKey & SortingKeyBits::MaterialMask) >> SortingKeyBits::MaterialShift); }
    uint8 GetMeshID() const { return static_cast<uint8>((SortingKey & SortingKeyBits::MeshMask) >> SortingKeyBits::MeshShift); }
    uint8 GetPriority() const { return static_cast<uint8>((SortingKey & SortingKeyBits::PriorityMask) >> SortingKeyBits::PriorityShift); }

    // XXX(KHJ): FName_None 처리하고 싶은데 없고 귀찮으니 어차피 직접 호출할 일 없어야 하니까 적당한 값으로 처리
    virtual FName GetName() const { return FName{""}; }
};
