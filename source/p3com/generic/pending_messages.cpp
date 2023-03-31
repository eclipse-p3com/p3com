// Copyright 2023 NXP

#include "p3com/generic/pending_messages.hpp"
#include "p3com/transport/transport_info.hpp"
#include "p3com/internal/log/logging.hpp"
#include "p3com/utility/helper_functions.hpp"

iox::p3com::PendingMessageManager::PendingMessageManager() noexcept
{
    iox::p3com::TransportInfo::doForAllEnabled([this](iox::p3com::TransportLayer& transport) {
        // Register callback for buffer sending
        transport.registerBufferSentCallback(
            [this](const void* ptr) { release(ptr); });
    });
}

void iox::p3com::PendingMessageManager::release(const void* userPayload) noexcept
{
    iox::cxx::optional<iox::p3com::PendingMessageManager::PendingMessage_t> msgToRelease{iox::cxx::nullopt};

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto* msgIt = m_pendingMessages.find(userPayload);
        if (msgIt == m_pendingMessages.end())
        {
            iox::p3com::LogError() << "[PendingMessageManager] Cannot release invalid pending message!";
        }
        else
        {
            auto& pendingMsg = *msgIt;
            pendingMsg.counter--;
            if (pendingMsg.counter == 0U)
            {
                msgToRelease.emplace(pendingMsg);
                m_pendingMessages.erase(msgIt);
            }
        }
    }

    if (msgToRelease.has_value())
    {
        std::lock_guard<std::mutex> lock{*msgToRelease->mutex};
        msgToRelease->subscriber->release(userPayload);
    }
}

bool iox::p3com::PendingMessageManager::anyPending(iox::popo::UntypedSubscriber& subscriber) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pendingMsg : m_pendingMessages)
    {
        if (pendingMsg.subscriber == &subscriber)
        {
            return true;
        }
    }
    return false;
}

bool iox::p3com::PendingMessageManager::push(const void* userPayload, std::mutex& mutex, iox::popo::UntypedSubscriber& subscriber) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto* msgIt = m_pendingMessages.find(userPayload);
    if (msgIt == m_pendingMessages.end())
    {
        const bool emplaced = m_pendingMessages.emplace(
            userPayload, iox::p3com::PendingMessageManager::PendingMessage_t{1U, &mutex, &subscriber});

        return emplaced;
    }
    else
    {
        iox::p3com::LogFatal() << "[PendingMessageManager] Attempted to push a pending message which laready exists!";
        return false;
    }
}
