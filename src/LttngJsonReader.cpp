// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "LttngJsonReader.h"

#include <chrono>

// clang-format off
#include <babeltrace/babeltrace.h>
// clang-format on

#include <jsonbuilder/JsonBuilder.h>

#include "BabelPtr.h"
#include "FailureHelpers.h"

using namespace jsonbuilder;

namespace LttngConsume {

void AddField(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
    const bt_field* field);

void AddTimestamp(JsonBuilder& builder, const bt_clock_snapshot* clock)
{
    int64_t nanosFromEpoch = 0;
    bt_clock_snapshot_status clockStatus =
        bt_clock_snapshot_get_ns_from_origin(clock, &nanosFromEpoch);
    FAIL_FAST_IF(clockStatus != BT_CLOCK_SNAPSHOT_STATUS_OK);

    auto nanos = std::chrono::nanoseconds{ nanosFromEpoch };
    std::chrono::system_clock::time_point eventTimestamp{ nanos };

    builder.push_back(builder.end(), "time", eventTimestamp);
}

void AddEventName(JsonBuilder& builder, const bt_event_class* eventClass)
{
    std::string eventName{ bt_event_class_get_name(eventClass) };
    std::replace(eventName.begin(), eventName.end(), ':', '.');
    builder.push_back(builder.end(), "name", eventName);
}

void AddFieldSignedInteger(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
    const bt_field* field)
{
    int64_t val = bt_field_signed_integer_get_value(field);
    builder.push_back(itr, fieldName, val);
}

void AddFieldUnsignedInteger(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
    const bt_field* field)
{
    uint64_t val = bt_field_unsigned_integer_get_value(field);
    builder.push_back(itr, fieldName, val);
}

void AddFieldReal(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
    const bt_field* field)
{
    double val = bt_field_real_get_value(field);
    builder.push_back(itr, fieldName, val);
}

void AddFieldEnum(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
    const bt_field* field)
{
    //     BabelPtr<bt_field_type> fieldType = bt_field_get_type(field);

    //     BabelPtr<bt_field> containerField =
    //     bt_field_enumeration_get_container(field); BabelPtr<bt_field_type>
    //     containerFieldType =
    //         bt_field_get_type(containerField.Get());

    //     int signedResult =
    //         bt_ctf_field_type_integer_get_signed(containerFieldType.Get());
    //     FAIL_FAST_IF(signedResult < 0);

    //     std::string name;
    //     BabelPtr<bt_field_type_enumeration_mapping_iterator> enumItr;
    //     if (signedResult == 0)
    //     {
    //         uint64_t val = 0;
    //         FAIL_FAST_IF(
    //             bt_field_unsigned_integer_get_value(containerField.Get(),
    //             &val)
    //             != 0);

    //         enumItr =
    //         bt_field_type_enumeration_find_mappings_by_unsigned_value(
    //             fieldType.Get(), val);

    //         if
    //         (bt_field_type_enumeration_mapping_iterator_next(enumItr.Get())
    //         < 0)
    //         {
    //             name = std::to_string(val);
    //         }
    //         else
    //         {
    //             const char* namePtr = nullptr;
    //             FAIL_FAST_IF(
    //                 bt_field_type_enumeration_mapping_iterator_get_signed(
    //                     enumItr.Get(), &namePtr, nullptr, nullptr) < 0);
    //             name = namePtr;
    //         }
    //     }
    //     else
    //     {
    //         int64_t val = 0;
    //         FAIL_FAST_IF(
    //             bt_field_signed_integer_get_value(containerField.Get(), &val)
    //             != 0);

    //         enumItr =
    //         bt_field_type_enumeration_find_mappings_by_signed_value(
    //             fieldType.Get(), val);

    //         if
    //         (bt_field_type_enumeration_mapping_iterator_next(enumItr.Get())
    //         < 0)
    //         {
    //             name = std::to_string(val);
    //         }
    //         else
    //         {
    //             const char* namePtr = nullptr;
    //             FAIL_FAST_IF(
    //                 bt_field_type_enumeration_mapping_iterator_get_signed(
    //                     enumItr.Get(), &namePtr, nullptr, nullptr) < 0);
    //             name = namePtr;
    //         }
    //     }

    //     builder.push_back(itr, fieldName, name);
}

void AddFieldString(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
    const bt_field* field)
{
    const char* val = bt_field_string_get_value(field);
    builder.push_back(itr, fieldName, val);
}

void AddFieldStruct(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
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

void AddFieldVariant(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
    const bt_field* field)
{
    // BabelPtr<bt_field> variantField =
    // bt_field_variant_get_current_field(field);

    // // Currently there is no support to list the name of the selected
    // subfield AddField(builder, itr, fieldName, variantField.Get());
}

bool StartsWith(nonstd::string_view str, nonstd::string_view queryPrefix)
{
    if (str.size() < queryPrefix.size())
    {
        return false;
    }

    return str.substr(0, queryPrefix.size()) == queryPrefix;
}

bool EndsWith(nonstd::string_view str, nonstd::string_view querySuffix)
{
    if (str.size() < querySuffix.size())
    {
        return false;
    }

    return str.substr(str.size() - querySuffix.size()) == querySuffix;
}

// bool ShouldTreatAsString(const bt_field_type* elementType)
// {
//     bt_field_type_id elementTypeId = bt_field_type_get_type_id(elementType);

//     if (elementTypeId == BT_FIELD_TYPE_ID_INTEGER)
//     {
//         bt_string_encoding encoding =
//             bt_field_type_integer_get_encoding(elementType);

//         if (encoding == BT_STRING_ENCODING_ASCII ||
//             encoding == BT_STRING_ENCODING_UTF8)
//         {
//             int integerSize = bt_field_type_integer_get_size(elementType);
//             if (integerSize == std::numeric_limits<unsigned char>::digits)
//             {
//                 return true;
//             }
//         }
//     }

//     return false;
// }

// void BuildStringFromCharArrayFields(std::string& stringField, bt_field*
// field)
// {
//     BabelPtr<bt_field_type> fieldType = bt_field_get_type(field);

//     int signedResult =
//     bt_ctf_field_type_integer_get_signed(fieldType.Get());
//     FAIL_FAST_IF(signedResult < 0);

//     char charVal = 0;
//     if (signedResult == 0)
//     {
//         uint64_t val;
//         FAIL_FAST_IF(bt_field_unsigned_integer_get_value(field, &val) <
//         0); charVal = static_cast<char>(val);
//     }
//     else
//     {
//         int64_t val;
//         FAIL_FAST_IF(bt_field_signed_integer_get_value(field, &val) < 0);
//         FAIL_FAST_IF(val > std::numeric_limits<char>::max());
//         FAIL_FAST_IF(val < std::numeric_limits<char>::min());
//         charVal = static_cast<char>(val);
//     }

//     stringField.push_back(charVal);
// }

// void HandleContiguousContainer(
//     JsonBuilder& builder,
//     JsonBuilder::iterator itr,
//     nonstd::string_view fieldName,
//     bt_field* field,
//     int numElements,
//     bt_field_type* elementType,
//     bt_field* (*getFieldFunc)(bt_field* field, uint64_t index))
// {
//     if (ShouldTreatAsString(elementType))
//     {
//         std::string stringField;
//         stringField.reserve(numElements);
//         for (int i = 0; i < numElements; i++)
//         {
//             BabelPtr<bt_field> elementField = getFieldFunc(field, i);
//             BuildStringFromCharArrayFields(stringField,
//             elementField.Get());
//         }

//         // Use .c_str() here to intentionally trim trailing
//         nul-characters builder.push_back(itr, fieldName,
//         stringField.c_str());
//     }
//     else
//     {
//         auto seqItr = builder.push_back(itr, fieldName, JsonArray);
//         for (int i = 0; i < numElements; i++)
//         {
//             BabelPtr<bt_field> elementField = getFieldFunc(field, i);
//             AddField(builder, seqItr, {}, elementField.Get());
//         }
//     }
// }

void AddFieldArray(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
    const bt_field* field)
{
    // BabelPtr<bt_field_type> arrayType = bt_field_get_type(field);

    // int64_t numElements =
    // bt_field_type_array_get_length(arrayType.Get());
    // FAIL_FAST_IF(numElements < 0);

    // BabelPtr<bt_field_type> elementType =
    //     bt_field_type_array_get_element_type(arrayType.Get());

    // HandleContiguousContainer(
    //     builder,
    //     itr,
    //     fieldName,
    //     field,
    //     numElements,
    //     elementType.Get(),
    //     bt_field_array_get_field);
}

void AddFieldSequence(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
    const bt_field* field)
{
    // BabelPtr<bt_field_type> seqType = bt_field_get_type(field);

    // BabelPtr<bt_field> lengthField = bt_field_sequence_get_length(field);

    // uint64_t numElements = 0;
    // FAIL_FAST_IF(
    //     bt_field_unsigned_integer_get_value(lengthField.Get(),
    //     &numElements) < 0);

    // BabelPtr<bt_field_type> elementType =
    //     bt_field_type_sequence_get_element_type(seqType.Get());

    // HandleContiguousContainer(
    //     builder,
    //     itr,
    //     fieldName,
    //     field,
    //     numElements,
    //     elementType.Get(),
    //     bt_field_sequence_get_field);
}

void AddField(
    JsonBuilder& builder,
    JsonBuilder::iterator itr,
    nonstd::string_view fieldName,
    const bt_field* field)
{
    bt_field_class_type fieldType = bt_field_get_class_type(field);

    // Skip added '_foo_sequence_field_length' type fields
    if (StartsWith(fieldName, "_") && EndsWith(fieldName, "_length"))
    {
        return;
    }

    switch (fieldType)
    {
    case BT_FIELD_CLASS_TYPE_SIGNED_INTEGER:
        AddFieldSignedInteger(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER:
        AddFieldUnsignedInteger(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_REAL:
        AddFieldReal(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION:
    case BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION:
        AddFieldEnum(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_STRING:
        AddFieldString(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_STRUCTURE:
        AddFieldStruct(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_VARIANT:
        AddFieldVariant(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_STATIC_ARRAY:
        AddFieldArray(builder, itr, fieldName, field);
        break;
    case BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY:
        AddFieldSequence(builder, itr, fieldName, field);
        break;
    default:
        FAIL_FAST_IF(true);
    }
}

void AddPacketContext(JsonBuilder& builder, const bt_event* event)
{
    const bt_packet* packet = bt_event_borrow_packet_const(event);
    const bt_field* packetContext = bt_packet_borrow_context_field_const(packet);
    AddFieldStruct(builder, builder.end(), "packetContext", packetContext);
}

void AddEventHeader(JsonBuilder& builder, const bt_event* event)
{
    const bt_event_class* eventClass = bt_event_borrow_class_const(event);
    const bt_stream_class* streamClass =
        bt_event_class_borrow_stream_class_const(eventClass);
    const bt_trace_class* traceClass =
        bt_stream_class_borrow_trace_class_const(streamClass);

    const bt_packet* packet = bt_event_borrow_packet_const(event);
    const bt_stream* stream = bt_packet_borrow_stream_const(packet);
    const bt_trace* trace = bt_stream_borrow_trace_const(stream);

    auto itr = builder.push_back(builder.end(), "eventHeader", JsonObject);

    {
        const char* traceName = bt_trace_get_name(trace);
        builder.push_back(itr, "trace", traceName);
    }

    uint64_t count = bt_trace_class_get_environment_entry_count(traceClass);
    for (uint64_t i = 0; i < count; i++)
    {
        const char* name = nullptr;
        const bt_value* val = nullptr;
        bt_trace_class_borrow_environment_entry_by_index_const(
            traceClass, i, &name, &val);

        // TODO: Add these values to the jsonBuilder
    }
}

void AddStreamEventContext(JsonBuilder& builder, const bt_event* event)
{
    const bt_field* streamEventContext =
        bt_event_borrow_common_context_field_const(event);

    if (streamEventContext)
    {
        AddFieldStruct(
            builder, builder.end(), "streamEventContext", streamEventContext);
    }
}

void AddEventContext(JsonBuilder& builder, const bt_event* event)
{
    const bt_field* eventContext =
        bt_event_borrow_specific_context_field_const(event);

    if (eventContext)
    {
        AddFieldStruct(builder, builder.end(), "eventContext", eventContext);
    }
}

void AddPayload(JsonBuilder& builder, const bt_event* event)
{
    const bt_field* payloadStruct = bt_event_borrow_payload_field_const(event);
    AddFieldStruct(builder, builder.end(), "data", payloadStruct);
}

JsonBuilder LttngJsonReader::DecodeEvent(const bt_message* message)
{
    JsonBuilder builder;

    const bt_event* event = bt_message_event_borrow_event_const(message);
    const bt_event_class* eventClass = bt_event_borrow_class_const(event);

    AddEventName(builder, eventClass);

    const bt_clock_snapshot* clock = nullptr;
    bt_clock_snapshot_state clockState =
        bt_message_event_borrow_default_clock_snapshot_const(message, &clock);
    FAIL_FAST_IF(clockState != BT_CLOCK_SNAPSHOT_STATE_KNOWN);

    AddTimestamp(builder, clock);

    AddPacketContext(builder, event);
    // AddEventHeader(builder, event);
    AddStreamEventContext(builder, event);
    AddEventContext(builder, event);
    AddPayload(builder, event);

    return builder;
}
}
