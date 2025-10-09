#pragma once

#include "Global/CoreTypes.h"

class FRHICommandList;
class FRHIDevice;

/**
 * @brief 언리얼 엔진의 LineBatcher와 유사한 라인 배치 렌더링 시스템
 * 디버그 라인, 그리드, 바운딩 박스 등을 효율적으로 렌더링
 */
class FLineBatcher
{
public:
	FLineBatcher();
	~FLineBatcher();

	void Initialize(FRHIDevice* InRHIDevice);
	void Shutdown();

	/**
	 * @brief 라인 배치 시작
	 */
	void BeginBatch();

	/**
	 * @brief 단일 라인 추가
	 */
	void AddLine(const FVector& Start, const FVector& End, const FVector4& Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f));

	/**
	 * @brief 여러 라인 추가
	 */
	void AddLines(const TArray<FVector>& StartPoints, const TArray<FVector>& EndPoints, const TArray<FVector4>& Colors);

	/**
	 * @brief 라인 배치 종료 및 CommandList에 렌더 명령 추가
	 */
	void EndBatch(FRHICommandList* CommandList, const FMatrix& ModelMatrix, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix, const FRect* ScissorRect = nullptr);

	/**
	 * @brief 배치된 라인 수 반환
	 */
	int32 GetLineCount() const { return BatchedLineStartPoints.Num(); }

	/**
	 * @brief 배치가 활성화되어 있는지 확인
	 */
	bool IsBatchActive() const { return bIsBatchActive; }

private:
	FRHIDevice* RHIDevice = nullptr;
	bool bIsInitialized = false;
	bool bIsBatchActive = false;

	// 배치된 라인 데이터
	TArray<FVector> BatchedLineStartPoints;
	TArray<FVector> BatchedLineEndPoints;
	TArray<FVector4> BatchedLineColors;
};
