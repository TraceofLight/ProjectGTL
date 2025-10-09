#pragma once

class UWorld;

/**
 * @brief 월드의 종류를 구분하는 enum
 */
UENUM()
enum class EWorldType : uint8
{
	None,		// 초기화되지 않은 상태
	Editor,		// 에디터 월드 (편집 모드)
	PIE,		// Play-In-Editor 월드
	Game		// 독립 실행 게임 월드
};
DECLARE_UINT8_ENUM_REFLECTION(EWorldType)

/**
 * @brief 월드 컨텍스트 정보를 담는 구조체
 * UWorld와 해당 월드의 메타 정보를 함께 관리
 */
struct FWorldContext
{
public:
	FWorldContext() = default;

	explicit FWorldContext(TObjectPtr<UWorld> InWorld)
		: ThisCurrentWorld(InWorld) {}

	// 월드 Getter & Setter
	TObjectPtr<UWorld> World() const { return ThisCurrentWorld; }
	void SetWorld(TObjectPtr<UWorld> InWorld) { ThisCurrentWorld = InWorld; }

	// 월드 타입 편의 함수
	bool IsEditorWorld() const;
	bool IsPIEWorld() const;
	bool IsGameWorld() const;

private:
	TObjectPtr<UWorld> ThisCurrentWorld = nullptr;
};
