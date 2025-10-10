#include "pch.h"
#include "Runtime/Renderer/Public/DrawIndexedPrimitivesCommand.h"

#include "Runtime/Core/Public/VertexTypes.h"

#include "Material/Public/Material.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"
#include "Runtime/Component/Public/StaticMeshComponent.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"
#include "Shader/Public/Shader.h"
#include "Texture/Public/Texture.h"
#include "Runtime/Renderer/Public/MaterialRenderProxy.h"
#include "Runtime/Renderer/Public/TextureRenderProxy.h"

class UAssetSubsystem;
class UShader;

/**
 * @brief Primitive Render 명령을 실행
 */
void FRHIDrawIndexedPrimitivesCommand::Execute()
{
	if (!RHIDevice || !Component)
	{
		return;
	}

	SetupShaderForComponent(Component);

	// RenderCommand 패턴: 실제 렌더링 로직을 여기서 직접 처리
	RenderComponent(Component);
}

void FRHIDrawIndexedPrimitivesCommand::SetupShaderForComponent(UPrimitiveComponent* InComponent)
{
	if (!InComponent || !RHIDevice)
	{
		UE_LOG_ERROR("DrawIndexedPrimitives: SetupShaderForComponent - Component(%p) or RHIDevice(%p) is null!", InComponent, RHIDevice);
		return;
	}

	// 셰이더 로드
	UAssetSubsystem* AssetSubsystem = GEngine->GetEngineSubsystem<UAssetSubsystem>();
	TObjectPtr<UShader> Shader = nullptr;
	if (AssetSubsystem)
	{
		// TODO: 머티리얼에서 셰이더 경로를 가져오는 로직 필요
		TArray<D3D11_INPUT_ELEMENT_DESC> LayoutDesc = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		Shader = AssetSubsystem->LoadShader("StaticMeshShader.hlsl", LayoutDesc);
	}

	// RHI에 셰이더 설정
	RHIDevice->SetShader(Shader);

	// 텍스처 및 샘플러 바인딩
	UMaterial* ComponentMaterial = InComponent->GetMaterial();
	ID3D11ShaderResourceView* TextureSRV = nullptr;

	if (ComponentMaterial)
	{
		// Material에서 RenderProxy 가져오기
		FMaterialRenderProxy* MaterialProxy = ComponentMaterial->GetRenderProxy();

		if (MaterialProxy && MaterialProxy->IsValid())
		{
			// RenderProxy를 통해 렌더 스레드에서 텍스처 리소스 요청
			TextureSRV = MaterialProxy->GetTextureForRendering_RenderThread(RHIDevice);
		}
		else
		{
			UE_LOG("SetupShader: MaterialProxy is null or invalid");
		}
	}
	else
	{
		UE_LOG("SetupShader: No material found on component, using default material");
		// Material이 없어도 렌더링은 계속 - 기본 Material 사용
	}

	// 4. 텍스처 SRV 설정
	if (TextureSRV)
	{
		ID3D11DeviceContext* DeviceContext = RHIDevice->GetDeviceContext();
		DeviceContext->PSSetShaderResources(0, 1, &TextureSRV);
		RHIDevice->PSSetDefaultSampler(0);
	}
	else
	{
		UE_LOG("SetupShader: No texture SRV available, using default");
		// 텍스처가 없어도 렌더링은 계속
	}
}

void FRHIDrawIndexedPrimitivesCommand::RenderComponent(UPrimitiveComponent* InComponent)
{
	if (!InComponent || !RHIDevice)
	{
		return;
	}

	// 상수 버퍼 업데이트 (Model, View, Projection) - 모든 메시에 실제 World Matrix 사용
	RHIDevice->UpdateConstantBuffers(InComponent->GetWorldMatrix(), ViewMatrix, ProjMatrix);


	// 컴포넌트 타입에 따른 렌더링 처리
	if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(InComponent))
	{
		// 기즈모인 경우 특별 처리
		if (bUseOverrideColor)
		{
			RenderGizmoComponent(StaticMeshComp);
		}
		else
		{
			RenderStaticMeshComponent(StaticMeshComp);
		}
	}
}

void FRHIDrawIndexedPrimitivesCommand::RenderStaticMeshComponent(UStaticMeshComponent* StaticMeshComp)
{
	if (!StaticMeshComp || !RHIDevice)
	{
		UE_LOG_WARNING("RenderStaticMeshComponent - Component or RHIDevice is null");
		return;
	}

	UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh();
	if (!StaticMesh)
	{
		UE_LOG_WARNING("RenderStaticMeshComponent - StaticMesh is null for component");
		return;
	}

	ID3D11DeviceContext* DeviceContext = RHIDevice->GetDeviceContext();

	// 정점 및 인덱스 버퍼 설정
	ID3D11Buffer* VertexBuffer = StaticMesh->GetVertexBuffer();
	ID3D11Buffer* IndexBuffer = StaticMesh->GetIndexBuffer();

	if (!VertexBuffer || !IndexBuffer)
	{
		UE_LOG_ERROR("RenderStaticMeshComponent - VertexBuffer(%p) or IndexBuffer(%p) is null for mesh: %s",
		             VertexBuffer, IndexBuffer, StaticMesh->GetAssetPathFileName().c_str());
		return;
	}

	// 버퍼 바인딩
	UINT stride = 0;
	switch (StaticMesh->GetVertexType())
	{
	case EVertexLayoutType::PositionColor:
		stride = sizeof(FVertexSimple);
		break;
	case EVertexLayoutType::PositionColorTextureNormal:
		stride = sizeof(FVertexDynamic);
		break;
	default:
		stride = sizeof(FVertexDynamic);
		break;
	}

	UINT offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
	DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// 기존 Renderer에서 설정했던 Render State들 복구
	// Rasterizer State는 ViewMode에 따라 설정 (여기서는 기본값)
	RHIDevice->RSSetState(EViewMode::Lit);

	// Blend State 설정 (기본: 비활성화)
	RHIDevice->OMSetBlendState(false);

	// 언리얼 엔진 스타일: StaticMesh Section별 렌더링
	const TArray<FStaticMeshSection>& MeshSections = StaticMesh->GetMeshGroupInfo();
	const TArray<UMaterialInterface*>& MaterialSlots = StaticMeshComp->GetMaterailSlots();

	if (MeshSections.Num() > 0)
	{
		// 언리얼 스타일: Section 단위로 렌더링
		for (int32 SectionIndex = 0; SectionIndex < MeshSections.Num(); ++SectionIndex)
		{
			const FStaticMeshSection& Section = MeshSections[SectionIndex];

			// Material Slot 인덱스로 MaterialInterface 가져오기
			UMaterialInterface* MaterialInterface = nullptr;
			if (Section.MaterialSlotIndex >= 0 && Section.MaterialSlotIndex < MaterialSlots.Num())
			{
				MaterialInterface = MaterialSlots[Section.MaterialSlotIndex];
			}

			// AssetSubsystem을 통해 Material 데이터 가져오기
			UAssetSubsystem* AssetSubsystem = GEngine->GetEngineSubsystem<UAssetSubsystem>();

			// 기본 Material 사용
			if (!MaterialInterface && AssetSubsystem)
			{
				MaterialInterface = AssetSubsystem->GetDefaultMaterial();
			}

			// Material 정보 추출 및 설정
			FObjMaterialInfo MaterialInfo;
			bool bHasTexture = false;

			if (MaterialInterface)
			{
				// MaterialInterface에서 MaterialInfo 추출
				if (UMaterial* Material = Cast<UMaterial>(MaterialInterface))
				{
					MaterialInfo = Material->GetMaterialInfo();
					bHasTexture = Material->HasTexture();
				}
				else
				{
					// UMaterialInstance나 다른 MaterialInterface 타입일 경우
					bHasTexture = MaterialInterface->HasTexture();
				}

				// Texture 바인딩 - Material에서 가져오기
				UTexture* Texture = MaterialInterface->GetTexture();
				ID3D11ShaderResourceView* SRV = nullptr;

				if (Texture)
				{
					FTextureRenderProxy* TextureProxy = Texture->GetRenderProxy();
					if (TextureProxy)
					{
						SRV = TextureProxy->GetTextureForRendering_RenderThread(RHIDevice);
					}
				}

				// 텍스처가 없으면 DefaultTexture 로드
				if (!SRV && AssetSubsystem)
				{
					TObjectPtr<UTexture> DefaultTexture = AssetSubsystem->LoadTexture("DefaultTexture.png");
					if (DefaultTexture)
					{
						FTextureRenderProxy* DefaultProxy = DefaultTexture->GetRenderProxy();
						if (DefaultProxy)
						{
							SRV = DefaultProxy->GetTextureForRendering_RenderThread(RHIDevice);
						}
					}
				}

				// SRV 바인딩
				if (SRV)
				{
					DeviceContext->PSSetShaderResources(0, 1, &SRV);
					bHasTexture = true;
				}
			}

			// Material 정보를 Pixel Shader에 전달
			RHIDevice->UpdatePixelConstantBuffers(MaterialInfo, MaterialInterface != nullptr, bHasTexture);

			// Draw Call 실행 (Command에서 처리되어야 함)
			DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			DeviceContext->DrawIndexed(Section.GetIndexCount(), Section.GetStartIndex(), 0);
		}
	}
	else
	{
		// Material이 없는 경우 흰색 기본 Material 사용
		FObjMaterialInfo ObjMaterialInfo;
		ObjMaterialInfo.DiffuseColor = FVector(1.0f, 1.0f, 1.0f); // 흰색 설정

		// DefaultTexture 로드 및 바인딩
		bool bHasTexture = false;
		UAssetSubsystem* AssetSubsystem = GEngine->GetEngineSubsystem<UAssetSubsystem>();
		if (AssetSubsystem)
		{
			TObjectPtr<UTexture> DefaultTexture = AssetSubsystem->LoadTexture("DefaultTexture.png");
			if (DefaultTexture)
			{
				FTextureRenderProxy* DefaultProxy = DefaultTexture->GetRenderProxy();
				if (DefaultProxy)
				{
					ID3D11ShaderResourceView* SRV = DefaultProxy->GetTextureForRendering_RenderThread(RHIDevice);
					if (SRV)
					{
						DeviceContext->PSSetShaderResources(0, 1, &SRV);
						bHasTexture = true;
					}
				}
			}
		}

		RHIDevice->UpdatePixelConstantBuffers(ObjMaterialInfo, true, bHasTexture);

		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		DeviceContext->DrawIndexed(StaticMesh->GetIndexCount(), 0, 0);
	}
}

void FRHIDrawIndexedPrimitivesCommand::RenderGizmoComponent(UStaticMeshComponent* StaticMeshComp)
{
	if (!StaticMeshComp || !RHIDevice) return;

	UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh();
	if (!StaticMesh)
	{
		return;
	}

	ID3D11DeviceContext* DeviceContext = RHIDevice->GetDeviceContext();

	// 기즈모 피킹을 위해 정상 Depth Test 사용
	RHIDevice->OmSetDepthStencilState(EComparisonFunc::LessEqual);

	// 알파 블렌딩 활성화 (기즈모 반투명 효과)
	RHIDevice->OMSetBlendState(true);

	// 정점 및 인덱스 버퍼 설정
	ID3D11Buffer* VertexBuffer = StaticMesh->GetVertexBuffer();
	ID3D11Buffer* IndexBuffer = StaticMesh->GetIndexBuffer();

	if (!VertexBuffer || !IndexBuffer)
	{
		return;
	}

	// 버퍼 바인딩
	UINT stride = 0;
	switch (StaticMesh->GetVertexType())
	{
	case EVertexLayoutType::PositionColor:
		stride = sizeof(FVertexSimple);
		break;
	case EVertexLayoutType::PositionColorTextureNormal:
		stride = sizeof(FVertexDynamic);
		break;
	default:
		stride = sizeof(FVertexDynamic);
		break;
	}

	UINT offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
	DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// 기즈모는 Unlit 모드로 렌더링
	RHIDevice->RSSetState(EViewMode::Unlit);

	// 기즈모 색상을 HighLightBuffer를 통해 설정 (Primitive.hlsl의 Gizmo 로직 사용)
	uint32 gizmoAxis = 0;
	if (bUseOverrideColor)
	{
		// 오버라이드 색상에 따라 축 결정 (빨강=X=1, 초록=Y=2, 파란=Z=3)
		if (OverrideColor.X > 0.5f && OverrideColor.Y < 0.5f && OverrideColor.Z < 0.5f)
			gizmoAxis = 1; // Red = X axis
		else if (OverrideColor.X < 0.5f && OverrideColor.Y > 0.5f && OverrideColor.Z < 0.5f)
			gizmoAxis = 2; // Green = Y axis
		else if (OverrideColor.X < 0.5f && OverrideColor.Y < 0.5f && OverrideColor.Z > 0.5f)
			gizmoAxis = 3; // Blue = Z axis
		else
			gizmoAxis = 1; // 기본값은 X축
	}

	// HighLightBuffer를 통해 기즈모 색상 설정 - OverrideColor 사용
	RHIDevice->UpdateHighLightConstantBuffers(
		0, // Picked = 0 (하이라이트 안함)
		OverrideColor, // Color = 오버라이드 색상 사용 (빨강/초록/파란)
		gizmoAxis, // X = 축 인덱스 (1=빨강, 2=초록, 3=파란)
		bIsHovering ? 1 : 0, // Y = 호버링 상태에 따라 노란색 활성화
		0, // Z = 0
		1 // Gizmo = 1 (기즈모 모드 활성화)
	);

	// 기즈모는 Material 없이 단순 렌더링
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DeviceContext->DrawIndexed(StaticMesh->GetIndexCount(), 0, 0);

	// 기즈모 렌더링 후 상태 정리 (다음 객체에 영향을 주지 않도록)
	// 참고: 실제 상태 복원은 각 RenderComponent 호출 시작에서 수행됨
}

// =============================================================================
// Sorting Key 초기화 구현
// =============================================================================
void FRHIDrawIndexedPrimitivesCommand::InitializeSortingKey(UPrimitiveComponent* InComponent,
                                                            const FMatrix& InViewMatrix)
{
	if (!InComponent)
	{
		SetSortingKey(0);
		return;
	}

	// Material 정보 추출
	MaterialPtr = InComponent->GetMaterial();
	uint16 materialID = 0;
	if (MaterialPtr)
	{
		// Material 포인터를 16-bit ID로 변환 (상위 비트 사용)
		uintptr_t materialAddr = reinterpret_cast<uintptr_t>(MaterialPtr);
		materialID = static_cast<uint16>((materialAddr >> 4) & 0xFFFF); // 연결 4비트 제거, 16비트 취치
	}

	// Mesh ID 추출 (StaticMesh 기반)
	uint8 meshID = 0;
	if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(InComponent))
	{
		if (UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh())
		{
			uintptr_t meshAddr = reinterpret_cast<uintptr_t>(StaticMesh);
			meshID = static_cast<uint8>((meshAddr >> 4) & 0xFF); // 8비트 Mesh ID
		}
	}

	// Depth 계산 단순화 (임시로 좌표계 사용)
	float depth = 100.0f; // 기본 depth 값

	// Depth를 32-bit 정수로 변환 (Unreal 방식)
	// 거리에 비례해서 정렬되도록 IEEE 754 비트 패턴 활용
	uint32 depthInt;
	if (depth >= 0.0f)
	{
		// 양수: IEEE 754 비트 패턴 그대로 사용
		depthInt = *(reinterpret_cast<uint32*>(&depth));
	}
	else
	{
		// 음수: 비트 변환으로 정렬 순서 유지
		uint32 temp = *(reinterpret_cast<uint32*>(&depth));
		depthInt = 0x80000000U ^ temp; // Sign bit flip + invert
	}

	// Priority 설정 (기즈모, 반투명 계살)
	uint8 priority = 128; // 기본 우선순위 (128 = 중간)

	if (bUseOverrideColor)
	{
		// 기즈모는 높은 우선순위 (앞에 렌더링)
		priority = 255; // 최고 우선순위
	}
	// 반투명 Material 체크는 나중에 구현 (임시로 비활성화)
	/*
	else if (MaterialPtr && MaterialPtr->IsTransparent())
	{
	    // 반투명 Material은 낮은 우선순위 (뒤에서 앞으로)
	    priority = 64; // 낮은 우선순위
	}
	*/

	// 64-bit Sorting Key 생성
	uint64 sortingKey = CreateSortingKey(depthInt, materialID, meshID, priority);
	SetSortingKey(sortingKey);

	// 디버그 정보 출력 (릴리즈에서는 제거)
#ifdef _DEBUG
	// UE_LOG("바그: [Sorting] Material: %p, Mesh: %p, Depth: %.2f, Priority: %d, Key: 0x%016llX\n",
	//        MaterialPtr, meshID, depth, priority, sortingKey);
#endif
}
