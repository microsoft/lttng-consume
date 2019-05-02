// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <babeltrace/ref.h>

namespace LttngConsume {

template<class BTType>
class BabelPtr
{
  public:
    BabelPtr() : _ptr(nullptr) {}

    ~BabelPtr() { bt_put(_ptr); }

    BabelPtr(const BabelPtr& other) : _ptr(bt_get(other._ptr)) {}

    BabelPtr(BabelPtr&& other)
    {
        _ptr = other._ptr;
        other._ptr = nullptr;
    }

    BabelPtr(BTType* ptr) : _ptr(ptr) {}

    BabelPtr& operator=(const BabelPtr& other)
    {
        BTType* ptr = bt_get(other._ptr);
        DiscardCurrentAndAttach(ptr);
        return *this;
    }

    BabelPtr& operator=(BabelPtr&& other)
    {
        DiscardCurrentAndAttach(other._ptr);
        other._ptr = nullptr;
        return *this;
    }

    BabelPtr& operator=(BTType* ptr)
    {
        DiscardCurrentAndAttach(ptr);
        return *this;
    }

    void IncrementingOwn(BTType* ptr)
    {
        DiscardCurrentAndAttach(ptr);
        bt_get(ptr);
    }

    BTType* Detach()
    {
        BTType* ptr = _ptr;
        _ptr = nullptr;
        return ptr;
    }

    BTType* Get() { return _ptr; }

    void Reset() { DiscardCurrentAndAttach(nullptr); }

    BTType** GetAddressOf()
    {
        DiscardCurrentAndAttach(nullptr);
        return &_ptr;
    }

    BTType** operator&() { return GetAddressOf(); }

    explicit operator bool() { return _ptr != nullptr; }

    bool operator==(const BabelPtr& other) { return _ptr == other._ptr; }

  private:
    void DiscardCurrentAndAttach(BTType* ptr)
    {
        bt_put(_ptr);
        _ptr = ptr;
    }

  private:
    BTType* _ptr;
};

}
