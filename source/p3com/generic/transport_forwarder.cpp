// Copyright 2023 NXP

#include "p3com/generic/transport_forwarder.hpp"
#include "p3com/generic/pending_messages.hpp"
#include "p3com/generic/data_writer.hpp"
#include "p3com/utility/helper_functions.hpp"
#include "p3com/internal/log/logging.hpp"

iox::p3com::TransportForwarder::TransportForwarder(
    iox::p3com::DiscoveryManager& discovery,
    iox::p3com::PendingMessageManager& pendingMessageManager,
    const iox::cxx::vector<capro::ServiceDescription, iox::p3com::MAX_FORWARDED_SERVICES>& forwardedServices) noexcept
    : m_discovery(discovery)
    , m_pendingMessageManager(pendingMessageManager)
    , m_terminateFlag(false)
{
    for (auto& service : forwardedServices)
    {
        m_forwardedServiceHashes.emplace_back(service.getClassHash());

        const NodeName_t FORWARDED_SERVICES_NODE_NAME{"iox-gw-forwarded"};
        iox::popo::SubscriberOptions options;
        options.nodeName = FORWARDED_SERVICES_NODE_NAME;
        m_forwardedServiceSubscribers.emplace_back(service, options);

        m_waitset.attachState(m_forwardedServiceSubscribers.back(), iox::popo::SubscriberState::HAS_DATA)
            .and_then([&service]() {
                iox::p3com::LogInfo() << "[TransportForwarder] Set up forwarding for service: " << service;
            })
            .or_else([](auto& error) {
                iox::p3com::LogError() << "[TransportForwarder] Could not attach listener subscriber callback!"
                                     << " Error code: " << static_cast<uint64_t>(error);
            });
    }

    if (!forwardedServices.empty())
    {
        m_waitsetThread = std::thread(&TransportForwarder::waitsetLoop, this);
    }
}

void iox::p3com::TransportForwarder::join() noexcept
{
    m_terminateFlag.store(true);
    if (m_waitsetThread.joinable())
    {
        m_waitsetThread.join();
    }
}

void iox::p3com::TransportForwarder::push(const void* userPayload,
                                        iox::capro::ServiceDescription::ClassHash hash,
                                        DeviceIndex_t deviceIndex) noexcept
{
    if (iox::p3com::containsElement(m_forwardedServiceHashes, hash))
    {
        std::lock_guard<std::mutex> lock(m_messagesToForwardMutex);
        m_messagesToForward.emplace(userPayload, deviceIndex);
    }
}

void iox::p3com::TransportForwarder::waitsetLoop() noexcept
{
    constexpr iox::units::Duration WAITSET_TIMEOUT{iox::units::Duration::fromMilliseconds(50)};
    while (!m_terminateFlag.load())
    {
        const auto notificationVector = m_waitset.timedWait(WAITSET_TIMEOUT);
        for (auto& notification : notificationVector)
        {
            iox::popo::UntypedSubscriber& subscriber = *notification->getOrigin<iox::popo::UntypedSubscriber>();
            const void* userPayload = nullptr;
            {
                std::lock_guard<std::mutex> lock{m_forwardedServiceSubscribersMutex};
                subscriber.take().and_then([&userPayload](const void* p) { userPayload = p; }).or_else([](auto& error) {
                    switch (error)
                    {
                    case iox::popo::ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL:
                        iox::p3com::LogError() << "[TransportForwarder] Forwarder subscriber failed because too many "
                                                "chunks are held in parallel!";
                        break;
                    case iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE:
                        break; // This is fine, it just means there are currently no messages to take
                    }
                });
            }

            if (userPayload != nullptr)
            {
                const auto chunkHeader = iox::mepoo::ChunkHeader::fromUserPayload(userPayload);
                const auto hash = subscriber.getServiceDescription().getClassHash();

                iox::p3com::DeviceIndex_t deviceIndex;
                // This check is not actually needed, because if the service hash is not
                // one of the forwarded ones, it definitely will not be present in
                // m_messagesToForward. However, this way we avoid the overhead of locking
                // the mutex, so this might be a teeny tiny optimization.
                if (iox::p3com::containsElement(m_forwardedServiceHashes, hash))
                {
                    std::lock_guard<std::mutex> lock(m_messagesToForwardMutex);
                    iox::p3com::DeviceIndex_t* deviceIndexPtr = m_messagesToForward.find(userPayload);
                    if (deviceIndexPtr != nullptr)
                    {
                        deviceIndex = *deviceIndexPtr;
                        m_messagesToForward.erase(deviceIndexPtr);
                    }
                }

                const auto deviceIndices = m_discovery.generateDeviceIndicesForwarding(hash, deviceIndex);
                if (deviceIndices.empty())
                {
                    std::lock_guard<std::mutex> lock{m_forwardedServiceSubscribersMutex};
                    subscriber.release(userPayload);
                }
                else
                {
                    iox::p3com::LogInfo() << "[TransportForwarder] Forwarding user message for service: "
                                        << subscriber.getServiceDescription();

                    iox::p3com::IoxChunkDatagramHeader_t datagramHeader;
                    datagramHeader.serviceHash = hash;
                    datagramHeader.messageHash = iox::p3com::generateHash();
                    datagramHeader.userPayloadSize = chunkHeader->userPayloadSize();
                    datagramHeader.userPayloadAlignment = chunkHeader->userPayloadAlignment();
                    datagramHeader.userHeaderSize =
                        (chunkHeader->userHeaderId() == iox::mepoo::ChunkHeader::NO_USER_HEADER)
                            ? 0U
                            : chunkHeader->userHeaderSize();
                    // submessage* fields will be filled for every submessage individually inside writeSegmented

                    iox::p3com::writeSegmented(datagramHeader,
                                             *chunkHeader,
                                             deviceIndices,
                                             m_pendingMessageManager,
                                             m_forwardedServiceSubscribersMutex,
                                             subscriber);
                }
            }
        }
    }
}
