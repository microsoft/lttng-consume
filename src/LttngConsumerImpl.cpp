// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "LttngConsumerImpl.h"

#include <string>
#include <thread>

#include <babeltrace/babeltrace.h>
#include <lttng-consume/LttngConsumer.h>

#include "BabelPtr.h"
#include "FailureHelpers.h"
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

static void CheckValue(bt_value_status status)
{
    switch (status)
    {
    case BT_VALUE_STATUS_OK:
        return;
    case BT_VALUE_STATUS_NOMEM:
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

    BabelPtr<const bt_plugin> ctfPlugin = bt_plugin_find("ctf");

    const bt_component_class_source* lttngLiveClass =
        bt_plugin_borrow_source_component_class_by_name_const(
            ctfPlugin.Get(), "lttng-live");

    BabelPtr<bt_value> paramsMap = bt_value_map_create();

    CheckValue(bt_value_map_insert_string_entry(
        paramsMap.Get(), "url", std::string{ _listeningUrl }.c_str()));

    CheckGraph(bt_graph_add_source_component(
        _graph.Get(),
        lttngLiveClass,
        "liveInput",
        paramsMap.Get(),
        &_lttngLiveSource));

    // Create filter component
    BabelPtr<const bt_plugin> utilsPlugin = bt_plugin_find("utils");

    const bt_component_class_filter* muxerClass =
        bt_plugin_borrow_filter_component_class_by_name_const(
            utilsPlugin.Get(), "muxer");

    CheckGraph(bt_graph_add_filter_component(
        _graph.Get(), muxerClass, "muxer", nullptr, &_muxerFilter));

    // Create sink component
    BabelPtr<const bt_component_class_sink> jsonBuilderSinkClass =
        GetJsonBuilderSinkComponentClass();

    JsonBuilderSinkInitParams jbInitParams;
    jbInitParams.OutputFunc = &callback;
    CheckGraph(bt_graph_add_sink_component_with_init_method_data(
        _graph.Get(),
        jsonBuilderSinkClass.Get(),
        "jsonbuildersinkinst",
        nullptr,
        &jbInitParams,
        &_jsonBuilderSink));

    CheckGraph(bt_graph_add_source_component_output_port_added_listener(
        _graph.Get(),
        SourceComponentOutputPortAddedListenerStatic,
        nullptr,
        this,
        nullptr));

    // Wire up existing ports
    const bt_port_output* lttngLiveSourceOutputPort =
        bt_component_source_borrow_output_port_by_name_const(
            _lttngLiveSource.Get(), "out");
    const bt_port_input* muxerFilterInputPort =
        bt_component_filter_borrow_input_port_by_name_const(
            _muxerFilter.Get(), "in0");

    const bt_port_output* muxerFilterOutputPort =
        bt_component_filter_borrow_output_port_by_name_const(
            _muxerFilter.Get(), "out");
    const bt_port_input* jsonBuilderSinkInputPort =
        bt_component_sink_borrow_input_port_by_name_const(
            _jsonBuilderSink.Get(), "in");

    CheckGraph(bt_graph_connect_ports(
        _graph.Get(), lttngLiveSourceOutputPort, muxerFilterInputPort, nullptr));
    CheckGraph(bt_graph_connect_ports(
        _graph.Get(), muxerFilterOutputPort, jsonBuilderSinkInputPort, nullptr));
}

void LttngConsumerImpl::SourceComponentOutputPortAddedListenerStatic(
    const bt_component_source* component,
    const bt_port_output* port,
    void* data)
{
    static_cast<LttngConsumerImpl*>(data)->SourceComponentOutputPortAddedListener(
        component, port);
}

void LttngConsumerImpl::SourceComponentOutputPortAddedListener(
    const bt_component_source* component,
    const bt_port_output* port)
{
    FAIL_FAST_IF(component != _lttngLiveSource.Get());

    int64_t muxerInputPortCount =
        bt_component_filter_get_input_port_count(_muxerFilter.Get());
    FAIL_FAST_IF(muxerInputPortCount < 0);

    for (int64_t i = 0; i < muxerInputPortCount; i++)
    {
        const bt_port_input* downstreamPort =
            bt_component_filter_borrow_input_port_by_index_const(
                _muxerFilter.Get(), i);

        if (!bt_port_is_connected(bt_port_input_as_port_const(downstreamPort)))
        {
            CheckGraph(bt_graph_connect_ports(
                _graph.Get(), port, downstreamPort, nullptr));
            return;
        }
    }

    FAIL_FAST_IF(true);
}

}
