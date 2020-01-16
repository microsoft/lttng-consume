// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <atomic>
#include <chrono>
#include <functional>

#include <nonstd/string_view.hpp>

#include "BabelPtr.h"

namespace jsonbuilder {
class JsonBuilder;
}

namespace LttngConsume {

class LttngConsumerImpl
{
  public:
    LttngConsumerImpl(
        nonstd::string_view listeningUrl,
        std::chrono::milliseconds pollInterval);

    void StartConsuming(std::function<void(jsonbuilder::JsonBuilder&&)> callback);

    void StopConsuming();

  private:
    static bt_graph_listener_func_status
    SourceComponentOutputPortAddedListenerStatic(
        const bt_component_source* component,
        const bt_port_output* port,
        void* data);

    void CreateGraph(std::function<void(jsonbuilder::JsonBuilder&&)>& callback);

    bt_graph_listener_func_status SourceComponentOutputPortAddedListener(
        const bt_component_source* component,
        const bt_port_output* port);

  private:
    std::string _listeningUrl;
    std::chrono::milliseconds _pollInterval;
    std::atomic<bool> _stopConsuming;

    BabelPtr<bt_graph> _graph;
    BabelPtr<const bt_component_source> _lttngLiveSource;
    BabelPtr<const bt_component_filter> _muxerFilter;
    BabelPtr<const bt_component_sink> _jsonBuilderSink;
};

}
