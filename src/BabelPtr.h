// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <type_traits>

#include <babeltrace2/babeltrace.h>

namespace LttngConsume {

namespace details {

// Specialized below with macros
template<class BabeltraceType>
struct BabelPtrRefCountHelper
{};

}

template<class BTType>
class BabelPtr
{
    using RefCounter =
        typename details::BabelPtrRefCountHelper<typename std::remove_cv<BTType>::type>;

  public:
    BabelPtr() : _ptr(nullptr) {}

    ~BabelPtr() { RefCounter::PutFunc(_ptr); }

    BabelPtr(const BabelPtr& other) : _ptr(other._ptr)
    {
        RefCounter::GetFunc(other._ptr);
    }

    BabelPtr(BabelPtr&& other)
    {
        _ptr = other._ptr;
        other._ptr = nullptr;
    }

    BabelPtr(BTType* ptr) : _ptr(ptr) {}

    BabelPtr& operator=(const BabelPtr& other)
    {
        RefCounter::GetFunc(other._ptr);
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
        RefCounter::PutFunc(_ptr);
        _ptr = ptr;
    }

  private:
    BTType* _ptr;
};

#define MAKE_PTR_TYPE(BabeltraceType)                \
    namespace details {                              \
    template<>                                       \
    struct BabelPtrRefCountHelper<BabeltraceType>    \
    {                                                \
        static void GetFunc(const BabeltraceType* p) \
        {                                            \
            BabeltraceType##_get_ref(p);             \
        }                                            \
        static void PutFunc(const BabeltraceType* p) \
        {                                            \
            BabeltraceType##_put_ref(p);             \
        }                                            \
    };                                               \
    }

MAKE_PTR_TYPE(bt_plugin)
MAKE_PTR_TYPE(bt_component_class_sink)
MAKE_PTR_TYPE(bt_value)
MAKE_PTR_TYPE(bt_graph)
MAKE_PTR_TYPE(bt_component_source)
MAKE_PTR_TYPE(bt_component_filter)
MAKE_PTR_TYPE(bt_component_sink)
MAKE_PTR_TYPE(bt_message_iterator)
}
