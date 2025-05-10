#pragma once
#include "Assertion.h"
#include <vector>

// for some reason felt the need to implement a linked list...

template<typename T>
struct LinkedListNode
{
    T elem;
    LinkedListNode<T>* prev = nullptr;
    LinkedListNode<T>* next = nullptr;
    int RefCount = 0;
};

// A linked list is either an owner or a viewer. Whenever it is combined with another linked list, it becomes a viewer.
// The heads are ref-counted. 
template<typename T>
class LinkedList
{
    LinkedListNode<T>* headIndex = nullptr;
    LinkedListNode<T>* tailIndex = nullptr;
    int size = 0;

public:
    bool IsOwner() const
    {
        if (size == 0) return true;
        if (headIndex->prev == nullptr && tailIndex->next == nullptr) return true;
        return false;
    }

    LinkedList(std::vector<LinkedListNode<T>* head, std::vector<LinkedListNode<T>* tail, int count)
        : headIndex(head), tailIndex(tail), size(count)
    {
        Assert(size > 0 || (headIndex == nullptr && tailIndex == nullptr), "Empty linked list must have head and tail set to -1.");
        if (IsOwner() && size != 0)
        {
            headIndex->RefCount += 1;
        }
    }

    template<typename... Args>
    void AddItems(const T& t, Args... args)
    {
        container.push_back({t});
        if (headIndex == -1)
        {
            headIndex = container.size() - 1;
            tailIndex = headIndex;
        }
        else
        {
            container.back().prev = tailIndex;
            container[tailIndex].next = container.size() - 1;
            tailIndex = container.size() - 1;
        }
        size++;
        if constexpr(sizeof...(args) != 0)
        {
            AddItems(args...);
        }
    }

    template<typename... Args>
    LinkedList(std::vector<LinkedListNode<T>>& vec, const T& t, Args... args)
        : container(vec)
    {
        AddItems(t, args...);
    }

    LinkedList(std::vector<LinkedListNode<T>>& vec)
        : container(vec)
    {
    }

    LinkedList<T>& operator=(const LinkedList<T>& other)
    {
        container = other.container;
        headIndex = other.headIndex;
        tailIndex = other.tailIndex;
        size = other.size;
        return *this;
    }


    int GetSize() const
    {
        return size;
    }

    T& operator[](int index)
    {
        if (!IsValid()) std::cout << "Warning: a linked list should not be used after it has been added to another.\n";
        if ()

        LinkedListNode<T>& ll = container[headIndex];
        for (int i = 0; i < index; i++)
        {
            ll = container[ll.next];
        }
        return ll.elem;
    }

    LinkedList<T> operator+(const LinkedList<T>& other)
    {
        if (!IsValid())
        {
            if (!other.IsValid())
            {
                std::cout << "Warning: a linked list should not be used after it has been added to another.\n";
            }
            else
            {
                return other;
            }
        }
        else if (!other.IsValid())
        {
            return *this;
        }

        // stitch together
        container[tailIndex].next = other.headIndex;
        container[other.headIndex].prev = tailIndex;

        return { container, headIndex, other.tailIndex, GetSize() + other.GetSize() };
    }

    LinkedList<T> Copy()
    {
        if (headIndex == -1) return { container };

        if (!IsValid()) std::cout << "Warning: a linked list should not be used after it has been added to another.\n";

        LinkedList<T> ret = { container, -1, -1, 0 };
        LinkedListNode<T>& ll = container[headIndex];
        while (true)
        {
            ret.AddItems(ll.elem);
            if (ll.next == -1) break;
            ll = container[ll.next];
        }
        return ret;
    }
};

