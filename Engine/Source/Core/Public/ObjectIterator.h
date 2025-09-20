#pragma once
#include "Core/Public/Object.h"

template <typename T>
class TObjectIterator
{
public:
	using ElementType = T;

	TObjectIterator()
		: Index(0)
		, EndIndex(GUObjectArray.size())
	{
		AdvanceToValid();
	}

	// End sentinel iterator
	static TObjectIterator End()
	{
		TObjectIterator It;
		It.Index = It.EndIndex;
		It.Current = nullptr;
		return It;
	}

	// Dereference
	T* operator*() const { return Current; }
	T* operator->() const { return Current; }

	// Increment (prefix)
	TObjectIterator& operator++()
	{
		++Index; // AdvanceToValid에서 현재 인덱스도 검사하므로 미리 하나 증가시키고 시작
		AdvanceToValid();
		return *this;
	}

	// Bool context (for while/for conditions)
	explicit operator bool() const { return Current != nullptr; }

	// Range-for compatibility
	bool operator!=(const TObjectIterator& InOther) const { return Index != InOther.Index; }

private:
	void AdvanceToValid()
	{
		Current = nullptr;
		for (int i = Index; i < EndIndex; ++i)
		{
			UObject* Obj = GUObjectArray[i].Get<UObject>();
			if (Obj)
			{
				auto* CastedObj = Cast<T>(Obj);
				if (CastedObj)
				{
					Current = CastedObj
					Index = i;
					return;
				}
			}
		}
	}

	size_t Index;
	size_t EndIndex;
	T* Current = nullptr;
};

template <typename T>
struct TObjectRange
{
	TObjectIterator<T> begin() const { return TObjectIterator<T>(); }
	TObjectIterator<T> end() const { return TObjectIterator<T>::End(); }
};

template <typename T>
inline TObjectRange<T> MakeObjectRange()
{
	return TObjectRange<T>();
}

