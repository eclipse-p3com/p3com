/*****************************************************************************
 *
 * Copyright 2022 NXP
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only
 * be used strictly in accordance with the applicable license terms. By
 * expressly accepting such terms or by downloading, installing, activating
 * and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If
 * you do not agree to be bound by the applicable license terms, then you may
 * not retain, install, activate or otherwise use the software.
 *
 ****************************************************************************/

#include "p3com/generic/serialization.hpp"

uint32_t iox::p3com::serialize(const iox::p3com::PubSubInfo_t& info, char* ptr) noexcept
{
    size_t offset = 0U;
    const auto pushPrimitive = [ptr, &offset](auto elem) noexcept {
        std::memcpy(ptr + offset, &elem, sizeof(elem));
        offset += sizeof(elem);
    };
    const auto pushString = [ptr, &offset](const auto& str) noexcept {
        const uint64_t size = str.size();
        std::memcpy(ptr + offset, str.c_str(), size + 1); // include null-character
        offset += size + 1;
    };

    // We dont care about endianness for now, since all supported platforms are little endian.
    // We also assume that all primitives (integer types) have a fixed size (e.g., use uint32_t instead of unsigned
    // int).
    pushPrimitive(static_cast<uint64_t>(info.userSubscribers.size()));

    for (const auto& service : info.userSubscribers)
    {
        for (const auto& str :
             {service.getServiceIDString(), service.getInstanceIDString(), service.getEventIDString()})
        {
            pushString(str);
        }
    }
    pushPrimitive(info.gatewayBitset);
    pushPrimitive(info.gatewayHash);
    pushPrimitive(info.infoHash);
    pushPrimitive(info.isTermination);

    iox::cxx::Expects(offset <= maxPubSubInfoSerializationSize());
    return static_cast<uint32_t>(offset);
}

uint32_t iox::p3com::deserialize(iox::p3com::PubSubInfo_t& info, const char* ptr, size_t size) noexcept
{
    size_t offset = 0U;
    const auto loadPrimitive = [ptr, &offset, size](auto* elem) noexcept {
        iox::cxx::Expects(offset + sizeof(*elem) <= size);
        std::memcpy(elem, ptr + offset, sizeof(*elem));
        offset += sizeof(*elem);
    };
    const auto loadString = [ptr, &offset, size](auto& str) noexcept {
        const bool success = str.unsafe_assign(ptr + offset);
        iox::cxx::Expects(success);
        iox::cxx::Expects(offset + str.size() + 1 <= size);
        offset += str.size() + 1;
    };

    uint64_t userSubscribersSize;
    loadPrimitive(&userSubscribersSize);
    iox::cxx::Expects(userSubscribersSize <= info.userSubscribers.capacity());
    info.userSubscribers.resize(userSubscribersSize);

    iox::capro::IdString_t serviceStr;
    iox::capro::IdString_t instanceStr;
    iox::capro::IdString_t eventStr;
    for (auto& service : info.userSubscribers)
    {
        loadString(serviceStr);
        loadString(instanceStr);
        loadString(eventStr);
        service = capro::ServiceDescription{serviceStr, instanceStr, eventStr};
    }
    loadPrimitive(&info.gatewayBitset);
    loadPrimitive(&info.gatewayHash);
    loadPrimitive(&info.infoHash);
    loadPrimitive(&info.isTermination);

    iox::cxx::Expects(offset <= maxPubSubInfoSerializationSize());
    return static_cast<uint32_t>(offset);
}

uint32_t iox::p3com::serialize(const iox::p3com::IoxChunkDatagramHeader_t& datagramHeader, char* ptr) noexcept
{
    size_t offset = 0U;
    const auto pushPrimitive = [ptr, &offset](auto elem) noexcept {
        std::memcpy(ptr + offset, &elem, sizeof(elem));
        offset += sizeof(elem);
    };

    for (uint32_t i = 0U; i < iox::capro::CLASS_HASH_ELEMENT_COUNT; ++i)
    {
        pushPrimitive(datagramHeader.serviceHash[i]);
    }
    pushPrimitive(datagramHeader.messageHash);
    pushPrimitive(datagramHeader.submessageCount);
    pushPrimitive(datagramHeader.submessageOffset);
    pushPrimitive(datagramHeader.submessageSize);
    pushPrimitive(datagramHeader.userPayloadSize);
    pushPrimitive(datagramHeader.userPayloadAlignment);
    pushPrimitive(datagramHeader.userHeaderSize);

    iox::cxx::Expects(offset <= maxIoxChunkDatagramHeaderSerializationSize());
    return static_cast<uint32_t>(offset);
}

uint32_t iox::p3com::deserialize(iox::p3com::IoxChunkDatagramHeader_t& datagramHeader, const char* ptr, size_t size) noexcept
{
    size_t offset = 0U;
    const auto loadPrimitive = [ptr, &offset, size](auto* elem) noexcept {
        iox::cxx::Expects(offset + sizeof(*elem) <= size);
        std::memcpy(elem, ptr + offset, sizeof(*elem));
        offset += sizeof(*elem);
    };

    for (uint32_t i = 0U; i < iox::capro::CLASS_HASH_ELEMENT_COUNT; ++i)
    {
        loadPrimitive(&datagramHeader.serviceHash[i]);
    }
    loadPrimitive(&datagramHeader.messageHash);
    loadPrimitive(&datagramHeader.submessageCount);
    loadPrimitive(&datagramHeader.submessageOffset);
    loadPrimitive(&datagramHeader.submessageSize);
    loadPrimitive(&datagramHeader.userPayloadSize);
    loadPrimitive(&datagramHeader.userPayloadAlignment);
    loadPrimitive(&datagramHeader.userHeaderSize);

    iox::cxx::Expects(offset <= maxIoxChunkDatagramHeaderSerializationSize());
    return static_cast<uint32_t>(offset);
}
