#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Factory/Public/FactorySystem.h"
#include "Factory/Public/NewObject.h"

class AAxis;
class UGizmo;
class UGrid;
class AActor;
class UPrimitiveComponent;

/**
 * @brief Level Show Flag Enum
 */
enum class EEngineShowFlags : uint64
{
	SF_Primitives = 0x01,
	SF_BillboardText = 0x10,
	SF_Bounds = 0x20,
	SF_Grid = 0x40,
	SF_StaticMeshes = 0x80,
	SF_BoundingBoxes = 0x100,
};

inline uint64 operator|(EEngineShowFlags lhs, EEngineShowFlags rhs)
{
	return static_cast<uint64>(lhs) | static_cast<uint64>(rhs);
}

inline uint64 operator&(uint64 lhs, EEngineShowFlags rhs)
{
	return lhs & static_cast<uint64>(rhs);
}

UCLASS()
class ULevel :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(ULevel, UObject)

public:
	ULevel();
	ULevel(const FName& InName);
	~ULevel() override;

	void Release();

	virtual void Init();
	virtual void Render();
	virtual void Cleanup();

	const TArray<TObjectPtr<AActor>>& GetLevelActors() const { return Actors; }
	TObjectPtr<UWorld> GetWorld() const override { return OwningWorld; }
	void SetOwningWorld(TObjectPtr<UWorld> InWorld) { OwningWorld = InWorld; }

	template <typename T>
	TObjectPtr<T> SpawnActor(const FName& InName = FName::FName_None,
	                         const FTransform& InTransform = {});

	// Actor 삭제
	bool DestroyActor(TObjectPtr<AActor> InActor);

	// Flag Control
	uint64 GetShowFlags() const { return ShowFlags; }
	void SetShowFlags(uint64 InShowFlags) { ShowFlags = InShowFlags; }

private:
	TArray<TObjectPtr<AActor>> Actors;
	TObjectPtr<UWorld> OwningWorld;

	// 빌보드는 처음에 표시 안하는 게 좋다는 의견이 있어 빌보드만 꺼놓고 출력
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
		static_cast<uint64>(EEngineShowFlags::SF_Bounds) |
		static_cast<uint64>(EEngineShowFlags::SF_Grid) |
		static_cast<uint64>(EEngineShowFlags::SF_StaticMeshes) |
		static_cast<uint64>(EEngineShowFlags::SF_BoundingBoxes);
};

// TODO(KHJ): 이거 쓰는지 확인하고 안 쓰면 지울 것
template <typename T>
TObjectPtr<T> ULevel::SpawnActor(const FName& InName, const FTransform& InTransform)
{
	// Factory 시스템 초기화
	static bool bFactorySystemInitialized = false;
	if (!bFactorySystemInitialized)
	{
		FFactorySystem::Initialize();
		bFactorySystemInitialized = true;
	}

	TObjectPtr<T> NewActor = ::SpawnActor<T>(TObjectPtr<ULevel>(this), InTransform, InName);

	if (NewActor)
	{
		Actors.Add(NewActor);
	}

	return NewActor;
}
