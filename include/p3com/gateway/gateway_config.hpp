// Copyright 2023 NXP

#ifndef P3COM_GATEWAY_CONFIG_HPP
#define P3COM_GATEWAY_CONFIG_HPP

#include "p3com/generic/types.hpp"

namespace iox
{
namespace p3com
{
struct GatewayConfig_t
{
    TransportType preferredTransport{TransportType::NONE};
    cxx::vector<capro::ServiceDescription, MAX_FORWARDED_SERVICES> forwardedServices;
};

class TomlGatewayConfigParser
{
  public:
    static GatewayConfig_t parse(roudi::ConfigFilePathString_t path) noexcept;
};

} // namespace p3com
} // namespace iox

#endif
