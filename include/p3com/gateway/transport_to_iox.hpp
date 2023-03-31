// Copyright 2023 NXP

#ifndef P3COM_TRANSPORT_TO_IOX_HPP
#define P3COM_TRANSPORT_TO_IOX_HPP

#include "iceoryx_posh/popo/untyped_publisher.hpp"

#include "p3com/gateway/gateway.hpp"
#include "p3com/generic/data_reader.hpp"
#include "p3com/generic/discovery.hpp"
#include "p3com/generic/segmented_messages.hpp"
#include "p3com/generic/transport_forwarder.hpp"
#include "p3com/generic/types.hpp"
#include "p3com/transport/transport.hpp"
#include "p3com/utility/vector_map.hpp"

#include <chrono>

namespace iox
{
namespace p3com
{
///
/// @brief p3com gateway implementation for the transport to iceoryx direction.
///
class Transport2Iceoryx : public Gateway<popo::UntypedPublisher>
{
  public:
    explicit Transport2Iceoryx(DiscoveryManager& discovery,
                                TransportForwarder& transportForwarder,
                                SegmentedMessageManager& segmentedMessageManager) noexcept;

    void updateChannels(const ServiceVector_t& services) noexcept;

  private:
    void setupChannel(const capro::ServiceDescription& service) noexcept;
    void deleteChannel(const capro::ServiceDescription& service) noexcept;

    void receive(const void* receivedUserPayload, size_t size, DeviceIndex_t deviceIndex) noexcept;
    void* loanBuffer(const void* serializedDatagramHeader, size_t size) noexcept;
    void releaseBuffer(const void* serializedDatagramHeader,
                       size_t size,
                       bool shoudRelease,
                       DeviceIndex_t deviceIndex) noexcept;

    DiscoveryManager& m_discovery;
    TransportForwarder& m_transportForwarder;
    SegmentedMessageManager& m_segmentedMessageManager;
};

} // namespace p3com
} // namespace iox

#endif // P3COM_TRANSPORT_TO_IOX_HPP
