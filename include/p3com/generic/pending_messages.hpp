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

#ifndef P3COM_PENDING_MESSAGES_HPP
#define P3COM_PENDING_MESSAGES_HPP

#include "p3com/utility/vector_map.hpp"

#include "iceoryx_posh/popo/untyped_subscriber.hpp"

namespace iox
{
namespace p3com
{
class PendingMessageManager
{
  public:
    PendingMessageManager() noexcept;

    PendingMessageManager(const PendingMessageManager&) = delete;
    PendingMessageManager(PendingMessageManager&&) = delete;
    PendingMessageManager& operator=(const PendingMessageManager&) = delete;
    PendingMessageManager& operator=(PendingMessageManager&&) = delete;
    ~PendingMessageManager() = default;

    bool push(const void* userPayload, std::mutex& mutex, popo::UntypedSubscriber& subscriber) noexcept;

    bool anyPending(popo::UntypedSubscriber& subscriber) noexcept;

    void release(const void* userPayload) noexcept;

  private:
    struct PendingMessage_t
    {
        uint32_t counter;
        std::mutex* mutex;
        popo::UntypedSubscriber* subscriber;
    };

    std::mutex m_mutex;
#if defined(__FREERTOS__)
    static constexpr uint32_t MAX_PENDING_MESSAGE_COUNT = 4U;
#else
    static constexpr uint32_t MAX_PENDING_MESSAGE_COUNT = 512U;
#endif
    cxx::vector_map<const void*, PendingMessage_t, MAX_PENDING_MESSAGE_COUNT> m_pendingMessages;
};

} // namespace p3com
} // namespace iox

#endif
