// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "LttngJsonReader.h"

#include <bitset>
#include <chrono>

#include <babeltrace2/babeltrace.h>
#include <jsonbuilder/JsonBuilder.h>

#include "BabelPtr.h"
#include "FailureHelpers.h"

using namespace jsonbuilder;

namespace LttngConsume {

void AddField(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field);

void AddTimestamp(JsonBuilder& builder, const bt_clock_snapshot* clock)
{
    int64_t nanosFromEpoch = 0;
    bt_clock_snapshot_get_ns_from_origin_status clockStatus =
        bt_clock_snapshot_get_ns_from_origin(clock, &nanosFromEpoch);
    FAIL_FAST_IF(clockStatus != BT_CLOCK_SNAPSHOT_GET_NS_FROM_ORIGIN_STATUS_OK);

    auto nanos = std::chrono::nanoseconds{ nanosFromEpoch };
    std::chrono::system_clock::time_point eventTimestamp{ nanos };

    builder.push_back(builder.root(), "time", eventTimestamp);
}

void AddEventName(
    JsonBuilder& builder,
    JsonIterator metadataItr,
    const bt_event_class* eventClass)
{
    std::string eventName{ bt_event_class_get_name(eventClass) };

    builder.push_back(metadataItr, "lttngName", eventName);

    // Replace the : separating provider and eventname with .
    std::replace(eventName.begin(), eventName.end(), ':', '.');

    // Parse keywords out of event name
    std::string_view eventNameView{ eventName };
    auto leadingSemicolonPos = eventNameView.find(';');
    if (leadingSemicolonPos != std::string_view::npos)
    {
        eventName.resize(leadingSemicolonPos);

        // Parse ';k;' or ';k0;k2;k19;'
        std::bitset<64> keywords = 0;
        while (leadingSemicolonPos < eventNameView.size() - 1)
        {
            auto nextSemicolonPos =
                eventNameView.find(';', leadingSemicolonPos + 1);
            FAIL_FAST_IF(nextSemicolonPos == std::string_view::npos);

            FAIL_FAST_IF(eventNameView[leadingSemicolonPos + 1] != 'k');

            size_t diffSemicolonPos = nextSemicolonPos - leadingSemicolonPos;
            FAIL_FAST_IF(diffSemicolonPos < 2);
            FAIL_FAST_IF(diffSemicolonPos > 4);

            // ';kX;' or ';kXY;'
            if (diffSemicolonPos >= 3)
            {
                char ch = eventNameView[leadingSemicolonPos + 2];
                FAIL_FAST_IF(ch < '0' || ch > '9');

                // [0, 63] final value
                int keywordBit = ch - '0';

                // ';kXY;' only
                if (diffSemicolonPos == 4)
                {
                    // Previously parsed value was actually tens digit
                    keywordBit *= 10;

                    ch = eventNameView[leadingSemicolonPos + 3];
                    FAIL_FAST_IF(ch < '0' || ch > '9');

                    keywordBit += (ch - '0');
                }

                FAIL_FAST_IF(keywordBit < 0);
                FAIL_FAST_IF(keywordBit > 63);

                keywords.set(keywordBit);
            }

            leadingSemicolonPos = nextSemicolonPos;
        }

        builder.push_back(metadataItr, "keywords", keywords.to_ullong());
    }

    builder.push_back(builder.root(), "name", eventName);
}

void AddFieldBool(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    bool val = bt_field_bool_get_value(field) == BT_TRUE;
    builder.push_back(itr, fieldName, val);
}

void AddFieldBitArray(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    uint64_t val = bt_field_bit_array_get_value_as_integer(field);
    builder.push_back(itr, fieldName, val);
}

void AddFieldSignedInteger(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    int64_t val = bt_field_integer_signed_get_value(field);
    builder.push_back(itr, fieldName, val);
}

void AddFieldUnsignedInteger(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    uint64_t val = bt_field_integer_unsigned_get_value(field);
    builder.push_back(itr, fieldName, val);
}

void AddFieldFloat(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    float val = bt_field_real_single_precision_get_value(field);
    builder.push_back(itr, fieldName, val);
}

void AddFieldDouble(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    double val = bt_field_real_double_precision_get_value(field);
    builder.push_back(itr, fieldName, val);
}

void AddFieldSignedEnum(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    const char* const* labels = nullptr;
    uint64_t labelsCount = 0;
    bt_field_enumeration_signed_get_mapping_labels(field, &labels, &labelsCount);

    if (labelsCount > 0)
    {
        builder.push_back(itr, fieldName, labels[0]);
    }
    else
    {
        int64_t val = bt_field_integer_signed_get_value(field);
        builder.push_back(itr, fieldName, std::to_string(val));
    }
}

void AddFieldUnsignedEnum(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    const char* const* labels = nullptr;
    uint64_t labelsCount = 0;
    bt_field_enumeration_unsigned_get_mapping_labels(field, &labels, &labelsCount);

    if (labelsCount > 0)
    {
        builder.push_back(itr, fieldName, labels[0]);
    }
    else
    {
        uint64_t val = bt_field_integer_unsigned_get_value(field);
        builder.push_back(itr, fieldName, std::to_string(val));
    }
}

void AddFieldString(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    const char* val = bt_field_string_get_value(field);
    uint64_t len = bt_field_string_get_length(field);
    builder.push_back(
        itr, fieldName, std::string_view{ val, static_cast<size_t>(len) });
}

void AddFieldStruct(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    auto structItr = builder.push_back(itr, fieldName, JsonObject);

    const bt_field_class* fieldClass = bt_field_borrow_class_const(field);
    uint64_t numFields = bt_field_class_structure_get_member_count(fieldClass);

    for (uint64_t i = 0; i < numFields; i++)
    {
        const bt_field_class_structure_member* structFieldClass =
            bt_field_class_structure_borrow_member_by_index_const(fieldClass, i);

        const char* structFieldName =
            bt_field_class_structure_member_get_name(structFieldClass);

        const bt_field* structField =
            bt_field_structure_borrow_member_field_by_index_const(field, i);

        AddField(builder, structItr, structFieldName, structField);
    }
}

void AddFieldArray(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    auto arrayItr = builder.push_back(itr, fieldName, JsonArray);

    uint64_t numElements = bt_field_array_get_length(field);
    for (uint64_t i = 0; i < numElements; i++)
    {
        const bt_field* elementField =
            bt_field_array_borrow_element_field_by_index_const(field, i);

        AddField(builder, arrayItr, {}, elementField);
    }
}

void AddFieldOption(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    const bt_field* optionData = bt_field_option_borrow_field_const(field);
    if (optionData)
    {
        AddField(builder, itr, fieldName, field);
    }
}

void AddFieldVariant(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    const bt_field_class* fieldClass = bt_field_borrow_class_const(field);

    const bt_field* selectedOptionField =
        bt_field_variant_borrow_selected_option_field_const(field);

    uint64_t variantSubfieldIndex =
        bt_field_variant_get_selected_option_index(field);

    const bt_field_class_variant_option* variantSubfieldClass =
        bt_field_class_variant_borrow_option_by_index_const(
            fieldClass, variantSubfieldIndex);

    const char* optionName =
        bt_field_class_variant_option_get_name(variantSubfieldClass);

    std::string variantFieldName = nonstd::to_string(fieldName);
    variantFieldName += "_";
    variantFieldName += optionName;

    AddField(builder, itr, variantFieldName, selectedOptionField);
}

bool StartsWith(std::string_view str, std::string_view queryPrefix)
{
    if (str.size() < queryPrefix.size())
    {
        return false;
    }

    return str.substr(0, queryPrefix.size()) == queryPrefix;
}

bool EndsWith(std::string_view str, std::string_view querySuffix)
{
    if (str.size() < querySuffix.size())
    {
        return false;
    }

    return str.substr(str.size() - querySuffix.size()) == querySuffix;
}

void AddField(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    std::string_view fieldName,
    const bt_field* field)
{
    // Skip added '_foo_sequence_field_length' type fields
    if (StartsWith(fieldName, "_") && EndsWith(fieldName, "_length"))
    {
        return;
    }

    bt_field_class_type fieldType = bt_field_get_class_type(field);
    switch (fieldType)
    {
    case BT_FIELD_CLASS_TYPE_BOOL:
        AddFieldBool(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_BIT_ARRAY:
        AddFieldBitArray(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER:
        AddFieldUnsignedInteger(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_SIGNED_INTEGER:
        AddFieldSignedInteger(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION:
        AddFieldUnsignedEnum(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION:
        AddFieldSignedEnum(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_SINGLE_PRECISION_REAL:
        AddFieldFloat(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_DOUBLE_PRECISION_REAL:
        AddFieldDouble(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_STRING:
        AddFieldString(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_STRUCTURE:
        AddFieldStruct(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_STATIC_ARRAY:
    case BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY_WITHOUT_LENGTH_FIELD:
    case BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY_WITH_LENGTH_FIELD:
        AddFieldArray(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_OPTION_WITHOUT_SELECTOR_FIELD:
    case BT_FIELD_CLASS_TYPE_OPTION_WITH_BOOL_SELECTOR_FIELD:
    case BT_FIELD_CLASS_TYPE_OPTION_WITH_UNSIGNED_INTEGER_SELECTOR_FIELD:
    case BT_FIELD_CLASS_TYPE_OPTION_WITH_SIGNED_INTEGER_SELECTOR_FIELD:
        AddFieldOption(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_VARIANT_WITHOUT_SELECTOR_FIELD:
    case BT_FIELD_CLASS_TYPE_VARIANT_WITH_UNSIGNED_INTEGER_SELECTOR_FIELD:
    case BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_INTEGER_SELECTOR_FIELD:
        AddFieldVariant(builder, itr, fieldName, field);
        break;
    default:
        FAIL_FAST_IF(true);
    }
}

void AddPacketContext(JsonBuilder& builder, const bt_event* event)
{
    const bt_packet* packet = bt_event_borrow_packet_const(event);
    const bt_field* packetContext = bt_packet_borrow_context_field_const(packet);
    AddFieldStruct(builder, builder.root(), "packetContext", packetContext);
}

void AddEventHeader(JsonBuilder& builder, const bt_event* event)
{
    const bt_packet* packet = bt_event_borrow_packet_const(event);
    const bt_stream* stream = bt_packet_borrow_stream_const(packet);
    const bt_trace* trace = bt_stream_borrow_trace_const(stream);

    auto itr = builder.push_back(builder.root(), "eventHeader", JsonObject);

    {
        const char* traceName = bt_trace_get_name(trace);
        if (!traceName)
        {
            traceName = "Unknown";
        }

        builder.push_back(itr, "trace", traceName);

        uint64_t count = bt_trace_get_environment_entry_count(trace);
        for (uint64_t i = 0; i < count; i++)
        {
            const char* name = nullptr;
            const bt_value* val = nullptr;
            bt_trace_borrow_environment_entry_by_index_const(
                trace, i, &name, &val);

            // TODO: Add these values to the jsonBuilder
        }
    }
}

void AddStreamEventContext(JsonBuilder& builder, const bt_event* event)
{
    const bt_field* streamEventContext =
        bt_event_borrow_common_context_field_const(event);

    if (streamEventContext)
    {
        AddFieldStruct(
            builder, builder.root(), "streamEventContext", streamEventContext);
    }
}

void AddEventContext(JsonBuilder& builder, const bt_event* event)
{
    const bt_field* eventContext =
        bt_event_borrow_specific_context_field_const(event);

    if (eventContext)
    {
        AddFieldStruct(builder, builder.root(), "eventContext", eventContext);
    }
}

void AddPayload(JsonBuilder& builder, const bt_event* event)
{
    const bt_field* payloadStruct = bt_event_borrow_payload_field_const(event);
    AddFieldStruct(builder, builder.root(), "data", payloadStruct);
}

JsonBuilder LttngJsonReader::DecodeEvent(const bt_message* message)
{
    JsonBuilder builder;

    const bt_event* event = bt_message_event_borrow_event_const(message);
    const bt_event_class* eventClass = bt_event_borrow_class_const(event);

    auto metadataItr = builder.push_back(builder.root(), "metadata", JsonObject);

    AddEventName(builder, metadataItr, eventClass);

    const bt_clock_snapshot* clock =
        bt_message_event_borrow_default_clock_snapshot_const(message);

    AddTimestamp(builder, clock);

    AddPacketContext(builder, event);
    AddEventHeader(builder, event);
    AddStreamEventContext(builder, event);
    AddEventContext(builder, event);
    AddPayload(builder, event);

    return builder;
}
}
