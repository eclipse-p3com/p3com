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
