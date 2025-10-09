#pragma once
#include "Widget.h"

/**
 * @brief 메인 메뉴바 바로 아래에 표시되는 툴바 위젯
 * 레벨 이름 표시 및 PlaceActor 드롭다운 메뉴 제공
 */
UCLASS()
class UToolbarWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UToolbarWidget, UWidget)

public:
	UToolbarWidget();
	~UToolbarWidget() override = default;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	float GetToolbarHeight() const { return ToolbarHeight; }
	bool IsToolbarVisible() const { return bIsToolbarVisible; }

private:
	bool bIsToolbarVisible = true;
	float ToolbarHeight = 0.0f;

	static void RenderLevelName();
	void RenderPlaceActorMenu();
	static void SpawnActorAtViewportCenter();
	static void SpawnStaticMeshActorAtViewportCenter();
};
