#pragma once
#include "Render/Renderer/Public/PrimitiveSceneProxy.h"

class UBillBoardComponent;

/**
 * @brief FBillBoardSceneProxy: BillBoard를 위한 특화된 씬 프록시
 * FontRenderer를 사용한 텍스트 렌더링을 지원
 */
class FBillBoardSceneProxy : public FPrimitiveSceneProxy
{
public:
	/**
	 * @brief 생성자
	 * @param InComponent BillBoard 컴포넌트
	 */
	explicit FBillBoardSceneProxy(const UBillBoardComponent* InComponent);

	/**
	 * @brief BillBoard는 특수한 렌더링 방식 사용
	 * @return 텍스트가 있으면 true (FontRenderer 사용)
	 */
	bool IsValidForRendering() const;

	/**
	 * @brief RT 매트릭스 가져오기
	 * @return BillBoard의 RT 매트릭스
	 */
	FMatrix GetRTMatrix() const { return RTMatrix; }

	/**
	 * @brief 렌더링할 텍스트 가져오기
	 * @return UUID 문자열
	 */
	const FString& GetDisplayText() const { return DisplayText; }

private:
	/** BillBoard의 변환 매트릭스 */
	FMatrix RTMatrix;

	/** 표시할 텍스트 */
	FString DisplayText;
};
