#pragma once
#include "Runtime/Actor/Public/Actor.h"
#include "Runtime/Component/Public/CameraComponent.h"

UCLASS()
class ACameraActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ACameraActor, AActor)

public:
	// Camera Speed Constants
	static constexpr float MIN_SPEED = 10.0f;
	static constexpr float MAX_SPEED = 70.0f;
	static constexpr float DEFAULT_SPEED = 20.0f;
	static constexpr float SPEED_ADJUST_STEP = 10.0f;

	ACameraActor();
	~ACameraActor() override;

	void BeginPlay() override;
	void Tick() override;

	// Camera Movement Speed Control
	float GetMoveSpeed() const { return CurrentMoveSpeed; }

	void SetMoveSpeed(float InSpeed)
	{
		CurrentMoveSpeed = clamp(InSpeed, MIN_SPEED, MAX_SPEED);
	}

	void AdjustMoveSpeed(float InDelta) { SetMoveSpeed(CurrentMoveSpeed + InDelta); }

	UCameraComponent* GetCameraComponent() const { return CameraComponent.Get(); }

private:
	TObjectPtr<UCameraComponent> CameraComponent = nullptr;
	float CurrentMoveSpeed = DEFAULT_SPEED;
};
