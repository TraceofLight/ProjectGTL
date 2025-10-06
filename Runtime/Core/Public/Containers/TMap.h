#pragma once

#include <unordered_map>
#include <utility> // for std::pair
#include <type_traits>

namespace Containers
{
    template<typename KeyType, typename ValueType>
    class TMap
    {
    public:
        using MapType = std::unordered_map<KeyType, ValueType>;

        TMap() = default;
        ~TMap() = default;

        // 요소 추가 또는 값 업데이트
        void Add(const KeyType& Key, const ValueType& Value)
        {
            Map.emplace(Key, Value);
        }

        // 요소 삭제 (키 기준)
        bool Remove(const KeyType& Key)
        {
            return Map.erase(Key) > 0;
        }

        // 해당 키가 존재하는지 검사
        bool Contains(const KeyType& Key) const
        {
            return Map.find(Key) != Map.end();
        }

        // 요소 가져오기 (키 기준)
        // 요소가 없으면 nullptr 반환
        ValueType* Find(const KeyType& Key)
        {
            auto Iter = Map.find(Key);
            if (Iter != Map.end())
                return &(Iter->second);
            return nullptr;
        }

        // const 버전
        const ValueType* Find(const KeyType& Key) const
        {
            auto Iter = Map.find(Key);
            if (Iter != Map.end())
                return &(Iter->second);
            return nullptr;
        }

        // 키 + 값으로 반복자 반환, begin() 과 end() 함수도 구현
        typename MapType::iterator begin() { return Map.begin(); }
        typename MapType::iterator end() { return Map.end(); }
        typename MapType::const_iterator begin() const { return Map.begin(); }
        typename MapType::const_iterator end() const { return Map.end(); }

        // find 함수 - STL과 호환성을 위해
        typename MapType::iterator find(const KeyType& Key) { return Map.find(Key); }
        typename MapType::const_iterator find(const KeyType& Key) const { return Map.find(Key); }

        // 요소 수 반환
        size_t Num() const { return Map.size(); }

        // 모든 요소 삭제
        void Empty() { Map.clear(); }

    private:
        MapType Map;
    };
}

// 편의성: 언리얼 스타일 네임스페이스 없이 TMap 사용
template<typename KeyType, typename ValueType>
using TMap = Containers::TMap<KeyType, ValueType>;