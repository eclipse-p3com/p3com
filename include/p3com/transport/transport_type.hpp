// Copyright 2023 NXP

#ifndef P3COM_TRANSPORT_TYPE_HPP
#define P3COM_TRANSPORT_TYPE_HPP

#include "p3com/generic/config.hpp"

#include <cstdint>

namespace iox
{
namespace p3com
{
/**
 * @brief Transport type
 * 
 */
enum struct TransportType
{
    // PCIe transport layer
    PCIE = 1,
    // UDP transport layer
    UDP = 2,
    // TCP transport layer
    TCP = 3,
    // None
    NONE = 4
};

static_assert(static_cast<uint32_t>(TransportType::NONE) == TRANSPORT_TYPE_COUNT, "");

/**
 * @brief Get index of transport
 * 
 * @param typeTransport 
 * @return uint32_t 
 */
inline uint32_t index(const TransportType typeTransport) noexcept
{
    return static_cast<uint32_t>(typeTransport);
}

/**
 * @brief Get type of transport
 * 
 * @param index 
 * @return TransportType
 */
inline TransportType type(const uint32_t index) noexcept
{
    return static_cast<TransportType>(index);
}

constexpr std::array<const char*, TRANSPORT_TYPE_COUNT> TRANSPORT_TYPE_NAMES{{"PCIE", "UDP", "TCP"}};

// List of gateway types which have the potential to lose messages during transfer
constexpr std::array<TransportType, 2U> LOSSY_TRANSPORT_TYPES{TransportType::UDP, TransportType::TCP};

} // namespace p3com
} // namespace iox

#endif
