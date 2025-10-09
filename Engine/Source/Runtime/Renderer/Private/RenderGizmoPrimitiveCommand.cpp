#include "pch.h"
#include "Runtime/Renderer/Public/RenderGizmoPrimitiveCommand.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Core/Public/VertexTypes.h"

void FRHIRenderGizmoPrimitiveCommand::Execute()
{
	// RHIDevice 유효성 검사
	if (!RHIDevice || !RHIDevice->IsInitialized())
	{
		UE_LOG_ERROR("RenderGizmoPrimitiveCommand: RHIDevice가 초기화되지 않았습니다");
		return;
	}

	ID3D11DeviceContext* DeviceContext = RHIDevice->GetDeviceContext();
	if (!DeviceContext)
	{
		UE_LOG_ERROR("RenderGizmoPrimitiveCommand: DeviceContext가 null입니다");
		return;
	}

	// Primitive 유효성 검사
	if (!Primitive.Vertexbuffer || Primitive.NumVertices == 0)
	{
		return; // 유효하지 않은 Primitive는 조용히 스킵
	}

	try
	{
		// 버텍스 버퍼 설정
		UINT stride = sizeof(FVertex);
		UINT offset = 0;
		DeviceContext->IASetVertexBuffers(0, 1, &Primitive.Vertexbuffer, &stride, &offset);

		// 토폴로지 설정
		DeviceContext->IASetPrimitiveTopology(Primitive.Topology);

		// 입력 레이아웃 설정
		if (Primitive.InputLayout)
		{
			DeviceContext->IASetInputLayout(Primitive.InputLayout);
		}

		// 셰이더 설정
		if (Primitive.VertexShader)
		{
			DeviceContext->VSSetShader(Primitive.VertexShader, nullptr, 0);
		}

		if (Primitive.PixelShader)
		{
			DeviceContext->PSSetShader(Primitive.PixelShader, nullptr, 0);
		}

		// 래스터라이저 상태 설정 - 기즈모는 컬링 없이 렌더링
		RHIDevice->RSSetState(EViewMode::Lit);

		// 깊이 스텐실 상태 - 기즈모는 항상 위에 표시
		if (Primitive.bShouldAlwaysVisible)
		{
			// 깊이 테스트 비활성화 또는 항상 통과하도록 설정
			// TODO: RHIDevice에 SetDepthStencilState 메서드 추가 필요
		}

		// 화면 공간 기즈모 스케일 계산 및 변환 행렬 업데이트
		FMatrix ModelMatrix;
		CalculateScreenSpaceTransform(ModelMatrix);

		// TODO: 실제 MVP 행렬 계산 및 상수 버퍼 업데이트
		// RHIDevice->UpdateConstantBuffer(ModelMatrix, ViewMatrix, ProjectionMatrix);

		// 드로우 콜
		if (Primitive.IndexBuffer && Primitive.NumIndices > 0)
		{
			DeviceContext->IASetIndexBuffer(Primitive.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			DeviceContext->DrawIndexed(Primitive.NumIndices, 0, 0);
		}
		else
		{
			DeviceContext->Draw(Primitive.NumVertices, 0);
		}
	}
	catch (...)
	{
		UE_LOG_ERROR("RenderGizmoPrimitiveCommand: 렌더링 중 예외 발생");
	}
}

void FRHIRenderGizmoPrimitiveCommand::CalculateScreenSpaceTransform(FMatrix& OutModelMatrix) const
{
	// 기즈모 위치까지의 거리 계산
	float Distance = (CameraLocation - Primitive.Location).Length();
	Distance = max(Distance, 0.1f); // 최소 거리 보장

	// 화면 공간 기반 스케일 계산
	// 거리에 비례하여 스케일을 조정하여 화면에서 일정한 크기 유지
	float ScreenSpaceScale = Distance * 0.1f;
	ScreenSpaceScale = max(ScreenSpaceScale, 0.01f); // 최소 스케일

	// 모델 행렬 계산: Scale * Rotation * Translation
	FMatrix ScaleMatrix = FMatrix::ScaleMatrix(Primitive.Scale * ScreenSpaceScale);
	FMatrix RotationMatrix = FMatrix::RotationMatrix(Primitive.Rotation);
	FMatrix TranslationMatrix = FMatrix::TranslationMatrix(Primitive.Location);

	OutModelMatrix = ScaleMatrix * RotationMatrix * TranslationMatrix;
}
