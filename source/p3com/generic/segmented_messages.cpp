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

#include "p3com/generic/segmented_messages.hpp"
#include "p3com/internal/log/logging.hpp"

void iox::p3com::SegmentedMessageManager::checkSegmentedMessages() noexcept
{
    const auto now = std::chrono::steady_clock::now();
    while (true)
    {
        iox::cxx::optional<iox::p3com::SegmentedMessageManager::SegmentedMessage_t> msgToRelease{iox::cxx::nullopt};

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& segmentedMsg : m_segmentedMessages)
            {
                if (segmentedMsg.deadline < now)
                {
                    iox::p3com::LogWarn() << "[SegmentedMessageManager] Detected message segment timeout, discarding!";
                    msgToRelease.emplace(segmentedMsg);
                    m_segmentedMessages.erase(&segmentedMsg);
                    break;
                }
            }
        }

        if (msgToRelease.has_value())
        {
            std::lock_guard<std::mutex> lock{*msgToRelease->mutex};
            msgToRelease->publisher->release(msgToRelease->userPayload);
        }
        else
        {
            break;
        }
    }
}

void iox::p3com::SegmentedMessageManager::push(iox::p3com::hash_t messageHash,
                                             uint32_t remainingSegments,
                                             void* userHeader,
                                             void* userPayload,
                                             std::mutex& mutex,
                                             iox::popo::UntypedPublisher& publisher,
                                             std::chrono::steady_clock::time_point deadline) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const bool emplaced =
        m_segmentedMessages.emplace(messageHash,
                                    iox::p3com::SegmentedMessageManager::SegmentedMessage_t{
                                        remainingSegments, userHeader, userPayload, &mutex, &publisher, deadline});
    if (!emplaced)
    {
        iox::p3com::LogWarn() << "[SegmentedMessageManager] Too many segmented messages at the same time, discarding!";
        publisher.release(userPayload);
    }
}

bool iox::p3com::SegmentedMessageManager::findAndDecrement(iox::p3com::hash_t messageHash,
                                                         void*& userHeader,
                                                         void*& userPayload) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto* msgIt = m_segmentedMessages.find(messageHash);
    if (msgIt != m_segmentedMessages.end())
    {
        auto& msg = *msgIt;

        userHeader = msg.userHeader;
        userPayload = msg.userPayload;
        msg.remainingSegments--;
        if (msg.remainingSegments == 0U)
        {
            m_segmentedMessages.erase(msgIt);
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

void iox::p3com::SegmentedMessageManager::find(iox::p3com::hash_t messageHash,
                                             void*& userHeader,
                                             void*& userPayload) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto* msgIt = m_segmentedMessages.find(messageHash);
    if (msgIt != m_segmentedMessages.end())
    {
        auto& msg = *msgIt;
        userHeader = msg.userHeader;
        userPayload = msg.userPayload;
    }
}

void iox::p3com::SegmentedMessageManager::release(hash_t messageHash) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto* msgIt = m_segmentedMessages.find(messageHash);
    if (msgIt != m_segmentedMessages.end())
    {
        auto& msg = *msgIt;
        msg.publisher->release(msg.userPayload);
        m_segmentedMessages.erase(msgIt);
    }
}

void iox::p3com::SegmentedMessageManager::releaseAll(popo::UntypedPublisher& publisher) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Remove elements in reverse order, to maintain validity of the iterator.
    for (auto it = m_segmentedMessages.end(); it != m_segmentedMessages.begin(); --it)
    {
        auto msgIt = it - 1;
        if (msgIt->publisher == &publisher)
        {
            msgIt->publisher->release(msgIt->userPayload);
            m_segmentedMessages.erase(msgIt);
        }
    }
}