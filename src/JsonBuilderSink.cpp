// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "JsonBuilderSink.h"

#include <array>
#include <memory>

#include <babeltrace2/babeltrace.h>

#include "BabelPtr.h"
#include "FailureHelpers.h"
#include "LttngJsonReader.h"

using namespace jsonbuilder;

namespace LttngConsume {
class JsonBuilderSink
{
  public:
    JsonBuilderSink(std::function<void(JsonBuilder&&)>& outputFunc)
        : _outputFunc(outputFunc)
    {}

    bt_self_component_status Run();

    bt_self_component_status PortConnected(
        bt_self_component_port_input* self_port,
        const bt_port_output* other_port);

  private:
    bt_self_component_status HandleMessage(const bt_message* message);

  private:
    BabelPtr<bt_self_component_port_input_message_iterator> _messageItr;
    std::function<void(JsonBuilder&&)>& _outputFunc;
};

bt_self_component_status JsonBuilderSink::Run()
{
    struct MessageArray
    {
        const bt_message** Messages = nullptr;
        uint64_t Count = 0;

        ~MessageArray()
        {
            for (uint64_t i = 0; i < Count; i++)
            {
                bt_message_put_ref(Messages[i]);
            }
        }
    };

    MessageArray messageArray;

    bt_message_iterator_status status =
        bt_self_component_port_input_message_iterator_next(
            _messageItr.Get(), &messageArray.Messages, &messageArray.Count);

    switch (status)
    {
    case BT_MESSAGE_ITERATOR_STATUS_END:
        _messageItr.Reset();
        return BT_SELF_COMPONENT_STATUS_END;
    case BT_MESSAGE_ITERATOR_STATUS_AGAIN:
        return BT_SELF_COMPONENT_STATUS_AGAIN;
    case BT_MESSAGE_ITERATOR_STATUS_OK:
        break;
    default:
        return BT_SELF_COMPONENT_STATUS_ERROR;
    }

    for (uint64_t i = 0; i < messageArray.Count; i++)
    {
        const bt_message* message = messageArray.Messages[i];
        if (bt_message_get_type(message) == BT_MESSAGE_TYPE_EVENT)
        {
            bt_self_component_status selfStatus = HandleMessage(message);
            if (selfStatus != BT_SELF_COMPONENT_STATUS_OK)
            {
                return selfStatus;
            }
        }
    }

    return BT_SELF_COMPONENT_STATUS_OK;
}

bt_self_component_status JsonBuilderSink::PortConnected(
    bt_self_component_port_input* self_port,
    const bt_port_output* other_port)
{
    (void) other_port;

    _messageItr = bt_self_component_port_input_message_iterator_create(self_port);
    FAIL_FAST_IF(!_messageItr);

    return BT_SELF_COMPONENT_STATUS_OK;
}

bt_self_component_status JsonBuilderSink::HandleMessage(const bt_message* message)
{
    LttngJsonReader reader;
    JsonBuilder builder = reader.DecodeEvent(message);

    _outputFunc(std::move(builder));

    return BT_SELF_COMPONENT_STATUS_OK;
}

bt_self_component_status JsonBuilderSink_RunStatic(bt_self_component_sink* self)
{
    auto jbSink = static_cast<JsonBuilderSink*>(bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self)));

    return jbSink->Run();
}

bt_self_component_status
JsonBuilderSink_InitStatic(bt_self_component_sink* self, const bt_value*, void* initParams)
{
    bt_self_component_status status =
        bt_self_component_sink_add_input_port(self, "in", nullptr, nullptr);
    if (status != BT_SELF_COMPONENT_STATUS_OK)
    {
        return status;
    }

    // Construct the JsonBuilderSink instance

    FAIL_FAST_IF(initParams == nullptr);
    JsonBuilderSinkInitParams* params =
        static_cast<JsonBuilderSinkInitParams*>(initParams);

    // Check each param
    FAIL_FAST_IF(params->OutputFunc == nullptr);

    // Set the user data, passing ownership in the case of success
    JsonBuilderSink* jsonBuilderSink = new JsonBuilderSink(*params->OutputFunc);
    bt_self_component_set_data(
        bt_self_component_sink_as_self_component(self), jsonBuilderSink);

    return BT_SELF_COMPONENT_STATUS_OK;
}

bt_self_component_status JsonBuilderSink_InputPortConnectedStatic(
    bt_self_component_sink* self,
    bt_self_component_port_input* self_port,
    const bt_port_output* other_port)
{
    auto jbSink = static_cast<JsonBuilderSink*>(bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self)));

    return jbSink->PortConnected(self_port, other_port);
}

void JsonBuilderSink_FinalizeStatic(bt_self_component_sink* self)
{
    auto jbSink = static_cast<JsonBuilderSink*>(bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self)));

    delete jbSink;
}

BabelPtr<const bt_component_class_sink> GetJsonBuilderSinkComponentClass()
{
    BabelPtr<bt_component_class_sink> jsonBuilderSinkClass =
        bt_component_class_sink_create("jsonbuilder", JsonBuilderSink_RunStatic);
    bt_component_class_sink_set_init_method(
        jsonBuilderSinkClass.Get(), JsonBuilderSink_InitStatic);
    bt_component_class_sink_set_input_port_connected_method(
        jsonBuilderSinkClass.Get(), JsonBuilderSink_InputPortConnectedStatic);
    bt_component_class_sink_set_finalize_method(
        jsonBuilderSinkClass.Get(), JsonBuilderSink_FinalizeStatic);

    BabelPtr<const bt_component_class_sink> returnVal =
        jsonBuilderSinkClass.Detach();

    return returnVal;
}

}
