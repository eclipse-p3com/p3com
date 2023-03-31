// Copyright 2023 NXP

#ifndef P3COM_CONFIG_HPP
#define P3COM_CONFIG_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"

#include <chrono>

namespace iox
{
namespace p3com
{
constexpr std::chrono::milliseconds DISCOVERY_PERIOD{200U};
constexpr std::chrono::milliseconds BROADCAST_SEND_TIMEOUT{50U};
constexpr std::chrono::milliseconds LOSSY_TRANSPORT_DISCOVERY_PERIOD{500U};

const RuntimeName_t IOX_GW_RUNTIME_NAME{"p3com-gw"};
const NodeName_t IOX_GW_NODE_NAME{"p3com-gw"};

#if defined(__FREERTOS__)
constexpr uint32_t MAX_TOPICS{6U};
#else
constexpr uint32_t MAX_TOPICS{32U};
#endif

constexpr uint32_t TRANSPORT_TYPE_COUNT{4U};

#if defined(__FREERTOS___)
constexpr uint32_t MAX_DEVICE_COUNT{2U};
constexpr uint32_t MAX_FORWARDED_SERVICES{0U};
#else
constexpr uint32_t MAX_DEVICE_COUNT{10U};
constexpr uint32_t MAX_FORWARDED_SERVICES{8U};
#endif

constexpr uint32_t USER_HEADER_ALIGNMENT{8U};
constexpr uint32_t MAX_NETWORK_IFACE_COUNT{10U};

} // namespace p3com
} // namespace iox

#endif // P3COM_CONFIG_HPP
