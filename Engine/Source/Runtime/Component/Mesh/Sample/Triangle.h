#pragma once

// 이등변삼각형 정점 데이터 (정규화: base [-0.5,0.5], 높이 1.0)
static FVertexSimple triangle_vertices[] =
{
	{-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}, // Left base
	{0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}, // Apex
	{0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}, // Right base
};
