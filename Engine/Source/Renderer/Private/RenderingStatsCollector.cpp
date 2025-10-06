#include "pch.h"
#include "Renderer/Public/RenderingStatsCollector.h"

IMPLEMENT_SINGLETON_CLASS(URenderingStatsCollector, UObject)

URenderingStatsCollector::URenderingStatsCollector()
{
}

URenderingStatsCollector::~URenderingStatsCollector()
{
}

void URenderingStatsCollector::ResetFrameStats()
{
    BasePassDrawCalls = 0;
    DebugPassDrawCalls = 0;
    ShadowPassDrawCalls = 0;
    TranslucentPassDrawCalls = 0;
    TrianglesDrawn = 0;
    VerticesDrawn = 0;
}

void URenderingStatsCollector::ResetAllStats()
{
    ResetFrameStats();
    TotalFrames = 0;
    TotalDrawCallsAllTime = 0;
    TotalTrianglesAllTime = 0;
}

void URenderingStatsCollector::PrintFrameStats() const
{
    UE_LOG("=== Rendering Frame Stats ===");
    UE_LOG("Base Pass Draw Calls: %u", BasePassDrawCalls);
    UE_LOG("Debug Pass Draw Calls: %u", DebugPassDrawCalls);
    UE_LOG("Shadow Pass Draw Calls: %u", ShadowPassDrawCalls);
    UE_LOG("Translucent Pass Draw Calls: %u", TranslucentPassDrawCalls);
    UE_LOG("Total Draw Calls: %u", GetTotalDrawCalls());
    UE_LOG("Triangles Drawn: %u", TrianglesDrawn);
    UE_LOG("Vertices Drawn: %u", VerticesDrawn);
    UE_LOG("=============================");
}