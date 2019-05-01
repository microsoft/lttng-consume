// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <chrono>
#include <functional>
#include <memory>

#include <jsonbuilder/JsonBuilder.h>
#include <nonstd/string_view.hpp>

namespace LttngConsume {

class LttngConsumerImpl;

class LttngConsumer
{
  public:
    LttngConsumer(
        nonstd::string_view listeningUrl,
        std::chrono::milliseconds pollInterval);

    ~LttngConsumer();

    void StartConsuming(std::function<void(jsonbuilder::JsonBuilder&&)> callback);

    void StopConsuming();

  private:
    std::unique_ptr<LttngConsumerImpl> _impl;
};

}
