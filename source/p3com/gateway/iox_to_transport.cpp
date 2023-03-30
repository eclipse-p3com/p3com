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

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"

#include "p3com/gateway/iox_to_transport.hpp"
#include "p3com/generic/config.hpp"
#include "p3com/generic/types.hpp"
#include "p3com/transport/transport_info.hpp"
#include "p3com/internal/log/logging.hpp"

iox::p3com::Iceoryx2Transport::Iceoryx2Transport(iox::p3com::DiscoveryManager& discovery,
                                                 iox::p3com::PendingMessageManager& pendingMessageManager) noexcept
    : m_discovery(discovery)
    , m_pendingMessageManager(pendingMessageManager)
    , m_terminateFlag(false)
    , m_suspendFlag(false)
    , m_waitsetThread(&Iceoryx2Transport::waitsetLoop, this)
{
}

void iox::p3com::Iceoryx2Transport::join() noexcept
{
    m_terminateFlag.store(true);
    if (m_waitsetThread.joinable())
    {
        m_waitsetThread.join();
    }
}

void iox::p3com::Iceoryx2Transport::updateChannels(const iox::p3com::ServiceVector_t& services) noexcept
{
    updateChannelsInternal(services,
                           [this](const iox::capro::ServiceDescription& service) { setupChannel(service); },
                           [this](const iox::capro::ServiceDescription& service) { deleteChannel(service); });
}

void iox::p3com::Iceoryx2Transport::setupChannel(const iox::capro::ServiceDescription& service) noexcept
{
    iox::popo::SubscriberOptions options;
    options.nodeName = IOX_GW_NODE_NAME;
    addChannel(service, options, [this](iox::popo::UntypedSubscriber& subscriber) {
        m_suspendFlag.store(true);
        std::lock_guard<std::mutex> lock{m_waitsetMutex};
        m_waitset.attachState(subscriber, iox::popo::SubscriberState::HAS_DATA).or_else([](auto& error) {
            iox::p3com::LogError() << "[Iceoryx2Transport] Could not attach subscriber callback!"
                                 << " Error code: " << static_cast<uint64_t>(error);
        });
        m_suspendFlag.store(false);
    });

    iox::p3com::LogInfo() << "[Iceoryx2Transport] Set up channel for service: " << service;
}

void
iox::p3com::Iceoryx2Transport::deleteChannel(const iox::capro::ServiceDescription& service) noexcept
{
    discardChannel(service, [this](iox::popo::UntypedSubscriber& subscriber) {
        {
            m_suspendFlag.store(true);
            std::lock_guard<std::mutex> lock{m_waitsetMutex};
            m_waitset.detachState(subscriber, iox::popo::SubscriberState::HAS_DATA);
            m_suspendFlag.store(false);
        }
        subscriber.unsubscribe();

        // Wait until all associated pending messages are finished.
        while(true)
        {
            if (m_pendingMessageManager.anyPending(subscriber))
            {
                std::this_thread::yield();
            }
            else
            {
                break;
            }
        }
    });

    iox::p3com::LogInfo() << "[Iceoryx2Transport] Discarded channel for service: " << service;
}

void iox::p3com::Iceoryx2Transport::waitsetLoop() noexcept
{
    constexpr iox::units::Duration WAITSET_TIMEOUT{iox::units::Duration::fromMilliseconds(50)};
    while(!m_terminateFlag.load())
    {
        decltype(m_waitset)::NotificationInfoVector notificationVector;
        {
            std::lock_guard<std::mutex> lock{m_waitsetMutex};
            notificationVector = m_waitset.timedWait(WAITSET_TIMEOUT);
        }
        while(m_suspendFlag.load())
        {
            std::this_thread::yield();
        }

        for (auto& notification : notificationVector)
        {
            iox::popo::UntypedSubscriber& subscriber = *notification->getOrigin<iox::popo::UntypedSubscriber>();

            const void* userPayload = nullptr;
            {
                std::lock_guard<std::mutex> lock{endpointsMutex()};
                subscriber.take().and_then([&userPayload](const void* p) { userPayload = p; }).or_else([](auto& error) {
                    switch (error)
                    {
                    case iox::popo::ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL:
                        iox::p3com::LogError()
                            << "[Iceoryx2Transport] Gateway subscriber failed because too many chunks "
                               "are held in parallel!";
                        break;
                    case iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE:
                        break; // This is fine, it just means there are currently no messages to take
                    }
                });
            }

            if (userPayload != nullptr)
            {
                const auto chunkHeader = iox::mepoo::ChunkHeader::fromUserPayload(userPayload);
                const auto serviceDescription = subscriber.getServiceDescription();
                const auto hash = serviceDescription.getClassHash();

                const auto deviceIndices = m_discovery.generateDeviceIndices(chunkHeader->originId(), hash);
                if (deviceIndices.empty())
                {
                    std::lock_guard<std::mutex> lock{endpointsMutex()};
                    subscriber.release(userPayload);
                }
                else
                {
                    iox::p3com::LogInfo() << "[Iceoryx2Transport] Forwarding user message from publisher ID "
                                        << static_cast<uint64_t>(chunkHeader->originId())
                                        << " for service: " << serviceDescription;

                    iox::p3com::IoxChunkDatagramHeader_t datagramHeader;
                    datagramHeader.serviceHash = hash;
                    datagramHeader.messageHash = iox::p3com::generateHash();
                    datagramHeader.userPayloadSize = chunkHeader->userPayloadSize();
                    datagramHeader.userPayloadAlignment = chunkHeader->userPayloadAlignment();
                    datagramHeader.userHeaderSize =
                        (chunkHeader->userHeaderId() == iox::mepoo::ChunkHeader::NO_USER_HEADER)
                            ? 0U
                            : chunkHeader->userHeaderSize();
                    // m_submessage* fields will be filled for every submessage individually inside writeSegmented

                    iox::p3com::writeSegmented(datagramHeader,
                                             *chunkHeader,
                                             deviceIndices,
                                             m_pendingMessageManager,
                                             endpointsMutex(),
                                             subscriber);
                }
            }
        }
    }
}
