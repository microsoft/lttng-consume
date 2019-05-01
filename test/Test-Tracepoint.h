// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER hello_world

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "Test-Tracepoint.h"

#if !defined(_HELLO_TP_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#    define _HELLO_TP_H

#    include <lttng/tracepoint.h>

// clang-format off

#ifdef TRACEPOINT_ENUM
TRACEPOINT_ENUM(
    hello_world,
    my_enum,
    TP_ENUM_VALUES(
        ctf_enum_value("ZERO", 0)
        ctf_enum_value("ONE", 1)
        ctf_enum_value("TWO", 2)
        ctf_enum_range("THREEFOUR", 3, 4)
        ctf_enum_value("FOUR", 4)
    )
)
#endif

TRACEPOINT_EVENT(
    hello_world,
    my_first_tracepoint,
    TP_ARGS(int, my_integer_arg, const char*, my_string_arg, const int*, my_int_array_arg, const char*, my_char_array_arg),
    TP_FIELDS(
        ctf_string(my_string_field, my_string_arg)
        ctf_integer(int, my_integer_field, my_integer_arg)
        ctf_integer(unsigned int, my_unsigned_integer_field, my_integer_arg)
        ctf_float(double, my_float_field, my_integer_arg)
#ifdef TRACEPOINT_ENUM
        ctf_enum(hello_world, my_enum, int, my_enum_field, my_integer_arg)
#endif
        ctf_array(int, my_int_array_field, my_int_array_arg, 3)
        ctf_sequence(int, my_int_seq_field, my_int_array_arg, unsigned int, (my_integer_arg % 3))
        ctf_array_text(char, my_char_array_text_field, my_char_array_arg, 5)
        ctf_sequence_text(char, my_char_seq_text_field, my_char_array_arg, unsigned int, (my_integer_arg % 5))))
// clang-format on

#endif /* _HELLO_TP_H */

#include <lttng/tracepoint-event.h>