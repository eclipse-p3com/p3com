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
