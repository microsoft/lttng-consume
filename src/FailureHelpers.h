// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <iostream>

#define FAIL_FAST_IF(expr)                                                   \
    if ((expr))                                                              \
    {                                                                        \
        std::cerr << "lttng-consume fatal error: expr " << #expr << " file " \
                  << __FILE__ << " line " << __LINE__ << std::endl;          \
        std::terminate();                                                    \
    }
