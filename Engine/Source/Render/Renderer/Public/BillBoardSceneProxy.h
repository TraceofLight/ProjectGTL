#pragma once
#include "Render/Renderer/Public/PrimitiveSceneProxy.h"
#include "Global/Matrix.h"

class UBillBoardComponent;

/**
 * @brief FBillBoardSceneProxy: BillBoard 컴포넌트의 렌더링 데이터를 관리하는 프록시 클래스
 * @note BillBoard는 항상 카메라를 향하는 2D 스프라이트로 텍스트를 표시
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
	 * @brief 소멸자
	 */
	virtual ~FBillBoardSceneProxy() override = default;

	/**
	 * @brief RT 매트릭스를 가져옴 (회전 및 평행이동)
	 * @return RT 매트릭스
	 */
	const FMatrix& GetRTMatrix() const { return RTMatrix; }

	/**
	 * @brief 표시할 텍스트를 가져옴
	 * @return 표시할 텍스트 문자열
	 */
	const FString& GetDisplayText() const { return DisplayText; }

private:
	/** RT 매트릭스 (회전 및 평행이동) */
	FMatrix RTMatrix;

	/** 표시할 텍스트 */
	FString DisplayText;
};
