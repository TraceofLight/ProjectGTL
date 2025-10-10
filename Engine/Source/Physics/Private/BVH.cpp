#include "pch.h"
#include "Physics/Public/BVH.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"

FBVH::FBVH()
{
}

FBVH::~FBVH()
{
}

int32 FBVH::Add(UPrimitiveComponent* Primitive)
{
    // 1. 노드 할당
    const int32 leafNodeIndex = AllocateNode();
    FBVHNode& leafNode = Nodes[leafNodeIndex];

    // 2. 리프 노드 설정
    leafNode.Primitive = Primitive;
    Primitive->GetWorldAABB(leafNode.BoundingBox.Min, leafNode.BoundingBox.Max);
    leafNode.ParentIndex = -1;
    leafNode.LeftChildIndex = -1;
    leafNode.RightChildIndex = -1;

    // 3. 트리에 삽입
    InsertLeaf(leafNodeIndex);

    return leafNodeIndex;
}

void FBVH::Remove(int32 NodeIndex)
{
    if (NodeIndex < 0 || NodeIndex >= Nodes.Num())
    {
        return; // 유효하지 않은 인덱스
    }

    RemoveLeaf(NodeIndex);
    FreeNode(NodeIndex);
}

bool FBVH::Update(int32 NodeIndex)
{
    if (NodeIndex < 0 || NodeIndex >= Nodes.Num())
    {
        return false; // 유효하지 않은 인덱스
    }

    FBVHNode& node = Nodes[NodeIndex];
    if (!node.IsLeaf() || !node.Primitive)
    {
        return false;
    }

    // 현재 프리미티브의 AABB
    FAABB primitiveAABB;
    node.Primitive->GetWorldAABB(primitiveAABB.Min, primitiveAABB.Max);

    // 기존 AABB가 현재 AABB를 포함하는지 확인
    if (node.BoundingBox.Min.X <= primitiveAABB.Min.X &&
        node.BoundingBox.Min.Y <= primitiveAABB.Min.Y &&
        node.BoundingBox.Min.Z <= primitiveAABB.Min.Z &&
        node.BoundingBox.Max.X >= primitiveAABB.Max.X &&
        node.BoundingBox.Max.Y >= primitiveAABB.Max.Y &&
        node.BoundingBox.Max.Z >= primitiveAABB.Max.Z)
    {
        return false; // 업데이트 필요 없음
    }

    // AABB가 변경되었으므로, 노드를 제거하고 다시 삽입
    RemoveLeaf(NodeIndex);
    node.BoundingBox = primitiveAABB;
    InsertLeaf(NodeIndex);

    return true;
}


void FBVH::InsertLeaf(int32 LeafNodeIndex)
{
    // 1. 트리가 비어있는 경우
    if (RootNodeIndex == -1)
    {
        RootNodeIndex = LeafNodeIndex;
        return;
    }

    // 2. 트리가 비어있지 않은 경우, 최적의 위치를 찾아서 삽입
    FAABB leafAABB = Nodes[LeafNodeIndex].BoundingBox;
    int32 currentNodeIndex = RootNodeIndex;

    // 리프 노드에 도달할 때까지 탐색
    while (!Nodes[currentNodeIndex].IsLeaf())
    {
        FBVHNode& currentNode = Nodes[currentNodeIndex];
        int32 leftChildIndex = currentNode.LeftChildIndex;
        int32 rightChildIndex = currentNode.RightChildIndex;

        FAABB combinedAABB = FAABB::Merge(currentNode.BoundingBox, leafAABB);
        float combinedArea = combinedAABB.GetArea();

        // 현재 노드에 머무르는 비용
        float pushDownCost = 2.0f * combinedArea;

        // 자식으로 내려가는 비용
        float descendCost = 2.0f * (combinedArea - currentNode.BoundingBox.GetArea());

        // 왼쪽 자식과 합쳤을 때의 비용 계산
        FBVHNode& leftChild = Nodes[leftChildIndex];
        FAABB leftCombined = FAABB::Merge(leafAABB, leftChild.BoundingBox);
        float leftCost = descendCost + leftCombined.GetArea();
        if (!leftChild.IsLeaf())
        {
            leftCost -= leftChild.BoundingBox.GetArea();
        }


        // 오른쪽 자식과 합쳤을 때의 비용 계산
        FBVHNode& rightChild = Nodes[rightChildIndex];
        FAABB rightCombined = FAABB::Merge(leafAABB, rightChild.BoundingBox);
        float rightCost = descendCost + rightCombined.GetArea();
        if (!rightChild.IsLeaf())
        {
            rightCost -= rightChild.BoundingBox.GetArea();
        }


        // 비용이 가장 적은 쪽으로 이동
        if (pushDownCost < leftCost && pushDownCost < rightCost)
        {
            break; // 현재 노드를 부모로 결정
        }

        if (leftCost < rightCost)
        {
            currentNodeIndex = leftChildIndex;
        }
        else
        {
            currentNodeIndex = rightChildIndex;
        }
    }

    // 3. 새로운 부모 노드 생성 및 연결
    int32 siblingIndex = currentNodeIndex;
    FBVHNode& siblingNode = Nodes[siblingIndex];
    int32 oldParentIndex = siblingNode.ParentIndex;

    int32 newParentIndex = AllocateNode();
    FBVHNode& newParentNode = Nodes[newParentIndex];
    newParentNode.ParentIndex = oldParentIndex;
    newParentNode.BoundingBox = FAABB::Merge(leafAABB, siblingNode.BoundingBox);
    newParentNode.LeftChildIndex = siblingIndex;
    newParentNode.RightChildIndex = LeafNodeIndex;

    Nodes[LeafNodeIndex].ParentIndex = newParentIndex;
    siblingNode.ParentIndex = newParentIndex;

    if (oldParentIndex == -1)
    {
        // 기존 루트가 sibling이었던 경우
        RootNodeIndex = newParentIndex;
    }
    else
    {
        // 기존 부모의 자식을 새로운 부모로 교체
        FBVHNode& oldParentNode = Nodes[oldParentIndex];
        if (oldParentNode.LeftChildIndex == siblingIndex)
        {
            oldParentNode.LeftChildIndex = newParentIndex;
        }
        else
        {
            oldParentNode.RightChildIndex = newParentIndex;
        }
    }

    // 4. 조상 노드들의 AABB 업데이트
    int32 ancestorIndex = newParentIndex;
    while (ancestorIndex != -1)
    {
        FBVHNode& ancestorNode = Nodes[ancestorIndex];
        int32 leftChild = ancestorNode.LeftChildIndex;
        int32 rightChild = ancestorNode.RightChildIndex;

        ancestorNode.BoundingBox = FAABB::Merge(Nodes[leftChild].BoundingBox, Nodes[rightChild].BoundingBox);
        ancestorIndex = ancestorNode.ParentIndex;
    }
}

void FBVH::RemoveLeaf(int32 LeafNodeIndex)
{
    // 1. 루트 노드인 경우
    if (LeafNodeIndex == RootNodeIndex)
    {
        RootNodeIndex = -1;
        return;
    }

    int32 parentIndex = Nodes[LeafNodeIndex].ParentIndex;
    FBVHNode& parentNode = Nodes[parentIndex];
    int32 grandParentIndex = parentNode.ParentIndex;

    // 2. 형제 노드 찾기
    int32 siblingIndex = (parentNode.LeftChildIndex == LeafNodeIndex) ? parentNode.RightChildIndex : parentNode.LeftChildIndex;

    // 3. 부모의 부모와 형제 노드를 연결
    if (grandParentIndex != -1)
    {
        FBVHNode& grandParentNode = Nodes[grandParentIndex];
        if (grandParentNode.LeftChildIndex == parentIndex)
        {
            grandParentNode.LeftChildIndex = siblingIndex;
        }
        else
        {
            grandParentNode.RightChildIndex = siblingIndex;
        }
        Nodes[siblingIndex].ParentIndex = grandParentIndex;

        // 부모 노드 제거
        FreeNode(parentIndex);

        // 4. 조상 노드들의 AABB 업데이트
        int32 ancestorIndex = grandParentIndex;
        while (ancestorIndex != -1)
        {
            FBVHNode& ancestorNode = Nodes[ancestorIndex];
            int32 leftChild = ancestorNode.LeftChildIndex;
            int32 rightChild = ancestorNode.RightChildIndex;

            ancestorNode.BoundingBox = FAABB::Merge(Nodes[leftChild].BoundingBox, Nodes[rightChild].BoundingBox);
            ancestorIndex = ancestorNode.ParentIndex;
        }
    }
    else // 부모가 루트였던 경우
    {
        RootNodeIndex = siblingIndex;
        Nodes[siblingIndex].ParentIndex = -1;
        FreeNode(parentIndex);
    }
}


int32 FBVH::AllocateNode()
{
    if (!FreeNodeIndices.IsEmpty())
    {
        int32 index = FreeNodeIndices.Top();
        FreeNodeIndices.Pop();
        Nodes[index] = {}; // 노드 초기화
        return index;
    }
    else
    {
        Nodes.Emplace();
        return static_cast<int32>(Nodes.Num() - 1);
    }
}

void FBVH::FreeNode(int32 NodeIndex)
{
    if (NodeIndex < 0 || NodeIndex >= Nodes.Num())
    {
        return;
    }
    FreeNodeIndices.Add(NodeIndex);
}
