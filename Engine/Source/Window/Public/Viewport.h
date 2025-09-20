#pragma once
#include "pch.h"

class FViewportClient;

class FViewport
{
public:
	FViewport(FViewportClient* InViewportClient);






private:
	/** The viewport's client. */
	FViewportClient* ViewportClient;

	/** The width of the viewport. */
	uint32 SizeX;

	/** The height of the viewport. */
	uint32 SizeY;

	/** The initial position of the viewport. */
	uint32 InitialPositionX;

	/** The initial position of the viewport. */
	uint32 InitialPositionY;

	// ViewportType으로 뷰포트의 종류를 나타내는 태그를 설정할 것임 (ex. 에디터 레벨뷰, 미디어뷰, 플레이어 뷰)
	/** Used internally for testing runtime instance type before casting */
	//FName ViewportType;
};
