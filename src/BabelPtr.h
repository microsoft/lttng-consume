// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <babeltrace/babeltrace.h>

namespace LttngConsume {

template<class Details>
class BabelPtr
{
    using BTType = typename Details::BTType;

  public:
    BabelPtr() : _ptr(nullptr) {}

    ~BabelPtr() { Details::PutFunc(_ptr); }

    BabelPtr(const BabelPtr& other) : _ptr(other._ptr)
    {
        Details::GetFunc(other._ptr);
    }

    BabelPtr(BabelPtr&& other)
    {
        _ptr = other._ptr;
        other._ptr = nullptr;
    }

    BabelPtr(const BTType* ptr) : _ptr(ptr) {}

    BabelPtr& operator=(const BabelPtr& other)
    {
        Details::GetFunc(other._ptr);
        DiscardCurrentAndAttach(other._ptr);
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
        Details::GetFunc(ptr);
    }

    const BTType* Detach()
    {
        BTType* ptr = _ptr;
        _ptr = nullptr;
        return ptr;
    }

    const BTType* Get() { return _ptr; }

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
    void DiscardCurrentAndAttach(const BTType* ptr)
    {
        Details::PutFunc(_ptr);
        _ptr = ptr;
    }

  private:
    BTType* _ptr;
};

#define MAKE_PTR_TYPE(BabelPtrPrefix, LibBabeltraceType)               \
    namespace details {                                                \
    struct BabelPtrPrefix##Details                                     \
    {                                                                  \
        using BTType = LibBabeltraceType;                              \
        static void GetFunc(const BTType* p) { bt_plugin_get_ref(p); } \
        static void PutFunc(const BTType* p) { bt_plugin_put_ref(p); } \
    };                                                                 \
    }                                                                  \
    using BabelPtrPrefix##Ptr = BabelPtr<details::BabelPtrPrefix##Details>;

MAKE_PTR_TYPE(BtPlugin, bt_plugin);
}
