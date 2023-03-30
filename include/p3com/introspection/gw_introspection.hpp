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

#ifndef P3COM_GW_INTROSPECTION_HPP
#define P3COM_GW_INTROSPECTION_HPP

#include <chrono>

#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_hoofs/internal/units/duration.hpp"

namespace iox
{
namespace p3com
{
bool isGwRunning() noexcept;

bool hasGwRegistered(popo::uid_t publisherUid) noexcept;

bool waitForGwRegistration(popo::uid_t publisherUid, units::Duration timeout = units::Duration::max()) noexcept;

} // namespace p3com
} // namespace iox

#endif
