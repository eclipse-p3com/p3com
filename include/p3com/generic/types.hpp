// Copyright 2023 NXP

#ifndef P3COM_TYPES_HPP
#define P3COM_TYPES_HPP

#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_hoofs/cxx/vector.hpp"
#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"

#include "p3com/generic/config.hpp"
#include "p3com/transport/transport_type.hpp"

#include <array>
#include <cstdint>

namespace iox
{
namespace p3com
{
using hash_t = uint32_t;
using bitset_t = std::bitset<64U>; // Force 64 bits even though we only need TRANSPORT_TYPE_COUNT, to be consistent
                                   // between 32bit and 64bit platforms
static_assert(sizeof(bitset_t) == 8U, "");

// List of service
using ServiceVector_t = cxx::vector<capro::ServiceDescription, MAX_TOPICS>;

/**
 * @brief Device index
 */
struct DeviceIndex_t
{
    // Type of transport
    TransportType type;
    // Device number
    uint32_t device;
};

/**
 * @brief Information of publisher and subscriber
 */
struct PubSubInfo_t
{
    // Vector service of user subscribers
    ServiceVector_t userSubscribers;
    // Gateway bitset
    bitset_t gatewayBitset;
    // Gateway hash
    hash_t gatewayHash;
    // Information of hash
    hash_t infoHash;
    // Status termination
    bool isTermination;
};

/**
 * @brief Save data of header for a iox chunk
 */
struct IoxChunkDatagramHeader_t
{
    // Service hash
    capro::ServiceDescription::ClassHash serviceHash;
    // Unique hash of this message, used to group submessages into messages
    hash_t messageHash;
    // Number of submessages that this message consists of
    uint32_t submessageCount;
    // Offset into the data that this message carries
    uint32_t submessageOffset;
    // Data size of this particular submessage
    uint32_t submessageSize;
    // Total payload size of this whole message
    uint32_t userPayloadSize;
    // User payload alignment
    uint32_t userPayloadAlignment;
    // User header size
    uint32_t userHeaderSize;
};

} // namespace p3com
} // namespace iox

#endif // P3COM_TYPES_HPP
