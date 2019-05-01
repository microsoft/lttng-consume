// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <iostream>

#define FAIL_FAST_IF(expr)               \
    if ((expr))                          \
    {                                    \
        std::cerr << #expr << std::endl; \
        std::terminate();                \
    }