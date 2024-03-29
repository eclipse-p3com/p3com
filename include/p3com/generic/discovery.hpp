// Copyright 2023 NXP

#ifndef P3COM_DISCOVERY_HPP
#define P3COM_DISCOVERY_HPP

#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"
#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/popo/publisher.hpp"

#include "p3com/generic/types.hpp"
#include "p3com/introspection/gw_introspection_types.hpp"
#include "p3com/gateway/gateway_config.hpp"
#include "p3com/utility/vector_map.hpp"
#include "p3com/transport/transport.hpp"

#include <cstdint>
#include <mutex>
#include <map>

namespace iox
{
namespace p3com
{
struct LocalState_t
{
    ServiceVector_t userPublishers;
    ServiceVector_t userSubscribers;
    cxx::vector<uint64_t, MAX_TOPICS> userPublisherPorts;
};

struct DeviceRecord_t
{
    PubSubInfo_t info;
    cxx::vector<DeviceIndex_t, TRANSPORT_TYPE_COUNT> deviceIndices;
};

using DeviceIndexVector_t = cxx::vector<DeviceIndex_t, MAX_DEVICE_COUNT>;

struct RemoteState_t
{
    cxx::vector<DeviceRecord_t, MAX_DEVICE_COUNT> records;
    cxx::vector_map<capro::ServiceDescription::ClassHash, DeviceIndexVector_t, MAX_TOPICS> deviceIndicesCache;
};

class DiscoveryManager
{
  public:
    explicit DiscoveryManager(const GatewayConfig_t& config) noexcept;
    ~DiscoveryManager() = default;

    DiscoveryManager(const DiscoveryManager&) = delete;
    DiscoveryManager(DiscoveryManager&&) = delete;
    DiscoveryManager& operator=(const DiscoveryManager&) = delete;
    DiscoveryManager& operator=(DiscoveryManager&&) = delete;

    void initialize(cxx::function_ref<void(const ServiceVector_t&)> updateCallback) noexcept;
    void deinitialize() noexcept;

    void addGatewayPublisher(const popo::UniquePortId& uid) noexcept;
    void discardGatewayPublisher(const popo::UniquePortId& uid) noexcept;

    const DeviceIndexVector_t& generateDeviceIndices(const popo::UniquePortId& uid,
                                                   const capro::ServiceDescription::ClassHash& serviceHash) noexcept;

    DeviceIndexVector_t generateDeviceIndicesForwarding(const capro::ServiceDescription::ClassHash& serviceHash,
                                                      DeviceIndex_t fromDeviceIndex) noexcept;

    void resendDiscoveryInfoToTransport(TransportType type) noexcept;

  private:
    DeviceIndexVector_t computeDeviceIndices(const capro::ServiceDescription::ClassHash& serviceHash) const
        noexcept;

    PubSubInfo_t generateDiscoveryInfo() noexcept;
    static void sendDiscoveryInfo(const PubSubInfo_t& info) noexcept;

    void waitsetLoop() noexcept;
    void readPortSubscriber() noexcept;

    void receiveRemoteDiscoveryInfo(const void* serializedData, size_t size, DeviceIndex_t deviceIndex) noexcept;
    ServiceVector_t updateNeededChannels() const noexcept;

    const hash_t m_gatewayHash;
    const TransportType m_preferredType;

    mutable std::recursive_mutex m_mutex;
    cxx::function_ref<void(const ServiceVector_t&)> m_updateCallback;

    popo::Subscriber<roudi::PortIntrospectionFieldTopic> m_portSubscriber;

    popo::Publisher<GwRegisteredData> m_gwIntrospectionPublisher;

    RemoteState_t m_remoteState;
    LocalState_t m_localState;

    // Store local gateway publisher UIDs
    cxx::vector<popo::UniquePortId, MAX_PUBLISHERS> m_gatewayPublisherUids;
    // Store last sent discovery info
    PubSubInfo_t m_lastSentDiscoveryInfo;

    popo::WaitSet<1U> m_waitset;
    std::atomic<bool> m_terminateFlag;
    std::thread m_waitsetThread;
};

} // namespace p3com
} // namespace iox

#endif // P3COM_DISCOVERY_HPP
