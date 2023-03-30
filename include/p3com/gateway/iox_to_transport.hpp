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

#ifndef P3COM_IOX_TO_TRANSPORT_HPP
#define P3COM_IOX_TO_TRANSPORT_HPP

#include "iceoryx_posh/popo/untyped_subscriber.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"

#include "p3com/generic/config.hpp"
#include "p3com/gateway/gateway.hpp"
#include "p3com/generic/types.hpp"
#include "p3com/generic/pending_messages.hpp"
#include "p3com/generic/data_writer.hpp"
#include "p3com/generic/discovery.hpp"
#include "p3com/transport/transport.hpp"
#include "p3com/utility/vector_map.hpp"

namespace iox
{
namespace p3com
{
///
/// @brief p3com gateway implementation for the iceoryx to transport direction.
///
class Iceoryx2Transport : public Gateway<iox::popo::UntypedSubscriber>
{
  public:
    explicit Iceoryx2Transport(DiscoveryManager& discovery, PendingMessageManager& pendingMessageManager) noexcept;

    void updateChannels(const ServiceVector_t& services) noexcept;

    void join() noexcept;

  private:
    void setupChannel(const capro::ServiceDescription& service) noexcept;
    void deleteChannel(const capro::ServiceDescription& service) noexcept;

    void waitsetLoop() noexcept;

    DiscoveryManager& m_discovery;
    PendingMessageManager& m_pendingMessageManager;

    std::mutex m_waitsetMutex;
    popo::WaitSet<MAX_TOPICS> m_waitset;

    std::atomic<bool> m_terminateFlag;
    std::atomic<bool> m_suspendFlag;
    std::thread m_waitsetThread;
};

} // namespace p3com
} // namespace iox

#endif // P3COM_IOX_TO_TRANSPORT_HPP
