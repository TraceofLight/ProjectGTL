#pragma once

#include "Physics/Public/AABB.h"
#include "Global/Types.h"

// Forward Declaration
class UPrimitiveComponent;

/**
 * @brief BVH 노드 구조체. 내부 노드(Internal Node)와 리프 노드(Leaf Node)의 역할을 겸합니다.
 */
struct FBVHNode
{
    // 노드가 감싸는 전체 영역
    FAABB BoundingBox;

    // 부모 노드의 인덱스. 트리를 거슬러 올라갈 때 사용됩니다.
    int32 ParentIndex = -1;

    // 자식 노드의 인덱스. 리프 노드일 경우 -1 값을 가집니다.
    int32 LeftChildIndex = -1;
    int32 RightChildIndex = -1;

    // 리프 노드일 경우에만 유효하며, 실제 오브젝트(Primitive)를 가리킵니다.
    // 현재는 단일 오브젝트만 담는다고 가정합니다.
    UPrimitiveComponent* Primitive = nullptr;

    /**
     * @brief 이 노드가 리프 노드인지 확인합니다.
     * @return 자식 노드가 없으면 true를 반환합니다.
     */
    bool IsLeaf() const
    {
        return LeftChildIndex == -1;
    }
};

/**
 * @brief 동적 바운딩 볼륨 계층(Dynamic Bounding Volume Hierarchy) 클래스.
 *        오브젝트의 추가, 제거, 갱신이 빈번한 실시간 환경에 적합합니다.
 */
class FBVH
{
public:
    FBVH();
    ~FBVH();

    // BVH에 프리미티브(오브젝트)를 추가하고, 해당 노드의 인덱스를 반환합니다.
    int32 Add(UPrimitiveComponent* Primitive);

    // 특정 노드(프리미티브)를 BVH에서 제거합니다.
    void Remove(int32 NodeIndex);

    // 프리미티브의 변형(Transform)이 변경되었을 때 호출하여 BVH를 갱신합니다.
    bool Update(int32 NodeIndex);

    // TODO: 공간 쿼리 인터페이스
    // bool Raycast(const FVector& Start, const FVector& End, FHitResult& OutHit) const;
    // void OverlapQuery(const FAABB& Box, TArray<UPrimitiveComponent*>& OutOverlaps) const;

private:
    // BVH의 모든 노드를 저장하는 배열. 포인터 대신 인덱스를 사용하여 캐시 효율을 높입니다.
    TArray<FBVHNode> Nodes;

    // BVH 트리의 루트 노드 인덱스.
    int32 RootNodeIndex = -1;

    // 비어있는 노드 인덱스 목록. 노드 제거 시 배열의 빈 공간을 재사용하기 위해 사용합니다.
    TArray<int32> FreeNodeIndices;

    // 내부 헬퍼 함수들
    void InsertLeaf(int32 LeafNodeIndex);
    void RemoveLeaf(int32 LeafNodeIndex);
    int32 AllocateNode();
    void FreeNode(int32 NodeIndex);
};

