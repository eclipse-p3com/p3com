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

#ifndef P3COM_SEGMENTED_MESSAGES_HPP
#define P3COM_SEGMENTED_MESSAGES_HPP

#include "p3com/utility/vector_map.hpp"
#include "p3com/generic/types.hpp"

#include "iceoryx_posh/popo/untyped_publisher.hpp"

namespace iox
{
namespace p3com
{
class SegmentedMessageManager
{
  public:
    SegmentedMessageManager() noexcept = default;

    SegmentedMessageManager(const SegmentedMessageManager&) = delete;
    SegmentedMessageManager(SegmentedMessageManager&&) = delete;
    SegmentedMessageManager& operator=(const SegmentedMessageManager&) = delete;
    SegmentedMessageManager& operator=(SegmentedMessageManager&&) = delete;
    ~SegmentedMessageManager() = default;

    void push(hash_t messageHash,
              uint32_t remainingSegments,
              void* userHeader,
              void* userPayload,
              std::mutex& mutex,
              popo::UntypedPublisher& publisher,
              std::chrono::steady_clock::time_point deadline) noexcept;

    void find(hash_t messageHash, void*& userHeader, void*& userPayload) noexcept;
    bool findAndDecrement(hash_t messageHash, void*& userHeader, void*& userPayload) noexcept;

    void release(hash_t messageHash) noexcept;
    void releaseAll(popo::UntypedPublisher& publisher) noexcept;

    void checkSegmentedMessages() noexcept;

  private:
    void release(const void* userPayload) noexcept;

    struct SegmentedMessage_t
    {
        uint32_t remainingSegments;
        void* userHeader;
        void* userPayload;
        std::mutex* mutex;
        iox::popo::UntypedPublisher* publisher;
        std::chrono::steady_clock::time_point deadline;
    };

    std::mutex m_mutex;
#if defined(__FREERTOS__)
    static constexpr uint32_t MAX_SEGMENTED_MESSAGE_COUNT = 4U;
#else
    static constexpr uint32_t MAX_SEGMENTED_MESSAGE_COUNT = 64U;
#endif
    cxx::vector_map<hash_t, SegmentedMessage_t, MAX_SEGMENTED_MESSAGE_COUNT> m_segmentedMessages;
};

} // namespace p3com
} // namespace iox

#endif
