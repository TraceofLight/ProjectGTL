#pragma once
#include "Runtime/Core/Public/Object.h"

/**
 * @brief 렌더링 통계를 수집하는 싱글톤 클래스
 * 언리얼 엔진의 렌더링 통계 수집 시스템과 유사
 */
UCLASS()
class URenderingStatsCollector : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(URenderingStatsCollector, UObject)

public:
	// Draw Call 통계
	void IncrementBasePassDrawCalls() { BasePassDrawCalls++; }
	void IncrementDebugPassDrawCalls() { DebugPassDrawCalls++; }
	void IncrementShadowPassDrawCalls() { ShadowPassDrawCalls++; }
	void IncrementTranslucentPassDrawCalls() { TranslucentPassDrawCalls++; }

	// Primitive 통계
	void IncrementTrianglesDrawn(uint32 Count) { TrianglesDrawn += Count; }
	void IncrementVerticesDrawn(uint32 Count) { VerticesDrawn += Count; }

	// 통계 조회
	uint32 GetBasePassDrawCalls() const { return BasePassDrawCalls; }
	uint32 GetDebugPassDrawCalls() const { return DebugPassDrawCalls; }
	uint32 GetShadowPassDrawCalls() const { return ShadowPassDrawCalls; }
	uint32 GetTranslucentPassDrawCalls() const { return TranslucentPassDrawCalls; }
	uint32 GetTotalDrawCalls() const { return BasePassDrawCalls + DebugPassDrawCalls + ShadowPassDrawCalls + TranslucentPassDrawCalls; }

	uint32 GetTrianglesDrawn() const { return TrianglesDrawn; }
	uint32 GetVerticesDrawn() const { return VerticesDrawn; }

	// 통계 리셋
	void ResetFrameStats();
	void ResetAllStats();

	// 디버깅용 출력
	void PrintFrameStats() const;

private:
	// Draw Call 카운터들
	uint32 BasePassDrawCalls = 0;
	uint32 DebugPassDrawCalls = 0;
	uint32 ShadowPassDrawCalls = 0;
	uint32 TranslucentPassDrawCalls = 0;

	// Primitive 카운터들
	uint32 TrianglesDrawn = 0;
	uint32 VerticesDrawn = 0;

	// 누적 통계 (프레임 간 유지)
	uint64 TotalFrames = 0;
	uint64 TotalDrawCallsAllTime = 0;
	uint64 TotalTrianglesAllTime = 0;
};