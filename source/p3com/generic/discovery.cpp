// Copyright 2023 NXP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/internal/capro/capro_message.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"

#include "p3com/gateway/gateway_config.hpp"
#include "p3com/generic/config.hpp"
#include "p3com/generic/discovery.hpp"
#include "p3com/generic/serialization.hpp"
#include "p3com/generic/types.hpp"
#include "p3com/internal/log/logging.hpp"
#include "p3com/transport/transport_info.hpp"
#include "p3com/utility/helper_functions.hpp"

#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>

iox::p3com::DiscoveryManager::DiscoveryManager(const iox::p3com::GatewayConfig_t& config) noexcept
    : m_gatewayHash(generateHash())
    , m_preferredType(config.preferredTransport)
    , m_portSubscriber(iox::roudi::IntrospectionPortService, {1U, 1U})
    , m_gwIntrospectionPublisher(iox::p3com::IntrospectionGwService, {1U})
    , m_terminateFlag(false)
{
    // Register discovery callback
    iox::p3com::TransportInfo::doForAllEnabled([this](iox::p3com::TransportLayer& transport) {
        transport.registerDiscoveryCallback(
            [this](const void* serializedData, size_t size, iox::p3com::DeviceIndex_t deviceIndex) {
                receiveRemoteDiscoveryInfo(serializedData, size, deviceIndex);
            });
    });

    iox::p3com::TransportInfo::registerTransportFailCallback([this]() {
        const std::lock_guard<std::recursive_mutex> lock{m_mutex};
        m_remoteState.deviceIndicesCache.clear();
        if (!m_terminateFlag.load())
        {
            sendDiscoveryInfo(generateDiscoveryInfo());
        }
    });
}

void iox::p3com::DiscoveryManager::initialize(
    iox::cxx::function_ref<void(const iox::p3com::ServiceVector_t&)> updateCallback) noexcept
{
    iox::p3com::LogInfo() << "[p3comGateway] Initializing discovery system with gateway hash " << m_gatewayHash;
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    m_updateCallback = updateCallback;

    // Register local discovery event
    m_waitset.attachState(m_portSubscriber, iox::popo::SubscriberState::HAS_DATA).or_else([](auto) {
        iox::p3com::LogError() << "[p3comGateway] Could not attach local discovery event.";
    });
    {
#if defined(__FREERTOS__)
        // For this particular thread, we need 40 kB of stack space
        m_waitsetThread = [this]() {
            free_rtos_std::stacksize_lock_section stacksize_lock{10240U};
            return std::thread(&iox::p3com::DiscoveryManager::waitsetLoop, this);
        }();
#else
        m_waitsetThread = std::thread(&iox::p3com::DiscoveryManager::waitsetLoop, this);
#endif
    }

    sendDiscoveryInfo(generateDiscoveryInfo());
}

void iox::p3com::DiscoveryManager::deinitialize() noexcept
{
    iox::p3com::LogInfo() << "[p3comGateway] Terminating discovery system with gateway hash " << m_gatewayHash;

    m_terminateFlag.store(true);
    m_waitsetThread.join();

    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_updateCallback = [](const iox::p3com::ServiceVector_t&) {};

    iox::p3com::PubSubInfo_t info = generateDiscoveryInfo();
    info.isTermination = true;
    sendDiscoveryInfo(info);
}

iox::p3com::PubSubInfo_t iox::p3com::DiscoveryManager::generateDiscoveryInfo() noexcept
{
    iox::p3com::PubSubInfo_t info;

    info.gatewayBitset = iox::p3com::TransportInfo::bitset();
    info.gatewayHash = m_gatewayHash;
    info.infoHash = iox::p3com::generateHash();
    info.isTermination = false;

    // We only need to send a single instance for every subscriber topic, since
    // the remote gateways only check whether at least one exists.
    iox::p3com::pushUnique(info.userSubscribers, m_localState.userSubscribers);

    return info;
}

iox::p3com::ServiceVector_t iox::p3com::DiscoveryManager::updateNeededChannels() const noexcept
{
    iox::p3com::ServiceVector_t neededServices;
    // Need a channel for every local publisher and subscriber...
    const iox::p3com::ServiceVector_t& userSubscribers = m_localState.userSubscribers;
    iox::p3com::pushUnique(neededServices, userSubscribers);
    const iox::p3com::ServiceVector_t& userPublishers = m_localState.userPublishers;
    iox::p3com::pushUnique(neededServices, userPublishers);

    // ...and for every remote subscriber.
    for (const iox::p3com::DeviceRecord_t& record : m_remoteState.records)
    {
        iox::p3com::pushUnique(neededServices, record.info.userSubscribers);
    }
    return neededServices;
}

void iox::p3com::DiscoveryManager::waitsetLoop() noexcept
{
    constexpr iox::units::Duration WAITSET_TIMEOUT{iox::units::Duration::fromMilliseconds(50)};
    while (!m_terminateFlag.load())
    {
        const auto notificationVector = m_waitset.timedWait(WAITSET_TIMEOUT);
        for (auto& notification : notificationVector)
        {
            iox::cxx::Expects(notification->doesOriginateFrom(&m_portSubscriber));
            readPortSubscriber();
        }
    }
}

void iox::p3com::DiscoveryManager::readPortSubscriber() noexcept
{
    iox::p3com::ServiceVector_t neededServices;
    {
        iox::p3com::LocalState_t newLocalState;
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        // Receive a new list of user publishers and subscribers
        m_portSubscriber.take()
            .and_then([&](auto& sample) {
                for (const auto& portData : sample->m_publisherList)
                {
                    // Ignore pubs from gateway and introspection
                    if (portData.m_node != iox::p3com::IOX_GW_NODE_NAME
                        && portData.m_caproInstanceID != iox::capro::IdString_t{"RouDi_ID"})
                    {
                        newLocalState.userPublishers.push_back(
                            {portData.m_caproServiceID, portData.m_caproInstanceID, portData.m_caproEventMethodID});
                        newLocalState.userPublisherPorts.push_back(portData.m_publisherPortID);
                    }
                }
                for (const auto& portData : sample->m_subscriberList)
                {
                    // Ignore subs from gateway and introspection
                    if (portData.m_node != iox::p3com::IOX_GW_NODE_NAME
                        && portData.m_caproInstanceID != iox::capro::IdString_t{"RouDi_ID"})
                    {
                        newLocalState.userSubscribers.push_back(
                            {portData.m_caproServiceID, portData.m_caproInstanceID, portData.m_caproEventMethodID});
                    }
                }
            })
            .or_else([&](auto& error) {
                if (error != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE)
                {
                    iox::p3com::LogError() << "[p3comGateway] Could not read introspection data with error: "
                                         << static_cast<int>(error);
                }
            });

        iox::p3com::ServiceVector_t oldUserPublishers = m_localState.userPublishers;
        iox::p3com::deleteElements(oldUserPublishers, newLocalState.userPublishers);
        iox::p3com::ServiceVector_t oldUserSubscribers = m_localState.userSubscribers;
        iox::p3com::deleteElements(oldUserSubscribers, newLocalState.userSubscribers);

        iox::p3com::ServiceVector_t newUserPublishers = newLocalState.userPublishers;
        iox::p3com::deleteElements(newUserPublishers, m_localState.userPublishers);
        iox::p3com::ServiceVector_t newUserSubscribers = newLocalState.userSubscribers;
        iox::p3com::deleteElements(newUserSubscribers, m_localState.userSubscribers);

        for (auto& service : oldUserPublishers)
        {
            iox::p3com::LogInfo() << "[p3comGateway] Detected destroyed user publisher: " << service;
        }
        for (auto& service : oldUserSubscribers)
        {
            iox::p3com::LogInfo() << "[p3comGateway] Detected destroyed user subscriber: " << service;
        }
        for (auto& service : newUserPublishers)
        {
            iox::p3com::LogInfo() << "[p3comGateway] Detected new user publisher: " << service;
        }
        for (auto& service : newUserSubscribers)
        {
            iox::p3com::LogInfo() << "[p3comGateway] Detected new user subscriber: " << service;
        }

        m_localState = newLocalState;

        // If nothing changed, we wrap up now...
        if (oldUserPublishers.empty() && oldUserSubscribers.empty() && newUserPublishers.empty()
            && newUserSubscribers.empty())
        {
            return;
        }

        // ...otherwise, we need to update local state and send update to other devices.
        neededServices = updateNeededChannels();
    }

    if (m_updateCallback)
    {
        m_updateCallback(neededServices);
    }

    // Now send discovery info. The only thing that could have changed are user subscribers, so only send if they
    // actually did change.
    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    const auto info{generateDiscoveryInfo()};
    if (info.userSubscribers != m_lastSentDiscoveryInfo.userSubscribers)
    {
        sendDiscoveryInfo(info);
        m_lastSentDiscoveryInfo = info;
    }

    m_gwIntrospectionPublisher.publishCopyOf(m_localState.userPublisherPorts).or_else([](auto& error) {
        iox::p3com::LogWarn() << "[p3comGateway] Unable to publish gateway introspection, error: " << error;
    });
}

void iox::p3com::DiscoveryManager::sendDiscoveryInfo(const iox::p3com::PubSubInfo_t& info) noexcept
{
    std::array<char, iox::p3com::maxPubSubInfoSerializationSize()> serializedBytes;
    const uint32_t serializedSize = iox::p3com::serialize(info, serializedBytes.data());
    iox::p3com::TransportInfo::doForAllEnabled([&serializedBytes, serializedSize](iox::p3com::TransportLayer& transport) {
        transport.sendBroadcast(serializedBytes.data(), serializedSize);
    });
}

void iox::p3com::DiscoveryManager::resendDiscoveryInfoToTransport(iox::p3com::TransportType type) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto info = generateDiscoveryInfo();

    std::array<char, iox::p3com::maxPubSubInfoSerializationSize()> serializedBytes;
    const uint32_t serializedSize = iox::p3com::serialize(info, serializedBytes.data());
    iox::p3com::TransportInfo::doFor(type, [&serializedBytes, serializedSize](iox::p3com::TransportLayer& transport) {
        transport.sendBroadcast(serializedBytes.data(), serializedSize);
    });
}

void iox::p3com::DiscoveryManager::receiveRemoteDiscoveryInfo(const void* serializedData,
                                                            size_t size,
                                                            DeviceIndex_t deviceIndex) noexcept
{
    // Static to avoid stack overflow. This is fine only when a single
    // transport layer is used, such as on FreeRTOS. Otherwise, this method can
    // be called concurrently from multiple transport layers.
#if defined(__FREERTOS__)
    static iox::p3com::PubSubInfo_t info;
#else
    iox::p3com::PubSubInfo_t info;
#endif
    iox::p3com::deserialize(info, static_cast<const char*>(serializedData), size);

    iox::p3com::ServiceVector_t neededServices;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto* recordIt = std::find_if(
            m_remoteState.records.begin(), m_remoteState.records.end(), [&](const iox::p3com::DeviceRecord_t& record) {
                return record.info.gatewayHash == info.gatewayHash;
            });

        // If this is a termination message, erase the record and return
        if (info.isTermination)
        {
            if (recordIt != m_remoteState.records.end())
            {
                m_remoteState.records.erase(recordIt);
                iox::p3com::LogInfo() << "[p3comGateway] Deleted device record for gateway hash " << info.gatewayHash;
            }
            return;
        }
        else
        {
            // If this is not-yet-seen device, we also send discovery info
            if (recordIt == m_remoteState.records.end())
            {
                iox::p3com::LogInfo() << "[p3comGateway] Registered device record for gateway hash " << info.gatewayHash;
                if (!m_terminateFlag.load())
                {
                    sendDiscoveryInfo(generateDiscoveryInfo());
                }
                m_remoteState.records.emplace_back();
                recordIt = &m_remoteState.records.back();
            }

            // Update the information in the found or newly created record
            auto& record = *recordIt;
            record.info = info;

            // Store device index
            auto* indexIt = std::find_if(record.deviceIndices.begin(),
                                         record.deviceIndices.end(),
                                         [&deviceIndex](const auto& i) { return i.type == deviceIndex.type; });
            if (indexIt == record.deviceIndices.end())
            {
                record.deviceIndices.push_back(deviceIndex);
            }
            else if (indexIt->device != deviceIndex.device)
            {
                iox::p3com::LogError() << "[p3comGateway] Internal discovery system error";
            }
        }

        m_remoteState.deviceIndicesCache.clear();
        neededServices = updateNeededChannels();
    }

    if (m_updateCallback)
    {
        m_updateCallback(neededServices);
    }
}

const iox::p3com::DeviceIndexVector_t&
iox::p3com::DiscoveryManager::generateDeviceIndices(const iox::popo::UniquePortId& uid,
                                                  const iox::capro::ServiceDescription::ClassHash& serviceHash) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    static const iox::p3com::DeviceIndexVector_t EMPTY_DEVICE_INDICES{};
    if (containsElement(m_gatewayPublisherUids, uid))
    {
        // This is a message from Transport2Iceoryx gateway, ignore it
        return EMPTY_DEVICE_INDICES;
    }
    if (!containsElement(m_localState.userPublisherPorts, static_cast<uint64_t>(uid)))
    {
        // This is a message from a user publisher which hasnt yet been discovered, ignore it
        return EMPTY_DEVICE_INDICES;
    }

    const auto it = m_remoteState.deviceIndicesCache.find(serviceHash);
    if (it == m_remoteState.deviceIndicesCache.end())
    {
        const bool emplaced =  m_remoteState.deviceIndicesCache.emplace(serviceHash, computeDeviceIndices(serviceHash));
        if (!emplaced)
        {
            iox::p3com::LogError() << "[p3comGateway] Out of memory in discovery system";
            return EMPTY_DEVICE_INDICES;
        }
        return m_remoteState.deviceIndicesCache.back();
    }
    else{
        return *it;
    }
}

iox::p3com::DeviceIndexVector_t iox::p3com::DiscoveryManager::computeDeviceIndices(
    const iox::capro::ServiceDescription::ClassHash& serviceHash) const noexcept
{
    iox::cxx::vector<iox::p3com::DeviceIndex_t, iox::p3com::MAX_DEVICE_COUNT> deviceIndices;
    for (const iox::p3com::DeviceRecord_t& r : m_remoteState.records)
    {
        // If there is some matching subscriber, send the message there
        if (iox::p3com::containsElement(r.info.userSubscribers, serviceHash))
        {
            const auto preferredType = iox::p3com::TransportInfo::findMatchingType(r.info.gatewayBitset, m_preferredType);
            if (preferredType != iox::p3com::TransportType::NONE)
            {
                const auto* indexIt = std::find_if(r.deviceIndices.begin(),
                                                   r.deviceIndices.end(),
                                                   [preferredType](const auto& i) { return i.type == preferredType; });
                if (indexIt != r.deviceIndices.end())
                {
                    deviceIndices.push_back({*indexIt});
                }
            }
        }
    }

    return deviceIndices;
}

iox::p3com::DeviceIndexVector_t iox::p3com::DiscoveryManager::generateDeviceIndicesForwarding(
    const iox::capro::ServiceDescription::ClassHash& serviceHash, iox::p3com::DeviceIndex_t fromDeviceIndex) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    iox::p3com::bitset_t fromDeviceBitset{};
    for (const iox::p3com::DeviceRecord_t& r : m_remoteState.records)
    {
        if (iox::p3com::containsElement(r.deviceIndices, fromDeviceIndex))
        {
            fromDeviceBitset = r.info.gatewayBitset;
        }
    }

    // If the source device was not found, that is weird and we should just ignore this message
    if (fromDeviceBitset.none())
    {
        iox::p3com::LogWarn() << "[p3comGateway] Could not find source device for a forwarded message! Discarding!";
        return {};
    }

    // So, we are looking for subscribers for this service, which are on
    // devices that dont share any enabled transport layer with the source
    // device
    iox::cxx::vector<iox::p3com::DeviceIndex_t, iox::p3com::MAX_DEVICE_COUNT> deviceIndices;
    for (const iox::p3com::DeviceRecord_t& r : m_remoteState.records)
    {
        // If there is some matching subscriber, send the message there
        const auto commonTransportLayers = fromDeviceBitset & r.info.gatewayBitset;
        if (commonTransportLayers.none() && iox::p3com::containsElement(r.info.userSubscribers, serviceHash))
        {
            const auto preferredType = iox::p3com::TransportInfo::findMatchingType(r.info.gatewayBitset, m_preferredType);
            if (preferredType != iox::p3com::TransportType::NONE)
            {
                const auto* indexIt = std::find_if(r.deviceIndices.begin(),
                                                   r.deviceIndices.end(),
                                                   [preferredType](const auto& i) { return i.type == preferredType; });
                if (indexIt != r.deviceIndices.end())
                {
                    deviceIndices.push_back({*indexIt});
                }
            }
        }
    }

    return deviceIndices;
}

void iox::p3com::DiscoveryManager::addGatewayPublisher(const iox::popo::UniquePortId& uid) noexcept
{
    // We assume that m_mutex is already locked by this thread
    m_gatewayPublisherUids.push_back(uid);
}

void iox::p3com::DiscoveryManager::discardGatewayPublisher(const iox::popo::UniquePortId& uid) noexcept
{
    // We assume that m_mutex is already locked by this thread
    deleteElement(m_gatewayPublisherUids, uid);
}
