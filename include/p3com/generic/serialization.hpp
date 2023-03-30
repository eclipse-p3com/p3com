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

#ifndef P3COM_SERIALIZATION_HPP
#define P3COM_SERIALIZATION_HPP

#include "p3com/generic/types.hpp"

namespace iox
{
namespace p3com
{
constexpr inline uint32_t maxPubSubInfoSerializationSize() noexcept
{
    size_t total_size = 0U;

    total_size += sizeof(uint64_t); // Number of topics for userSubscribers

    // Each string is serialized as its c_str, with null-character
    constexpr uint32_t SERVICE_DESCRIPTION_SER_SIZE = 3 * (capro::IdString_t::capacity() + 1);
    total_size += MAX_TOPICS * SERVICE_DESCRIPTION_SER_SIZE;

    total_size += sizeof(bitset_t); // gatewayBitset
    total_size += sizeof(hash_t);   // gatewayHash
    total_size += sizeof(hash_t);   // infoHash
    total_size += sizeof(bool);     // isTermination

    return static_cast<uint32_t>(total_size);
}

constexpr inline uint32_t maxIoxChunkDatagramHeaderSerializationSize() noexcept
{
    size_t total_size = 0U;

    total_size += capro::CLASS_HASH_ELEMENT_COUNT * sizeof(uint32_t); // serviceHash
    total_size += sizeof(hash_t);                                     // messageHash
    total_size += sizeof(uint32_t);                                   // submessageCount
    total_size += sizeof(uint32_t);                                   // submessageOffset
    total_size += sizeof(uint32_t);                                   // submessageSize
    total_size += sizeof(uint32_t);                                   // userPayloadSize
    total_size += sizeof(uint32_t);                                   // userPayloadAlignment
    total_size += sizeof(uint32_t);                                   // userHeaderSize

    return static_cast<uint32_t>(total_size);
}


uint32_t serialize(const PubSubInfo_t& info, char* ptr) noexcept;
uint32_t deserialize(PubSubInfo_t& info, const char* ptr, size_t size) noexcept;

uint32_t serialize(const IoxChunkDatagramHeader_t& datagramHeader, char* ptr) noexcept;
uint32_t deserialize(IoxChunkDatagramHeader_t& datagramHeader, const char* ptr, size_t size) noexcept;

} // namespace p3com
} // namespace iox

#endif
