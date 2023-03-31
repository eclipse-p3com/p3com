// Copyright 2023 NXP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/gateway/gateway_generic.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"

#include "p3com/gateway/transport_to_iox.hpp"
#include "p3com/generic/config.hpp"
#include "p3com/generic/data_writer.hpp"
#include "p3com/generic/segmented_messages.hpp"
#include "p3com/generic/serialization.hpp"
#include "p3com/generic/types.hpp"
#include "p3com/internal/log/logging.hpp"
#include "p3com/transport/transport_info.hpp"

namespace
{
constexpr std::chrono::nanoseconds NS_PER_BYTE{500U};
}

iox::p3com::Transport2Iceoryx::Transport2Iceoryx(iox::p3com::DiscoveryManager& discovery,
                                                 iox::p3com::TransportForwarder& transportForwarder,
                                                 iox::p3com::SegmentedMessageManager& segmentedMessageManager) noexcept
    : m_discovery(discovery)
    , m_transportForwarder(transportForwarder)
    , m_segmentedMessageManager(segmentedMessageManager)
{
    iox::p3com::TransportInfo::doForAllEnabled([this](iox::p3com::TransportLayer& transport) {
        // Register callback for user data received over transport
        transport.registerUserDataCallback(
            [this](const void* serializedUserPayload, size_t size, DeviceIndex_t deviceIndex) {
                receive(serializedUserPayload, size, deviceIndex);
            });

        // Register callback for buffer loaning
        transport.registerBufferNeededCallback([this](const void* serializedDatagramHeader, size_t size) {
            return loanBuffer(serializedDatagramHeader, size);
        });

        // Register callback for buffer releasing
        transport.registerBufferReleasedCallback(
            [this](const void* serializedDatagramHeader, size_t size, bool shouldRelease, DeviceIndex_t deviceIndex) {
                releaseBuffer(serializedDatagramHeader, size, shouldRelease, deviceIndex);
            });
    });
}

void iox::p3com::Transport2Iceoryx::updateChannels(const iox::p3com::ServiceVector_t& services) noexcept
{
    updateChannelsInternal(
        services,
        [this](const iox::capro::ServiceDescription& service) { setupChannel(service); },
        [this](const iox::capro::ServiceDescription& service) { deleteChannel(service); });
}

void iox::p3com::Transport2Iceoryx::receive(const void* serializedUserPayload,
                                           size_t size,
                                           iox::p3com::DeviceIndex_t deviceIndex) noexcept
{
    // Obtain the datagram header
    iox::p3com::IoxChunkDatagramHeader_t datagramHeader;
    const char* serializedUserPayloadPtr = static_cast<const char*>(serializedUserPayload);
    const uint32_t serializedDatagramHeaderSize = iox::p3com::deserialize(datagramHeader, serializedUserPayloadPtr, size);
    serializedUserPayloadPtr += serializedDatagramHeaderSize;

    // Validate header
    if (size != datagramHeader.submessageSize + serializedDatagramHeaderSize)
    {
        iox::p3com::LogInfo() << "[Transport2Iceoryx] Received invalid user message, discarding!";
        return;
    }

    doForChannel(datagramHeader.serviceHash, [&](iox::popo::UntypedPublisher& publisher) {
        void* userPayload = nullptr;
        void* userHeader = nullptr;
        bool shouldPublish = false;
        iox::p3com::LogInfo() << "[Transport2Iceoryx] Forwarding user message for service: "
                            << publisher.getServiceDescription()
                            << ", submessage offset: " << datagramHeader.submessageOffset;

        if (datagramHeader.submessageOffset == 0U)
        {
            const bool hasUserHeader = datagramHeader.userHeaderSize != 0U;

            // Need to allocate new buffer
            publisher
                .loan(datagramHeader.userPayloadSize,
                      datagramHeader.userPayloadAlignment,
                      hasUserHeader ? datagramHeader.userHeaderSize : iox::CHUNK_NO_USER_HEADER_SIZE,
                      hasUserHeader ? iox::p3com::USER_HEADER_ALIGNMENT : iox::CHUNK_NO_USER_HEADER_ALIGNMENT)
                .and_then([&](auto* p) {
                    userHeader = hasUserHeader ? iox::mepoo::ChunkHeader::fromUserPayload(p)->userHeader() : nullptr;
                    userPayload = p;
                })
                .or_else([](auto& error) {
                    if (error == iox::popo::AllocationError::TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL)
                    {
                        iox::p3com::LogWarn()
                            << "[Transport2Iceoryx] Too many chunks allocated in parallel, discarding!";
                    }
                    else if (error == iox::popo::AllocationError::RUNNING_OUT_OF_CHUNKS)
                    {
                        iox::p3com::LogWarn() << "[Transport2Iceoryx] Running out of chunks, discarding!";
                    }
                    else
                    {
                        iox::p3com::LogError() << "[Transport2Iceoryx] Could not loan chunk, discarding! Error code: "
                                             << static_cast<uint64_t>(error);
                    }
                });
            if (userPayload == nullptr)
            {
                return;
            }

            if (datagramHeader.submessageCount > 1U)
            {
                // Compute the deadline of this message
                const auto deadline = std::chrono::steady_clock::now()
                                      + NS_PER_BYTE * (datagramHeader.userHeaderSize + datagramHeader.userPayloadSize);

                m_segmentedMessageManager.push(datagramHeader.messageHash,
                                               datagramHeader.submessageCount - 1U,
                                               userHeader,
                                               userPayload,
                                               endpointsMutex(),
                                               publisher,
                                               deadline);
                shouldPublish = false;
            }
            else
            {
                shouldPublish = true;
            }
        }
        else
        {
            shouldPublish =
                m_segmentedMessageManager.findAndDecrement(datagramHeader.messageHash, userHeader, userPayload);
        }

        if (userPayload != nullptr)
        {
            iox::p3com::takeNext(datagramHeader,
                               serializedUserPayloadPtr,
                               static_cast<uint8_t*>(userHeader),
                               static_cast<uint8_t*>(userPayload));
            if (shouldPublish)
            {
                m_transportForwarder.push(userPayload, datagramHeader.serviceHash, deviceIndex);
                publisher.publish(userPayload);
            }
        }
    });
}

void* iox::p3com::Transport2Iceoryx::loanBuffer(const void* serializedDatagramHeader, size_t size) noexcept
{
    // Obtain the datagram header
    iox::p3com::IoxChunkDatagramHeader_t datagramHeader;
    const char* serializedDatagramHeaderPtr = static_cast<const char*>(serializedDatagramHeader);
    iox::p3com::deserialize(datagramHeader, serializedDatagramHeaderPtr, size);

    void* bufferPtr = nullptr;
    doForChannel(datagramHeader.serviceHash, [&](iox::popo::UntypedPublisher& publisher) {
        void* userPayload = nullptr;
        void* userHeader = nullptr;

        if (datagramHeader.submessageOffset == 0U)
        {
            const bool hasUserHeader = datagramHeader.userHeaderSize != 0U;

            // Need to allocate new buffer
            publisher
                .loan(datagramHeader.userPayloadSize,
                      datagramHeader.userPayloadAlignment,
                      hasUserHeader ? datagramHeader.userHeaderSize : iox::CHUNK_NO_USER_HEADER_SIZE,
                      hasUserHeader ? iox::p3com::USER_HEADER_ALIGNMENT : iox::CHUNK_NO_USER_HEADER_ALIGNMENT)
                .and_then([&](auto* p) {
                    userHeader = hasUserHeader ? iox::mepoo::ChunkHeader::fromUserPayload(p)->userHeader() : nullptr;
                    userPayload = p;
                })
                .or_else([](auto& error) {
                    if (error == iox::popo::AllocationError::TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL)
                    {
                        iox::p3com::LogWarn()
                            << "[Transport2Iceoryx] Too many chunks allocated in parallel, discarding!";
                    }
                    else if (error == iox::popo::AllocationError::RUNNING_OUT_OF_CHUNKS)
                    {
                        iox::p3com::LogWarn() << "[Transport2Iceoryx] Running out of chunks, discarding!";
                    }
                    else
                    {
                        iox::p3com::LogError() << "[Transport2Iceoryx] Could not loan chunk, discarding! Error code: "
                                             << static_cast<uint64_t>(error);
                    }
                });
            if (userPayload == nullptr)
            {
                return;
            }

            // Compute the deadline of this message
            const auto deadline = std::chrono::steady_clock::now()
                                  + NS_PER_BYTE * (datagramHeader.userHeaderSize + datagramHeader.userPayloadSize);

            // Save the segmented message into the list
            m_segmentedMessageManager.push(datagramHeader.messageHash,
                                           datagramHeader.submessageCount,
                                           userHeader,
                                           userPayload,
                                           endpointsMutex(),
                                           publisher,
                                           deadline);
        }
        else
        {
            m_segmentedMessageManager.find(datagramHeader.messageHash, userHeader, userPayload);
        }

        // Is this submessage filling the user header or the user payload?
        bufferPtr = (datagramHeader.submessageOffset < datagramHeader.userHeaderSize) ? userHeader : userPayload;
    });
    return bufferPtr;
}

void iox::p3com::Transport2Iceoryx::releaseBuffer(const void* serializedDatagramHeader,
                                                 size_t size,
                                                 bool shouldRelease,
                                                 iox::p3com::DeviceIndex_t deviceIndex) noexcept
{
    // Obtain the datagram header
    iox::p3com::IoxChunkDatagramHeader_t datagramHeader;
    const char* serializedDatagramHeaderPtr = static_cast<const char*>(serializedDatagramHeader);
    iox::p3com::deserialize(datagramHeader, serializedDatagramHeaderPtr, size);

    doForChannel(datagramHeader.serviceHash, [&](iox::popo::UntypedPublisher& publisher) {
        if (shouldRelease)
        {
            m_segmentedMessageManager.release(datagramHeader.messageHash);
        }
        else
        {
            void* userHeader = nullptr;
            void* userPayload = nullptr;
            const bool shouldPublish =
                m_segmentedMessageManager.findAndDecrement(datagramHeader.messageHash, userHeader, userPayload);
            if (shouldPublish)
            {
                m_transportForwarder.push(userPayload, datagramHeader.serviceHash, deviceIndex);
                publisher.publish(userPayload);
            }
        }
    });
}

void iox::p3com::Transport2Iceoryx::setupChannel(const capro::ServiceDescription& service) noexcept
{
    iox::popo::PublisherOptions options;
    options.nodeName = IOX_GW_NODE_NAME;
    addChannel(service, options, [this](iox::popo::UntypedPublisher& publisher) {
        m_discovery.addGatewayPublisher(publisher.getUid());
    });

    iox::p3com::LogInfo() << "[Transport2Iceoryx] Set up channel for service: " << service;
}

void iox::p3com::Transport2Iceoryx::deleteChannel(const iox::capro::ServiceDescription& service) noexcept
{
    // Note: m_segmentedMessageMutex is always locked *after* internal gateway mutex!!! Avoid deadlocks.
    discardChannel(service, [this](iox::popo::UntypedPublisher& publisher) {
        // Release segmented messages associated with this channel
        m_segmentedMessageManager.releaseAll(publisher);

        m_discovery.discardGatewayPublisher(publisher.getUid());
    });

    iox::p3com::LogInfo() << "[Transport2Iceoryx] Discarded channel for service: " << service;
}
