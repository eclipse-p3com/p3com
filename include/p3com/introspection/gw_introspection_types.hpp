// Copyright 2023 NXP

#ifndef P3COM_GW_INTROSPECTION_TYPES_HPP
#define P3COM_GW_INTROSPECTION_TYPES_HPP

#include "iceoryx_posh/roudi/introspection_types.hpp"
#include "p3com/generic/config.hpp"

namespace iox
{
namespace p3com
{
/**
 * @brief List of publisher IDs that are registered with the p3com gateway
 */
using GwRegisteredPublisherPortData = cxx::vector<uint64_t, MAX_TOPICS>;

const capro::ServiceDescription
    IntrospectionGwService(roudi::INTROSPECTION_SERVICE_ID, "RouDi_ID", "RegisteredPublishers");

} // namespace p3com
} // namespace iox

#endif
