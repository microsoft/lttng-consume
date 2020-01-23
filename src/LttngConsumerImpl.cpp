// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "LttngConsumerImpl.h"

#include <string>
#include <thread>

#include <babeltrace2/babeltrace.h>
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

    bt_graph_run_status status;
    while ((status = bt_graph_run(_graph.Get())) == BT_GRAPH_RUN_STATUS_AGAIN &&
           !_stopConsuming)
    {
        std::this_thread::sleep_for(_pollInterval);
    }

    if (status != BT_GRAPH_RUN_STATUS_AGAIN)
    {
        std::cerr << "Final graph status: " << status << std::endl;
    }
    FAIL_FAST_IF(status != BT_GRAPH_RUN_STATUS_AGAIN);
}

void LttngConsumerImpl::StopConsuming()
{
    _stopConsuming = true;
}

static void CheckBtError(int32_t status)
{
    switch (status)
    {
    case BT_GRAPH_RUN_STATUS_OK:
        return;
    case BT_GRAPH_RUN_STATUS_MEMORY_ERROR:
        throw std::bad_alloc();
    default:
        std::terminate();
    }
}

void LttngConsumerImpl::CreateGraph(
    std::function<void(jsonbuilder::JsonBuilder&&)>& callback)
{
    bt_logging_set_global_level(BT_LOGGING_LEVEL_WARNING);

    _graph = bt_graph_create(0);

    BabelPtr<const bt_plugin> ctfPlugin;

    bt_plugin_find_status pluginFindStatus = bt_plugin_find(
        "ctf", BT_FALSE, BT_FALSE, BT_TRUE, BT_FALSE, BT_TRUE, &ctfPlugin);
    CheckBtError(pluginFindStatus);

    const bt_component_class_source* lttngLiveClass =
        bt_plugin_borrow_source_component_class_by_name_const(
            ctfPlugin.Get(), "lttng-live");

    BabelPtr<bt_value> urlArray = bt_value_array_create();
    CheckBtError(bt_value_array_append_string_element(
        urlArray.Get(), std::string{ _listeningUrl }.c_str()));

    BabelPtr<bt_value> paramsMap = bt_value_map_create();
    CheckBtError(
        bt_value_map_insert_entry(paramsMap.Get(), "inputs", urlArray.Get()));
    CheckBtError(bt_value_map_insert_string_entry(
        paramsMap.Get(), "session-not-found-action", "continue"));

    CheckBtError(bt_graph_add_source_component(
        _graph.Get(),
        lttngLiveClass,
        "liveInput",
        paramsMap.Get(),
        BT_LOGGING_LEVEL_WARNING,
        &_lttngLiveSource));

    // Create filter component
    BabelPtr<const bt_plugin> utilsPlugin;

    pluginFindStatus = bt_plugin_find(
        "utils", BT_FALSE, BT_FALSE, BT_TRUE, BT_FALSE, BT_TRUE, &utilsPlugin);
    CheckBtError(pluginFindStatus);

    const bt_component_class_filter* muxerClass =
        bt_plugin_borrow_filter_component_class_by_name_const(
            utilsPlugin.Get(), "muxer");

    CheckBtError(bt_graph_add_filter_component(
        _graph.Get(),
        muxerClass,
        "muxer",
        nullptr,
        BT_LOGGING_LEVEL_WARNING,
        &_muxerFilter));

    // Create sink component
    BabelPtr<const bt_component_class_sink> jsonBuilderSinkClass =
        GetJsonBuilderSinkComponentClass();

    JsonBuilderSinkInitParams jbInitParams;
    jbInitParams.OutputFunc = &callback;

    CheckBtError(bt_graph_add_sink_component_with_initialize_method_data(
        _graph.Get(),
        jsonBuilderSinkClass.Get(),
        "jsonbuildersinkinst",
        nullptr,
        &jbInitParams,
        BT_LOGGING_LEVEL_INFO,
        &_jsonBuilderSink));

    CheckBtError(bt_graph_add_source_component_output_port_added_listener(
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

    CheckBtError(bt_graph_connect_ports(
        _graph.Get(), lttngLiveSourceOutputPort, muxerFilterInputPort, nullptr));
    CheckBtError(bt_graph_connect_ports(
        _graph.Get(), muxerFilterOutputPort, jsonBuilderSinkInputPort, nullptr));
}

bt_graph_listener_func_status
LttngConsumerImpl::SourceComponentOutputPortAddedListenerStatic(
    const bt_component_source* component,
    const bt_port_output* port,
    void* data)
{
    return static_cast<LttngConsumerImpl*>(data)
        ->SourceComponentOutputPortAddedListener(component, port);
}

bt_graph_listener_func_status
LttngConsumerImpl::SourceComponentOutputPortAddedListener(
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
            CheckBtError(bt_graph_connect_ports(
                _graph.Get(), port, downstreamPort, nullptr));
            return BT_GRAPH_LISTENER_FUNC_STATUS_OK;
        }
    }

    FAIL_FAST_IF(true);
}

}
