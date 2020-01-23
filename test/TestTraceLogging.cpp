// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <chrono>
#include <iostream>
#include <thread>
#include <unistd.h>

#include <catch2/catch.hpp>
#include <jsonbuilder/JsonRenderer.h>
#include <lttng-consume/LttngConsumer.h>
#include <tracelogging/TraceLoggingProvider.h>

TRACELOGGING_DEFINE_PROVIDER(
    g_provider,
    "MyTestProvider",
    (0xb3864c38, 0x4273, 0x58c5, 0x54, 0x5b, 0x8b, 0x36, 0x08, 0x34, 0x34, 0x71));
TRACELOGGING_DEFINE_PROVIDER(
    g_providerKeywords,
    "MyTestProviderKeywords",
    (0xb3864c38, 0x4273, 0x58c5, 0x54, 0x5b, 0x8b, 0x36, 0x08, 0x34, 0x34, 0x72));

using namespace jsonbuilder;

static std::string MakeConnectionString(nonstd::string_view sessionName)
{
    std::string result = "net://localhost/host/";

    char hostnameBuf[256];
    gethostname(hostnameBuf, 256);

    result += hostnameBuf;
    result += "/";
    result += nonstd::to_string(sessionName);

    return result;
}

void RunConsumerTraceLogging(LttngConsume::LttngConsumer& consumer, int& renderCount)
{
    consumer.StartConsuming([&renderCount](JsonBuilder&& jsonBuilder) {
        JsonRenderer renderer;
        renderer.Pretty(true);

        nonstd::string_view jsonString = renderer.Render(jsonBuilder);
        REQUIRE(!jsonString.empty());

        auto itr = jsonBuilder.find("name");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonUtf8);
        REQUIRE(
            itr->GetUnchecked<nonstd::string_view>() ==
            "MyTestProvider.MyTestEvent");

        itr = jsonBuilder.find("time");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonTime);
        REQUIRE(
            itr->GetUnchecked<std::chrono::system_clock::time_point>() <=
            std::chrono::system_clock::now());

        itr = jsonBuilder.find("data");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonObject);

        REQUIRE(jsonBuilder.count(itr) == 2);

        // TODO (nichbop): Promote this
        itr = jsonBuilder.find("data", "_ms_ActivityId");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonArray);

        itr = jsonBuilder.find("data", "CountedString");
        REQUIRE(itr != jsonBuilder.end());
        REQUIRE(itr->Type() == JsonUtf8);
        REQUIRE(itr->GetUnchecked<nonstd::string_view>() == "Banana");

        renderCount++;
    });
}

TEST_CASE("LttngConsumer TraceLogging does not crash", "[consumer]")
{
    system("lttng destroy lttngconsume-tracelogging");
    system("lttng create lttngconsume-tracelogging --live");
    system(
        "lttng enable-event -s lttngconsume-tracelogging --userspace MyTestProvider:*");
    system(
        "lttng add-context -s lttngconsume-tracelogging -u -t procname -t vpid");
    system("lttng start lttngconsume-tracelogging");

    std::this_thread::sleep_for(std::chrono::seconds{ 1 });

    std::string connectionString =
        MakeConnectionString("lttngconsume-tracelogging");

    LttngConsume::LttngConsumer consumer{ connectionString,
                                          std::chrono::milliseconds{ 50 } };

    constexpr int c_eventsToFire = 1;

    int eventCallbacks = 0;
    std::thread consumptionThread{ RunConsumerTraceLogging,
                                   std::ref(consumer),
                                   std::ref(eventCallbacks) };

    TraceLoggingRegister(g_provider);

    std::string val = "Banana";
    TraceLoggingWrite(
        g_provider,
        "MyTestEvent",
        TraceLoggingCountedString(val.data(), val.size(), "CountedString"));

    TraceLoggingUnregister(g_provider);

    std::this_thread::sleep_for(std::chrono::seconds{ 2 });

    consumer.StopConsuming();
    consumptionThread.join();

    REQUIRE(eventCallbacks == c_eventsToFire);
}

struct KeywordTestValue
{
    std::string OriginalName;
    std::string ParsedName;
    uint64_t Keywords;
};

void RunConsumerTestKeywords(
    LttngConsume::LttngConsumer& consumer,
    const std::vector<KeywordTestValue>& keywordTestValues,
    int& renderCount)
{
    consumer.StartConsuming(
        [&keywordTestValues, &renderCount](JsonBuilder&& jsonBuilder) {
            JsonRenderer renderer;
            renderer.Pretty(true);

            nonstd::string_view jsonString = renderer.Render(jsonBuilder);
            REQUIRE(!jsonString.empty());

            auto itr = jsonBuilder.find("name");
            REQUIRE(itr != jsonBuilder.end());
            REQUIRE(itr->Type() == JsonUtf8);
            REQUIRE(
                itr->GetUnchecked<nonstd::string_view>() ==
                keywordTestValues[renderCount].ParsedName);

            auto metadataItr = jsonBuilder.find("metadata");
            REQUIRE(metadataItr->Type() == JsonObject);

            itr = jsonBuilder.find(metadataItr, "lttngName");
            REQUIRE(itr != jsonBuilder.end());
            REQUIRE(itr->Type() == JsonUtf8);
            REQUIRE(
                itr->GetUnchecked<nonstd::string_view>() ==
                keywordTestValues[renderCount].OriginalName);

            uint64_t keywordVal = 0;
            itr = jsonBuilder.find(metadataItr, "keywords");
            REQUIRE(itr->Type() == JsonUInt);
            REQUIRE(itr->ConvertTo(keywordVal));
            REQUIRE(keywordVal == keywordTestValues[renderCount].Keywords);

            renderCount++;
        });
}

TEST_CASE("LttngConsumer parses keywords", "[consumer]")
{
    system("lttng destroy lttngconsume-tracelogging-keywords");
    system("lttng create lttngconsume-tracelogging-keywords --live");
    system(
        "lttng enable-event -s lttngconsume-tracelogging-keywords --userspace MyTestProviderKeywords:*");
    system(
        "lttng add-context -s lttngconsume-tracelogging-keywords -u -t procname -t vpid");
    system("lttng start lttngconsume-tracelogging-keywords");

    std::this_thread::sleep_for(std::chrono::seconds{ 1 });

    std::string connectionString =
        MakeConnectionString("lttngconsume-tracelogging-keywords");

    LttngConsume::LttngConsumer consumer{ connectionString,
                                          std::chrono::milliseconds{ 50 } };

    constexpr uint64_t highestBit = 0x1ull << 63;

    std::vector<KeywordTestValue> nameKeywordPairs = {
        { "MyTestProviderKeywords:NoKeywords;k;",
          "MyTestProviderKeywords.NoKeywords",
          0 },
        { "MyTestProviderKeywords:OneKeywordMinValue;k0;",
          "MyTestProviderKeywords.OneKeywordMinValue",
          1 },
        { "MyTestProviderKeywords:OneKeywordMaxValue;k63;",
          "MyTestProviderKeywords.OneKeywordMaxValue",
          highestBit },
        { "MyTestProviderKeywords:ManyKeywords;k0;k7;k58;k60;",
          "MyTestProviderKeywords.ManyKeywords",
          0x1400000000000081ull }
    };

    int eventCallbacks = 0;
    std::thread consumptionThread{ RunConsumerTestKeywords,
                                   std::ref(consumer),
                                   std::cref(nameKeywordPairs),
                                   std::ref(eventCallbacks) };

    TraceLoggingRegister(g_providerKeywords);

    TraceLoggingWrite(g_providerKeywords, "NoKeywords");
    TraceLoggingWrite(
        g_providerKeywords, "OneKeywordMinValue", TraceLoggingKeyword(0x1));
    TraceLoggingWrite(
        g_providerKeywords, "OneKeywordMaxValue", TraceLoggingKeyword(highestBit));
    TraceLoggingWrite(
        g_providerKeywords,
        "ManyKeywords",
        TraceLoggingKeyword(0x1400000000000081ull));

    TraceLoggingUnregister(g_providerKeywords);

    std::this_thread::sleep_for(std::chrono::seconds{ 2 });

    consumer.StopConsuming();
    consumptionThread.join();

    REQUIRE(eventCallbacks == nameKeywordPairs.size());
}