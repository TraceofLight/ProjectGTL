#pragma once

#define NOMINMAX

// Window Library
#include <windows.h>
#include <wrl.h>

// D3D Library
#include <d3d11.h>
#include <d3dcompiler.h>

// Standard Library
#include <cmath>
#include <cassert>
#include <string>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <functional>
#include <filesystem>
#include <iterator>
#include <sstream>

// Global Included
#include "Source/Global/Types.h"
#include "Source/Global/Memory.h"
#include "Source/Global/Constant.h"
#include "Source/Global/Enum.h"
#include "Source/Global/Matrix.h"
#include "Source/Global/Vector.h"
#include "Source/Global/CoreTypes.h"
#include "Source/Global/Macro.h"
#include "Source/Global/Function.h"

using std::clamp;
using std::unordered_map;
using std::to_string;
using std::function;
using std::wstring;
using std::cout;
using std::cerr;
using std::min;
using std::max;
using std::exception;
using std::stoul;
using std::ofstream;
using std::ifstream;
using std::setw;
using std::sort;
using std::shared_ptr;
using std::unique_ptr;
using std::streamsize;
using Microsoft::WRL::ComPtr;

// File System
namespace filesystem = std::filesystem;
using filesystem::path;
using filesystem::exists;
using filesystem::create_directories;

#define IMGUI_DEFINE_MATH_OPERATORS
#include "Source/Render/UI/Window/Public/ConsoleWindow.h"

#include "Engine/Public/Engine.h"
#include "Engine/Public/EngineEditor.h"
#include "Engine/Public/GameInstance.h"
#include "Engine/Public/LocalPlayer.h"

// DT Include - 전역 DeltaTime 접근을 위한 extern 선언
extern float GDeltaTime;

// 빌드 조건에 따른 Library 분류
#ifdef _DEBUG
#define DIRECTX_TOOL_KIT R"(DirectXTK\DirectXTK_debug)"
#else
#define DIRECTX_TOOL_KIT R"(DirectXTK\DirectXTK)"
#endif

// Library Linking
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, DIRECTX_TOOL_KIT)
