// Copyright 2023 NXP

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
