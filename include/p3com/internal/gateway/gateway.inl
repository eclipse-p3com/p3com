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

#ifndef P3COM_INTERNAL_GATEWAY_HPP
#define P3COM_INTERNAL_GATEWAY_HPP

#include "p3com/utility/helper_functions.hpp"
#include "p3com/internal/log/logging.hpp"

template <typename Endpoint>
inline void iox::p3com::Gateway<Endpoint>::updateChannelsInternal(
    const iox::p3com::ServiceVector_t& services,
    iox::cxx::function_ref<void(const iox::capro::ServiceDescription&)> setupFn,
    iox::cxx::function_ref<void(const iox::capro::ServiceDescription&)> deleteFn) noexcept
{
    iox::p3com::ServiceVector_t servicesToDelete;
    iox::p3com::ServiceVector_t servicesToSetup;

    std::lock_guard<std::mutex> lock{m_endpointsMutex};

    for (const auto& endpoint : m_endpoints)
    {
        const auto service = endpoint->getServiceDescription();
        if (!iox::p3com::containsElement(services, service))
        {
            servicesToDelete.push_back(service);
        }
    }

    for (const auto& service : services)
    {
        const auto hash = service.getClassHash();
        auto it = m_endpoints.find(hash);

        if (it == m_endpoints.end())
        {
            servicesToSetup.push_back(service);
        }
    }

    for (const auto& service : servicesToDelete)
    {
        deleteFn(service);
    }
    for (const auto& service : servicesToSetup)
    {
        setupFn(service);
    }
}

template <typename Endpoint>
template <typename PubSubOptions>
inline void iox::p3com::Gateway<Endpoint>::addChannel(const iox::capro::ServiceDescription& service,
                                                    const PubSubOptions& options,
                                                    iox::cxx::function_ref<void(Endpoint&)> f) noexcept
{
    const bool emplaced = m_endpoints.emplace(service.getClassHash(), std::make_unique<Endpoint>(service, options));
    if (!emplaced)
    {
        iox::p3com::LogError() << "[Gateway] Could not add new channel for service: " << service;
    }
    else
    {
        f(*m_endpoints.back());
    }
}


template <typename Endpoint>
inline void iox::p3com::Gateway<Endpoint>::discardChannel(const iox::capro::ServiceDescription& service,
                                                        iox::cxx::function_ref<void(Endpoint&)> f) noexcept
{
    auto it = m_endpoints.find(service.getClassHash());
    if (it == m_endpoints.end())
    {
        iox::p3com::LogError() << "[Gateway] Could not discard channel for service: " << service;
    }
    else
    {
        f(**it);
        m_endpoints.erase(it);
    }
}

template <typename Endpoint>
inline void iox::p3com::Gateway<Endpoint>::doForChannel(iox::capro::ServiceDescription::ClassHash hash,
                                                      iox::cxx::function_ref<void(Endpoint&)> f) noexcept
{
    std::lock_guard<std::mutex> lock{m_endpointsMutex};

    auto it = m_endpoints.find(hash);
    if (it != m_endpoints.end())
    {
        f(**it);
    }
}

template <typename Endpoint>
inline std::mutex& iox::p3com::Gateway<Endpoint>::endpointsMutex() noexcept
{
    return m_endpointsMutex;
}

#endif
