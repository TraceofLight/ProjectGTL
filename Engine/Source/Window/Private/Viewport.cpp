#include "Source/Window/Public/Viewport.h"


FViewport::FViewport(FViewportClient* InViewportClient)
	: ViewportClient(InViewportClient)
	, InitialPositionX(0)                 // Window origin X
	, InitialPositionY(0)                 // Window origin Y
	, SizeX(0)                            // Initial width (finalized on init)
	, SizeY(0)
{
}
