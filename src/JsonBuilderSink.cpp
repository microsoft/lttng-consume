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

    bt_component_class_sink_consume_method_status Run();

    bt_component_class_sink_graph_is_configured_method_status
    GraphIsConfigured(bt_self_component_sink* self);

  public:
    static constexpr const char* c_inputPortName = "in";

  private:
    void HandleMessage(const bt_message* message);

  private:
    BabelPtr<bt_self_component_port_input_message_iterator> _messageItr;
    std::function<void(JsonBuilder&&)>& _outputFunc;
};

bt_component_class_sink_consume_method_status JsonBuilderSink::Run()
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

    bt_message_iterator_next_status status =
        bt_self_component_port_input_message_iterator_next(
            _messageItr.Get(), &messageArray.Messages, &messageArray.Count);

    std::cout << "JsonBuilderSink|Run: NextIteratorStatus[" << status << "]\n";

    switch (status)
    {
    case BT_MESSAGE_ITERATOR_NEXT_STATUS_END:
        _messageItr.Reset();
        return BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_END;
    case BT_MESSAGE_ITERATOR_NEXT_STATUS_AGAIN:
        return BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_AGAIN;
    case BT_MESSAGE_ITERATOR_NEXT_STATUS_OK:
        break;
    default:
        return BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_ERROR;
    }

    for (uint64_t i = 0; i < messageArray.Count; i++)
    {
        std::cout << "JsonBuilderSink|HandleMessage: Count[" << i << "]\n";
        const bt_message* message = messageArray.Messages[i];
        if (bt_message_get_type(message) == BT_MESSAGE_TYPE_EVENT)
        {
            HandleMessage(message);
        }
    }

    std::cout << "JsonBuilderSink|HandleMessage: Complete\n";

    return BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_OK;
}

bt_component_class_sink_graph_is_configured_method_status
JsonBuilderSink::GraphIsConfigured(bt_self_component_sink* self)
{
    bt_self_component_port_input* inputPort =
        bt_self_component_sink_borrow_input_port_by_name(self, c_inputPortName);

    bt_self_component_port_input_message_iterator_create_from_sink_component_status status =
        bt_self_component_port_input_message_iterator_create_from_sink_component(
            self, inputPort, &_messageItr);
    if (status !=
        BT_SELF_COMPONENT_PORT_INPUT_MESSAGE_ITERATOR_CREATE_FROM_SINK_COMPONENT_STATUS_OK)
    {
        return static_cast<bt_component_class_sink_graph_is_configured_method_status>(
            status);
    }
    FAIL_FAST_IF(!_messageItr);

    std::cout << "JsonBuilderSink|GraphIsConfigured Complete\n";

    return BT_COMPONENT_CLASS_SINK_GRAPH_IS_CONFIGURED_METHOD_STATUS_OK;
}

void JsonBuilderSink::HandleMessage(const bt_message* message)
{
    LttngJsonReader reader;
    JsonBuilder builder = reader.DecodeEvent(message);

    _outputFunc(std::move(builder));
}

bt_component_class_sink_consume_method_status
JsonBuilderSink_RunStatic(bt_self_component_sink* self)
{
    auto jbSink = static_cast<JsonBuilderSink*>(bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self)));

    return jbSink->Run();
}

bt_component_class_initialize_method_status JsonBuilderSink_InitStatic(
    bt_self_component_sink* self,
    bt_self_component_sink_configuration*,
    const bt_value*,
    void* init_method_data)
{
    bt_self_component_add_port_status addPortStatus =
        bt_self_component_sink_add_input_port(
            self, JsonBuilderSink::c_inputPortName, nullptr, nullptr);
    if (addPortStatus != BT_SELF_COMPONENT_ADD_PORT_STATUS_OK)
    {
        return static_cast<bt_component_class_initialize_method_status>(
            addPortStatus);
    }

    // Construct the JsonBuilderSink instance

    FAIL_FAST_IF(init_method_data == nullptr);
    JsonBuilderSinkInitParams* params =
        static_cast<JsonBuilderSinkInitParams*>(init_method_data);

    // Check each param
    FAIL_FAST_IF(params->OutputFunc == nullptr);

    // Set the user data, passing ownership in the case of success
    JsonBuilderSink* jsonBuilderSink = new JsonBuilderSink(*params->OutputFunc);
    bt_self_component_set_data(
        bt_self_component_sink_as_self_component(self), jsonBuilderSink);

    return BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_OK;
}

bt_component_class_sink_graph_is_configured_method_status
JsonBuilderSink_GraphIsConfiguredStatic(bt_self_component_sink* self)
{
    auto jbSink = static_cast<JsonBuilderSink*>(bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self)));

    return jbSink->GraphIsConfigured(self);
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
    bt_component_class_sink_set_initialize_method(
        jsonBuilderSinkClass.Get(), JsonBuilderSink_InitStatic);
    bt_component_class_sink_set_graph_is_configured_method(
        jsonBuilderSinkClass.Get(), JsonBuilderSink_GraphIsConfiguredStatic);
    bt_component_class_sink_set_finalize_method(
        jsonBuilderSinkClass.Get(), JsonBuilderSink_FinalizeStatic);

    BabelPtr<const bt_component_class_sink> returnVal =
        jsonBuilderSinkClass.Detach();

    return returnVal;
}

}
