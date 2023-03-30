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

#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/service_discovery.hpp"
#include "iceoryx_hoofs/internal/units/duration.hpp"

#include "p3com/introspection/gw_introspection_types.hpp"
#include "p3com/introspection/gw_introspection.hpp"

bool iox::p3com::isGwRunning() noexcept
{
    bool found = false;
    iox::runtime::ServiceDiscovery serviceDiscovery;
    serviceDiscovery.findService(
        iox::p3com::IntrospectionGwService.getServiceIDString(),
        iox::p3com::IntrospectionGwService.getInstanceIDString(),
        iox::p3com::IntrospectionGwService.getEventIDString(),
        [&found](const iox::capro::ServiceDescription&) { found = true; },
        iox::popo::MessagingPattern::PUB_SUB);
    return found;
}

bool iox::p3com::hasGwRegistered(iox::popo::uid_t publisherUid) noexcept
{
    iox::popo::Subscriber<iox::p3com::GwRegisteredPublisherPortData> sub{iox::p3com::IntrospectionGwService, {1U, 1U}};
    while (sub.getSubscriptionState() != SubscribeState::SUBSCRIBED)
    {
    }

    bool hasRegistered = false;
    sub.take().and_then([publisherUid, &hasRegistered](auto& ports) {
        const auto it = std::find(ports->begin(), ports->end(), static_cast<uint64_t>(publisherUid));
        hasRegistered = it != ports->end();
    });

    // Need to wait for unsubscription to avoid exhausting subscriber ports
    sub.unsubscribe();
    while (sub.getSubscriptionState() != SubscribeState::NOT_SUBSCRIBED)
    {
    }
    return hasRegistered;
}

bool iox::p3com::waitForGwRegistration(iox::popo::uid_t publisherUid, iox::units::Duration timeout) noexcept
{
    iox::popo::Subscriber<iox::p3com::GwRegisteredPublisherPortData> sub{iox::p3com::IntrospectionGwService, {1U, 1U}};
    while (sub.getSubscriptionState() != SubscribeState::SUBSCRIBED)
    {
    }

    iox::popo::WaitSet<> waitset;
    const bool attached = static_cast<bool>(waitset.attachState(sub, iox::popo::SubscriberState::HAS_DATA));
    if (!attached)
    {
        return false;
    }

    bool hasRegistered = false;
    auto now = std::chrono::steady_clock::now();
    const auto end = now + std::chrono::nanoseconds{timeout.toNanoseconds()};
    while (!hasRegistered && now < end)
    {
        const auto remaining = std::chrono::duration_cast<std::chrono::nanoseconds>(end - now);
        auto notificationVector = waitset.timedWait(iox::units::Duration::fromNanoseconds(remaining.count()));
        if (notificationVector.empty())
        {
            // Timeout
            break;
        }
        for (auto& notification : notificationVector)
        {
            if (notification->doesOriginateFrom(&sub))
            {
                sub.take().and_then([publisherUid, &hasRegistered](auto& ports) {
                    const auto it = std::find(ports->begin(), ports->end(), static_cast<uint64_t>(publisherUid));
                    hasRegistered = it != ports->end();
                });
                if (hasRegistered)
                {
                    break;
                }
            }
        }
        now = std::chrono::steady_clock::now();
    }

    // Need to wait for unsubscription to avoid exhausting subscriber ports
    sub.unsubscribe();
    while (sub.getSubscriptionState() != SubscribeState::NOT_SUBSCRIBED)
    {
    }
    return hasRegistered;
}
