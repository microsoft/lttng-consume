// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "LttngConsumerImpl.h"

#include <string>
#include <thread>

#include <lttng-consume/LttngConsumer.h>

// clang-format off
#include <babeltrace/ctf-ir/field-types.h>
#include <babeltrace/babeltrace.h>
// clang-format on

#include "FailureHelpers.h"
#include "BabelPtr.h"
#include "JsonBuilderSink.h"

namespace LttngConsume {

LttngConsumerImpl::LttngConsumerImpl(
    nonstd::string_view listeningUrl,
    std::chrono::milliseconds pollInterval)
    : _listeningUrl(listeningUrl)
    , _pollInterval(pollInterval)
    , _stopConsuming(false)
{}

void LttngConsumerImpl::StartConsuming(
    std::function<void(jsonbuilder::JsonBuilder&&)> callback)
{
    CreateGraph(callback);

    bt_graph_status status;
    while ((status = bt_graph_run(_graph.Get())) == BT_GRAPH_STATUS_AGAIN &&
           !_stopConsuming)
    {
        std::this_thread::sleep_for(_pollInterval);
    }

    if (status != BT_GRAPH_STATUS_AGAIN)
    {
        std::cerr << "Final graph status: " << status << std::endl;
    }
    FAIL_FAST_IF(status != BT_GRAPH_STATUS_AGAIN);
}

void LttngConsumerImpl::StopConsuming()
{
    _stopConsuming = true;
}

static void CheckGraph(bt_graph_status status)
{
    switch (status)
    {
    case BT_GRAPH_STATUS_OK:
        return;
    case BT_GRAPH_STATUS_NOMEM:
        throw std::bad_alloc();
    default:
        std::terminate();
    }
}

void LttngConsumerImpl::CreateGraph(
    std::function<void(jsonbuilder::JsonBuilder&&)>& callback)
{
    bt_logging_set_global_level(BT_LOGGING_LEVEL_WARN);

    _graph = bt_graph_create();

    // Create source component
    BabelPtr<bt_component_class> lttngLiveClass = bt_plugin_find_component_class(
        "ctf",
        "lttng-live",
        bt_component_class_type::BT_COMPONENT_CLASS_TYPE_SOURCE);

    BabelPtr<bt_value> paramsMap = bt_value_map_create();
    bt_value_map_insert_string(
        paramsMap.Get(), "url", std::string{ _listeningUrl }.c_str());

    CheckGraph(bt_graph_add_component(
        _graph.Get(),
        lttngLiveClass.Get(),
        "liveInput",
        paramsMap.Get(),
        &_lttngLiveSource));

    // Create filter component
    BabelPtr<bt_component_class> muxerClass = bt_plugin_find_component_class(
        "utils", "muxer", bt_component_class_type::BT_COMPONENT_CLASS_TYPE_FILTER);
    CheckGraph(bt_graph_add_component(
        _graph.Get(), muxerClass.Get(), "muxer", nullptr, &_muxerFilter));

    // Create sink component
    BabelPtr<bt_component_class> jsonBuilderSinkClass =
        GetJsonBuilderSinkComponentClass();

    JsonBuilderSinkInitParams jbInitParams;
    jbInitParams.OutputFunc = &callback;
    CheckGraph(bt_graph_add_component_with_init_method_data(
        _graph.Get(),
        jsonBuilderSinkClass.Get(),
        "jsonbuildersinkinst",
        nullptr,
        &jbInitParams,
        &_jsonBuilderSink));

    FAIL_FAST_IF(
        bt_graph_add_port_added_listener(
            _graph.Get(), LttngConsumerImpl::PortAddedListenerStatic, nullptr, this) <
        0);

    // Wire up existing ports
    BabelPtr<bt_port> lttngLiveSourceOutputPort =
        bt_component_source_get_output_port_by_name(
            _lttngLiveSource.Get(), "no-stream");
    BabelPtr<bt_port> muxerFilterInputPort =
        bt_component_filter_get_input_port_by_name(_muxerFilter.Get(), "in0");

    BabelPtr<bt_port> muxerFilterOutputPort =
        bt_component_filter_get_output_port_by_name(_muxerFilter.Get(), "out");
    BabelPtr<bt_port> jsonBuilderSinkInputPort =
        bt_component_sink_get_input_port_by_name(_jsonBuilderSink.Get(), "in");

    CheckGraph(bt_graph_connect_ports(
        _graph.Get(),
        lttngLiveSourceOutputPort.Get(),
        muxerFilterInputPort.Get(),
        nullptr));
    CheckGraph(bt_graph_connect_ports(
        _graph.Get(),
        muxerFilterOutputPort.Get(),
        jsonBuilderSinkInputPort.Get(),
        nullptr));
}

void LttngConsumerImpl::PortAddedListenerStatic(bt_port* port, void* data)
{
    static_cast<LttngConsumerImpl*>(data)->PortAddedListener(port);
}

void LttngConsumerImpl::PortAddedListener(bt_port* port)
{
    BabelPtr<bt_component> portOwningComponent = bt_port_get_component(port);
    if (portOwningComponent == _lttngLiveSource)
    {
        int64_t muxerInputPortCount =
            bt_component_filter_get_input_port_count(_muxerFilter.Get());
        FAIL_FAST_IF(muxerInputPortCount < 0);

        for (int64_t i = 0; i < muxerInputPortCount; i++)
        {
            BabelPtr<bt_port> downstreamPort =
                bt_component_filter_get_input_port_by_index(_muxerFilter.Get(), i);

            if (!bt_port_is_connected(downstreamPort.Get()))
            {
                CheckGraph(bt_graph_connect_ports(
                    _graph.Get(), port, downstreamPort.Get(), nullptr));
                return;
            }
        }

        FAIL_FAST_IF(true);
    }
}

}
