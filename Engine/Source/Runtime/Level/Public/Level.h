#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Factory/Public/FactorySystem.h"
#include "Factory/Public/NewObject.h"

class AAxis;
class AGizmo;
class AGrid;
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
	virtual void Update();
	virtual void Render();
	virtual void Cleanup();

	const TArray<TObjectPtr<AActor>>& GetLevelActors() const { return LevelActors; }

	template <typename T, typename... Args>
	TObjectPtr<T> SpawnActor(const FName& InName = "");

	// Actor 삭제
	bool DestroyActor(TObjectPtr<AActor> InActor);
	void MarkActorForDeletion(TObjectPtr<AActor> InActor); // 지연 삭제를 위한 마킹

	void SetSelectedActor(AActor* InActor);
	TObjectPtr<AActor> GetSelectedActor() const { return SelectedActor; }
	AGizmo* GetGizmo() const { return Gizmo; }

	uint64 GetShowFlags() const { return ShowFlags; }
	void SetShowFlags(uint64 InShowFlags) { ShowFlags = InShowFlags; }

private:
	TArray<TObjectPtr<AActor>> LevelActors;

	// 지연 삭제를 위한 리스트
	TArray<TObjectPtr<AActor>> ActorsToDelete;

	TObjectPtr<AActor> SelectedActor = nullptr;
	TObjectPtr<AGizmo> Gizmo = nullptr;
	TObjectPtr<AAxis> Axis = nullptr;
	TObjectPtr<AGrid> Grid = nullptr;

	// 빌보드는 처음에 표시 안하는 게 좋다는 의견이 있어 빌보드만 꺼놓고 출력
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
		static_cast<uint64>(EEngineShowFlags::SF_Bounds);

	// 지연 삭제 처리 함수
	void ProcessPendingDeletions();
};

template <typename T, typename... Args>
TObjectPtr<T> ULevel::SpawnActor(const FName& InName)
{
	// Factory 시스템 초기화
	static bool bFactorySystemInitialized = false;
	if (!bFactorySystemInitialized)
	{
		FFactorySystem::Initialize();
		bFactorySystemInitialized = true;
	}

	// NewObject.h에 정의된 전역 SpawnActor 함수 호출
	TObjectPtr<T> RawActor = ::SpawnActor<T>(TObjectPtr<ULevel>(this), FTransform(), InName);
	TObjectPtr<T> NewActor = TObjectPtr<T>(RawActor);

	if (NewActor)
	{
		LevelActors.push_back(NewActor);
		NewActor->BeginPlay();
	}

	return NewActor;
}
