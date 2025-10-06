#pragma once

//STL Redefine
#include <vector>
#include <unordered_set>
#include <list>
#include <queue>
#include <stack>

template<typename T, typename Alloc = std::allocator<T>>
using TLinkedList = std::list<T, Alloc>;
template<typename T, typename Alloc = std::allocator<T>>
using TDoubleLinkedList = std::list<T, Alloc>;
template<typename T1, typename T2>
using TPair = std::pair<T1, T2>;
template<typename T, size_t N>
using TStaticArray = std::array<T, N>;
template<typename T, typename Container = std::deque<T>>
using TQueue = std::queue<T, Container>;

using uint8 = std::uint8_t;
using int8 = std::int8_t;
using uint16 = std::uint16_t;
using int16 = std::int16_t;
using uint32 = std::uint32_t;
using int32 = std::int32_t;
using uint64 = std::uint64_t;
using int64 = std::int64_t;

// Extension
#include "FString.h"

// Container
#include "Source/Runtime/Core/Public/Containers/TArray.h"
#include "Source/Runtime/Core/Public/Containers/TMap.h"
#include "Source/Runtime/Core/Public/Containers/TSet.h"
