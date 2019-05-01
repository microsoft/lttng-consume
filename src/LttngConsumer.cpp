// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <lttng-consume/LttngConsumer.h>

#include "LttngConsumerImpl.h"

namespace LttngConsume {
LttngConsumer::LttngConsumer(
    nonstd::string_view listeningUrl,
    std::chrono::milliseconds pollInterval)
{
    _impl.reset(new LttngConsumerImpl(listeningUrl, pollInterval));
}

LttngConsumer::~LttngConsumer() = default;

void LttngConsumer::StartConsuming(
    std::function<void(jsonbuilder::JsonBuilder&&)> callback)
{
    _impl->StartConsuming(callback);
}

void LttngConsumer::StopConsuming()
{
    _impl->StopConsuming();
}
}