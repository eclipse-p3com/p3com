// Copyright 2023 NXP

#ifndef P3COM_TRANSPORT_FORWARDER_HPP
#define P3COM_TRANSPORT_FORWARDER_HPP

#include "p3com/generic/config.hpp"
#include "p3com/generic/discovery.hpp"
#include "p3com/generic/pending_messages.hpp"
#include "p3com/utility/vector_map.hpp"

#include "iceoryx_posh/capro/service_description.hpp"

namespace iox
{
namespace p3com
{
class TransportForwarder
{
  public:
    TransportForwarder(
        DiscoveryManager& discovery,
        PendingMessageManager& pendingMessageManager,
        const cxx::vector<capro::ServiceDescription, MAX_FORWARDED_SERVICES>& forwardedServices) noexcept;

    TransportForwarder(const TransportForwarder&) = delete;
    TransportForwarder(TransportForwarder&&) = delete;
    TransportForwarder& operator=(const TransportForwarder&) = delete;
    TransportForwarder& operator=(TransportForwarder&&) = delete;
    ~TransportForwarder() = default;

    void push(const void* userPayload, capro::ServiceDescription::ClassHash hash, DeviceIndex_t deviceIndex) noexcept;

    void join() noexcept;

  private:
    void waitsetLoop() noexcept;

    DiscoveryManager& m_discovery;
    PendingMessageManager& m_pendingMessageManager;

    cxx::vector<capro::ServiceDescription::ClassHash, MAX_FORWARDED_SERVICES> m_forwardedServiceHashes;

    std::mutex m_forwardedServiceSubscribersMutex;
    cxx::vector<popo::UntypedSubscriber, MAX_FORWARDED_SERVICES> m_forwardedServiceSubscribers;

    std::mutex m_messagesToForwardMutex;
    cxx::vector_map<const void*, DeviceIndex_t, MAX_FORWARDED_SERVICES> m_messagesToForward;

    popo::WaitSet<MAX_FORWARDED_SERVICES> m_waitset;

    std::atomic<bool> m_terminateFlag;
    std::thread m_waitsetThread;
};

} // namespace p3com
} // namespace iox

#endif
