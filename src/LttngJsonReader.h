// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <jsonbuilder/JsonBuilder.h>

struct bt_message;

namespace LttngConsume {

class LttngJsonReader
{
  public:
    jsonbuilder::JsonBuilder DecodeEvent(const bt_message* message);
};

}
