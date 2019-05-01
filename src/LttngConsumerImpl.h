// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <atomic>
#include <chrono>
#include <functional>

#include <nonstd/string_view.hpp>

#include "BabelPtr.h"

struct bt_component;
struct bt_graph;
struct bt_port;

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
    static void PortAddedListenerStatic(bt_port* port, void* data);

    void CreateGraph(std::function<void(jsonbuilder::JsonBuilder&&)>& callback);

    void PortAddedListener(bt_port* port);

  private:
    std::string _listeningUrl;
    std::chrono::milliseconds _pollInterval;
    std::atomic<bool> _stopConsuming;

    BabelPtr<bt_graph> _graph;
    BabelPtr<bt_component> _lttngLiveSource;
    BabelPtr<bt_component> _muxerFilter;
    BabelPtr<bt_component> _jsonBuilderSink;
};

}
