// Copyright 2023 NXP

#include "p3com/generic/data_writer.hpp"
#include "p3com/generic/pending_messages.hpp"
#include "p3com/generic/serialization.hpp"
#include "p3com/generic/types.hpp"
#include "p3com/internal/log/logging.hpp"
#include "p3com/transport/transport_info.hpp"

#include <array>
#include <mutex>

namespace
{
uint32_t divideAndRoundUp(uint32_t divident, uint32_t divisor) noexcept
{
    return static_cast<uint32_t>((divident + divisor - 1U) / divisor);
}

bool writeSegmentedInternal(iox::p3com::IoxChunkDatagramHeader_t& datagramHeader,
                            const uint8_t* const userHeaderBytes,
                            const uint8_t* const userPayloadBytes,
                            const iox::p3com::DeviceIndex_t& deviceIndex) noexcept
{
    // Obtain the corresponding transport and its maximum message size
    uint32_t pendingCount = 0U;
    iox::p3com::TransportInfo::doFor(deviceIndex.type, [&](iox::p3com::TransportLayer& transport) {
        std::array<char, iox::p3com::maxIoxChunkDatagramHeaderSerializationSize()> serializedDatagramHeaderBytes;
        const uint32_t maxTransportPayloadSize =
            static_cast<uint32_t>(transport.maxMessageSize() - iox::p3com::maxIoxChunkDatagramHeaderSerializationSize());
        datagramHeader.submessageCount = divideAndRoundUp(datagramHeader.userHeaderSize, maxTransportPayloadSize)
                                         + divideAndRoundUp(datagramHeader.userPayloadSize, maxTransportPayloadSize);

        // Send individual submessages
        uint32_t remainingUserHeaderSize = datagramHeader.userHeaderSize;
        uint32_t remainingUserPayloadSize = datagramHeader.userPayloadSize;

        datagramHeader.submessageOffset = 0U;
        while (remainingUserHeaderSize != 0U)
        {
            datagramHeader.submessageSize = std::min(maxTransportPayloadSize, remainingUserHeaderSize);
            const uint32_t serializedDatagramHeaderSize =
                iox::p3com::serialize(datagramHeader, serializedDatagramHeaderBytes.data());

            const bool isPending = transport.sendUserData(serializedDatagramHeaderBytes.data(),
                                                          serializedDatagramHeaderSize,
                                                          userHeaderBytes + datagramHeader.submessageOffset,
                                                          datagramHeader.submessageSize,
                                                          deviceIndex.device);
            if (isPending)
            {
                iox::p3com::LogFatal()
                    << "[DataWriter] Pending messages for user headers are not supported in this p3com gateway "
                       "version! Please make your user headers smaller!";
            }

            datagramHeader.submessageOffset += datagramHeader.submessageSize;
            remainingUserHeaderSize -= datagramHeader.submessageSize;
        }

        while (remainingUserPayloadSize != 0U)
        {
            datagramHeader.submessageSize = std::min(maxTransportPayloadSize, remainingUserPayloadSize);

            const uint32_t serializedDatagramHeaderSize =
                iox::p3com::serialize(datagramHeader, serializedDatagramHeaderBytes.data());
            pendingCount += static_cast<uint32_t>(transport.sendUserData(
                serializedDatagramHeaderBytes.data(),
                serializedDatagramHeaderSize,
                userPayloadBytes + datagramHeader.submessageOffset - datagramHeader.userHeaderSize,
                datagramHeader.submessageSize,
                deviceIndex.device));

            datagramHeader.submessageOffset += datagramHeader.submessageSize;
            remainingUserPayloadSize -= datagramHeader.submessageSize;
        }
    });

    if (pendingCount > 1U)
    {
        iox::p3com::LogFatal() << "[DataWriter] Multiple pending submessages for a single message are not supported in "
                                "this p3com gateway version!";
    }
    return pendingCount == 1U;
}

} // anonymous namespace

void iox::p3com::writeSegmented(
    iox::p3com::IoxChunkDatagramHeader_t& datagramHeader,
    const iox::mepoo::ChunkHeader& chunkHeader,
    const iox::cxx::vector<iox::p3com::DeviceIndex_t, iox::p3com::MAX_DEVICE_COUNT>& deviceIndices,
    iox::p3com::PendingMessageManager& pendingMessageManager,
    std::mutex& mutex,
    iox::popo::UntypedSubscriber& subscriber) noexcept
{
    for (const auto& i : deviceIndices)
    {
        bool shouldBePending = false;
        bool shouldDiscard = false;
        iox::p3com::TransportInfo::doFor(i.type, [&](auto& transport) {
            shouldBePending = transport.willBePending(datagramHeader.userPayloadSize);
            if (shouldBePending)
            {
                const bool pushed = pendingMessageManager.push(chunkHeader.userPayload(), mutex, subscriber);
                if (!pushed)
                {
                    iox::p3com::LogWarn() << "[DataWriter] Exceeded maximum number of pending messages! Discarding!";
                    shouldDiscard = true;
                }
            }
        });
        if (shouldDiscard)
        {
            std::lock_guard<std::mutex> lock{mutex};
            subscriber.release(chunkHeader.userPayload());
            return;
        }

        const bool isPending = writeSegmentedInternal(datagramHeader,
                                                      static_cast<const uint8_t*>(chunkHeader.userHeader()),
                                                      static_cast<const uint8_t*>(chunkHeader.userPayload()),
                                                      i);
        if (!isPending)
        {
            if (shouldBePending)
            {
                // This likely means that the message should have been pending,
                // but sending failed so it isnt pending. Note that mutex is
                // locked inside the release function, so we dont need to lock
                // it here.
                pendingMessageManager.release(chunkHeader.userPayload());
            }
            else
            {
                std::lock_guard<std::mutex> lock{mutex};
                subscriber.release(chunkHeader.userPayload());
            }
        }
    }
}
