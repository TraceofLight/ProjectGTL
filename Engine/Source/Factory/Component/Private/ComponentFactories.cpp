#include "pch.h"
#include "Factory/Component/Public/ComponentFactory.h"

#include "Runtime/Component/Mesh/Public/CubeComponent.h"
#include "Runtime/Component/Mesh/Public/SphereComponent.h"
#include "Runtime/Component/Mesh/Public/TriangleComponent.h"
#include "Runtime/Component/Mesh/Public/SquareComponent.h"
#include "Runtime/Component/Public/LineComponent.h"
#include "Runtime/Component/Public/SceneComponent.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"
#include "Runtime/Component/Public/StaticMeshComponent.h"

// 정적 인스턴스
static TObjectPtr<TComponentFactory<UCubeComponent>> GCubeComponentFactory;
static TObjectPtr<TComponentFactory<USphereComponent>> GSphereComponentFactory;
static TObjectPtr<TComponentFactory<UTriangleComponent>> GTriangleComponentFactory;
static TObjectPtr<TComponentFactory<USquareComponent>> GSquareComponentFactory;
static TObjectPtr<TComponentFactory<ULineComponent>> GLineComponentFactory;
static TObjectPtr<TComponentFactory<USceneComponent>> GSceneComponentFactory;
static TObjectPtr<TComponentFactory<UPrimitiveComponent>> GPrimitiveComponentFactory;
static TObjectPtr<TComponentFactory<UStaticMeshComponent>> GStaticMeshComponentFactory;

// 생성자에서 정적 인스턴스에 대한 Register 함수 호출
struct FComponentFactoryRegistrar
{
	FComponentFactoryRegistrar()
	{
		UFactory::RegisterFactory(GCubeComponentFactory);
		UFactory::RegisterFactory(GSphereComponentFactory);
		UFactory::RegisterFactory(GTriangleComponentFactory);
		UFactory::RegisterFactory(GSquareComponentFactory);
		UFactory::RegisterFactory(GLineComponentFactory);
		UFactory::RegisterFactory(GSceneComponentFactory);
		UFactory::RegisterFactory(GPrimitiveComponentFactory);
		UFactory::RegisterFactory(GStaticMeshComponentFactory);

		UE_LOG_SUCCESS("ComponentFactories: 모든 Component Factory 클래스 등록이 완료되었습니다");
	}
};

// 정적 인스턴스로 자동 등록
static FComponentFactoryRegistrar GComponentFactoryRegistrar;
