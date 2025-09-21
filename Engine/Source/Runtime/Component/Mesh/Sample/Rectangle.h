#pragma once

static FVertexSimple rectangle_vertices[] =
{
	{-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}, // 0: 좌하 (Red)
	{1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}, // 1: 우하 (Green)
	{1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}, // 2: 우상 (Blue)
	{-1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}, // 3: 좌상 (White)
};

// 인덱스 순서를 시계방향(CW)으로 배치
static uint32 rectangle_indices[] =
{
	0, 2, 1,
	0, 3, 2
};
