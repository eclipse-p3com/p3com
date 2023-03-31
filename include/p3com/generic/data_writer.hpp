// Copyright 2023 NXP

#ifndef P3COM_DATA_WRITER_HPP
#define P3COM_DATA_WRITER_HPP

#include "p3com/generic/pending_messages.hpp"
#include "p3com/generic/types.hpp"

#include "iceoryx_posh/mepoo/chunk_header.hpp"

namespace iox
{
namespace p3com
{
/**
 * @brief Write the user message over all enabled transport layers.
 *
 * @param datagramHeader
 * @param chunkHeader
 * @param deviceIndices
 * @param pendingMessageManager
 * @param subscriber
 *
 * @return 
 */
void writeSegmented(IoxChunkDatagramHeader_t& datagramHeader,
                    const mepoo::ChunkHeader& chunkHeader,
                    const cxx::vector<DeviceIndex_t, MAX_DEVICE_COUNT>& deviceIndices,
                    PendingMessageManager& pendingMessageManager,
                    std::mutex& mutex,
                    popo::UntypedSubscriber& subscriber) noexcept;

} // namespace p3com
} // namespace iox

#endif // P3COM_DATA_WRITER_HPP
