// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "JsonBuilderSink.h"

#include <array>
#include <memory>

// clang-format off
// #include <babeltrace/ctf-ir/field-types.h>
#include <babeltrace/babeltrace.h>
// clang-format on

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

    void PortConnected(
        bt_self_component_port_input* self_port,
        const bt_port_output* other_port);

  private:
    bt_self_component_status PersistEvent(bt_notification* eventNotif);

  private:
    BabelPtr<bt_notification_iterator> _notifIter;
    std::function<void(JsonBuilder&&)>& _outputFunc;
};

bt_self_component_status JsonBuilderSink::Run()
{
    bt_notification_iterator_status status =
        bt_notification_iterator_next(_notifIter.Get());
    switch (status)
    {
    case BT_NOTIFICATION_ITERATOR_STATUS_END:
        _notifIter.Reset();
        return BT_SELF_COMPONENT_STATUS_END;
    case BT_NOTIFICATION_ITERATOR_STATUS_AGAIN:
        return BT_SELF_COMPONENT_STATUS_AGAIN;
    case BT_NOTIFICATION_ITERATOR_STATUS_OK:
        break;
    default:
        return BT_SELF_COMPONENT_STATUS_ERROR;
    }

    BabelPtr<bt_notification> notification =
        bt_notification_iterator_get_notification(_notifIter.Get());
    switch (bt_notification_get_type(notification.Get()))
    {
    case BT_NOTIFICATION_TYPE_EVENT:
    {
        return PersistEvent(notification.Get());
    }
    default:
        return BT_SELF_COMPONENT_STATUS_ERROR;
    }
}

void JsonBuilderSink::PortConnected(
    bt_self_component_port_input* self_port,
    const bt_port_output* other_port)
{
    constexpr std::array<bt_notification, 2> c_types = {
        { BT_NOTIFICATION_TYPE_EVENT, BT_NOTIFICATION_TYPE_SENTINEL }
    };

    BabelPtr<bt_private_connection> connection =
        bt_private_port_get_private_connection(self_port);
    FAIL_FAST_IF(
        bt_private_connection_create_notification_iterator(
            connection.Get(), c_types.data(), &_notifIter) !=
        BT_CONNECTION_STATUS_OK);
}

bt_self_component_status
JsonBuilderSink::PersistEvent(bt_notification* eventNotif)
{
    LttngJsonReader reader;
    JsonBuilder builder = reader.DecodeEvent(eventNotif);

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
    bt_self_component_sink_add_input_port(self, "in", nullptr, nullptr);

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

void JsonBuilderSink_InputPortConnectedStatic(
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

BtComponentClassSinkConstPtr GetJsonBuilderSinkComponentClass()
{
    BtComponentClassSinkPtr jsonBuilderSinkClass =
        bt_component_class_sink_create("jsonbuilder", JsonBuilderSink_RunStatic);
    bt_component_class_sink_set_init_method(
        jsonBuilderSinkClass.Get(), JsonBuilderSink_InitStatic);
    bt_component_class_sink_set_input_port_connected_method(
        jsonBuilderSinkClass.Get(), JsonBuilderSink_InputPortConnectedStatic);
    bt_component_class_sink_set_finalize_method(
        jsonBuilderSinkClass.Get(), JsonBuilderSink_FinalizeStatic);

    // TODO: Change cast to method call
    BtComponentClassSinkConstPtr returnVal =
        reinterpret_cast<const bt_component_class_sink*>(
            jsonBuilderSinkClass.Detach());

    return returnVal;
}

}
