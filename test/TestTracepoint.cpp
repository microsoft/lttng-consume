// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <chrono>
#include <iostream>
#include <thread>
#include <unistd.h>

#include <catch2/catch.hpp>
#include <jsonbuilder/JsonRenderer.h>
#include <lttng-consume/LttngConsumer.h>

#include "Test-Tracepoint.h"

using namespace jsonbuilder;

void RunConsumer(LttngConsume::LttngConsumer& consumer, int& renderCount)
{
    consumer.StartConsuming([&renderCount](JsonBuilder&& jsonBuilder) {
        JsonRenderer renderer;
        renderer.Pretty(true);

        std::string_view jsonString = renderer.Render(jsonBuilder);
        REQUIRE(!jsonString.empty());

        auto itr = jsonBuilder.find("name");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonUtf8);
        REQUIRE(
            itr->GetUnchecked<std::string_view>() ==
            "hello_world.my_first_tracepoint");

        itr = jsonBuilder.find("time");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonTime);
        REQUIRE(
            itr->GetUnchecked<std::chrono::system_clock::time_point>() <=
            std::chrono::system_clock::now());

        itr = jsonBuilder.find("packetContext");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonObject);

        itr = jsonBuilder.find("eventHeader");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonObject);

        itr = jsonBuilder.find("streamEventContext");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonObject);

        itr = jsonBuilder.find("streamEventContext", "procname");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonUtf8);
        REQUIRE(!itr->GetUnchecked<std::string_view>().empty());

        itr = jsonBuilder.find("streamEventContext", "vpid");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonInt);
        REQUIRE(itr->GetUnchecked<int>() > 0);

        itr = jsonBuilder.find("data");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonObject);

#ifdef TRACEPOINT_ENUM
        unsigned int expectedFieldCount = 9;
#else
        unsigned int expectedFieldCount = 8;
#endif
        REQUIRE(jsonBuilder.count(itr) == expectedFieldCount);

        itr = jsonBuilder.find("data", "my_string_field");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonUtf8);
        REQUIRE(
            itr->GetUnchecked<std::string_view>() == std::to_string(renderCount));

        itr = jsonBuilder.find("data", "my_integer_field");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonInt);
        REQUIRE(itr->GetUnchecked<int>() == renderCount);

        itr = jsonBuilder.find("data", "my_unsigned_integer_field");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonUInt);
        REQUIRE(itr->GetUnchecked<unsigned int>() == renderCount);

        itr = jsonBuilder.find("data", "my_float_field");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonFloat);
        REQUIRE(itr->GetUnchecked<float>() == renderCount);

#ifdef TRACEPOINT_ENUM
        const std::string_view c_enumVals[] = {
            "ZERO", "ONE", "TWO", "THREEFOUR", "THREEFOUR"
        };

        itr = jsonBuilder.find("data", "my_enum_field");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonUtf8);
        REQUIRE(
            itr->GetUnchecked<std::string_view>() ==
            (renderCount < 5 ? std::string{ c_enumVals[renderCount] } :
                               std::to_string(renderCount)));
#endif

        itr = jsonBuilder.find("data", "my_int_array_field");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonArray);
        REQUIRE(jsonBuilder.count(itr) == 3);

        int i = 0;
        for (auto valItr = itr.begin(); valItr != itr.end(); ++valItr, ++i)
        {
            REQUIRE(valItr != jsonBuilder.end());
            REQUIRE(valItr->Type() == JsonInt);
            REQUIRE(valItr->GetUnchecked<int>() == i);
        }
        REQUIRE(i == 3);

        itr = jsonBuilder.find("data", "my_int_seq_field");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonArray);
        REQUIRE(jsonBuilder.count(itr) == (renderCount % 3));

        i = 0;
        for (auto valItr = itr.begin(); valItr != itr.end(); ++valItr, ++i)
        {
            REQUIRE(valItr != jsonBuilder.end());
            REQUIRE(valItr->Type() == JsonInt);
            REQUIRE(valItr->GetUnchecked<int>() == i);
        }
        REQUIRE(i == (renderCount % 3));

        itr = jsonBuilder.find("data", "my_char_array_text_field");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonUtf8);
        REQUIRE(itr->GetUnchecked<std::string_view>() == "abcde");

        itr = jsonBuilder.find("data", "my_char_seq_text_field");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonUtf8);
        REQUIRE(
            itr->GetUnchecked<std::string_view>() ==
            std::string_view{ "abcde" }.substr(0, renderCount % 5));

        renderCount++;
    });
}

static std::string MakeConnectionString(std::string_view sessionName)
{
    std::string result = "net://localhost/host/";

    char hostnameBuf[256];
    gethostname(hostnameBuf, 256);

    result += hostnameBuf;
    result += "/";
    result += sessionName;

    return result;
}

TEST_CASE("LttngConsumer callbacks happen", "[consumer]")
{
    system("lttng destroy lttngconsume-tracepoint");
    system("lttng create lttngconsume-tracepoint --live");
    system(
        "lttng enable-event -s lttngconsume-tracepoint --userspace hello_world:*");
    system("lttng add-context -s lttngconsume-tracepoint -u -t procname -t vpid");
    system("lttng start lttngconsume-tracepoint");

    std::this_thread::sleep_for(std::chrono::seconds{ 1 });

    std::string connectionString =
        MakeConnectionString("lttngconsume-tracepoint");

    LttngConsume::LttngConsumer consumer{ connectionString,
                                          std::chrono::milliseconds{ 50 } };

    constexpr int c_eventsToFire = 250;

    int eventCallbacks = 0;
    std::thread consumptionThread{ RunConsumer,
                                   std::ref(consumer),
                                   std::ref(eventCallbacks) };

    constexpr int c_intArray[] = { 0, 1, 2 };
    constexpr char c_charArray[] = { 'a', 'b', 'c', 'd', 'e' };

    int i = 0;
    tracepoint(
        hello_world,
        my_first_tracepoint,
        i,
        std::to_string(i).c_str(),
        c_intArray,
        c_charArray);
    tracepoint(
        hello_world,
        my_first_tracepoint2,
        i,
        std::to_string(i).c_str(),
        c_intArray,
        c_charArray);

    std::this_thread::sleep_for(std::chrono::milliseconds{ 2 });

    std::this_thread::sleep_for(std::chrono::seconds{ 2 });

    consumer.StopConsuming();
    consumptionThread.join();

    REQUIRE(eventCallbacks == c_eventsToFire);
}
