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
#include <queue>
#include <mutex>

// Global Included
#include "Global/Types.h"
#include "Global/Memory.h"
#include "Global/Constant.h"
#include "Global/Enum.h"
#include "Global/Matrix.h"
#include "Global/Quaternion.h"
#include "Global/Vector.h"
#include "Global/CoreTypes.h"
#include "Global/Macro.h"
#include "Global/Function.h"
#include "Global/Paths.h"

// Pointer
#include "Source/Runtime/Core/Public/Templates/SharedPtr.h"
#include "Source/Runtime/Core/Public/Templates/UniquePtr.h"
#include "Source/Runtime/Core/Public/Templates/WeakPtr.h"

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
using std::mutex;
using std::lock_guard;
using std::atomic;
using std::queue;
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
#include "Runtime/UI/Window/Public/ConsoleWindow.h"

#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Engine/Public/EditorEngine.h"
#include "Runtime/Engine/Public/GameInstance.h"
#include "Runtime/Engine/Public/LocalPlayer.h"

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
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, DIRECTX_TOOL_KIT)

#define EDITOR_MODE 1
