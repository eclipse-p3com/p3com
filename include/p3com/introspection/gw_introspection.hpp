// Copyright 2023 NXP

#ifndef P3COM_GW_INTROSPECTION_HPP
#define P3COM_GW_INTROSPECTION_HPP

#include <chrono>

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_hoofs/internal/units/duration.hpp"

namespace iox
{
namespace p3com
{
bool isGwRunning() noexcept;

bool hasGwRegistered(popo::uid_t publisherUid) noexcept;

bool waitForGwRegistration(popo::uid_t publisherUid, units::Duration timeout = units::Duration::max()) noexcept;

bool hasServiceRegistered(const capro::ServiceDescription& service) noexcept;

bool waitForServiceRegistration(const capro::ServiceDescription& service,
                                units::Duration timeout = units::Duration::max()) noexcept;

} // namespace p3com
} // namespace iox

#endif
