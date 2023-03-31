// Copyright 2023 NXP

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
