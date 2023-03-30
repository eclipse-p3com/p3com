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

#ifndef P3COM_GATEWAY_HPP
#define P3COM_GATEWAY_HPP

#include "p3com/generic/config.hpp"
#include "p3com/generic/types.hpp"
#include "p3com/utility/vector_map.hpp"

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_hoofs/cxx/function_ref.hpp"

#include <memory>
#include <mutex>

namespace iox
{
namespace p3com
{
template <typename Endpoint>
class Gateway
{
  protected:
    /**
     * @brief Update channels to match the given list of services
     *
     * @param services
     * @param setupFn
     * @param deleteFn
     */
    void updateChannelsInternal(const ServiceVector_t& services,
                                cxx::function_ref<void(const capro::ServiceDescription&)> setupFn,
                                cxx::function_ref<void(const capro::ServiceDescription&)> deleteFn) noexcept;

    /**
     * @brief Function to be executed *after* the channel creation
     *
     * @tparam PubSubOptions
     * @param service
     * @param options
     * @param f
     */
    template <typename PubSubOptions>
    void addChannel(const capro::ServiceDescription& service,
                    const PubSubOptions& options,
                    cxx::function_ref<void(Endpoint&)> f) noexcept;

    /**
     * @brief Function to be executed *before* the channel deletion
     *
     * @param service
     * @param f
     */
    void discardChannel(const capro::ServiceDescription& service, cxx::function_ref<void(Endpoint&)> f) noexcept;

    /**
     * @brief Execute a function for a particular endpoint
     *
     * @param hash
     * @param f
     */
    void doForChannel(capro::ServiceDescription::ClassHash hash, cxx::function_ref<void(Endpoint&)> f) noexcept;

    /**
     * @brief Get endpoints mutex
     *
     * @return 
     */
    std::mutex& endpointsMutex() noexcept;

  private:
    /**
     * @brief Mutex protecting m_endpoints.
     */
    std::mutex m_endpointsMutex;

    /**
     * @brief Vector of endpoints
     * @note We have to use std::unique_ptr since popo::UntypedPublisher and
     * popo::UntypedSubscriber dont have move ctor and move assignment
     * operator.
     */
    cxx::vector_map<capro::ServiceDescription::ClassHash, std::unique_ptr<Endpoint>, MAX_TOPICS> m_endpoints;
};

} // namespace p3com
} // namespace iox

#include "p3com/internal/gateway/gateway.inl"

#endif
