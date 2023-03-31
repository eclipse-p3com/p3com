// Copyright 2023 NXP

#ifndef IOX_UDP_TRANSPORT_HPP
#define IOX_UDP_TRANSPORT_HPP

#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_hoofs/cxx/vector.hpp"

#include "p3com/generic/config.hpp"
#include "p3com/generic/types.hpp"
#include "p3com/transport/transport.hpp"
#include "p3com/transport/udp/udp_transport_broadcast.hpp"

#include <asio.hpp>

#include <array>
#include <cstdint>
#include <mutex>
#include <thread>

namespace iox
{
namespace p3com
{
namespace udp
{
class UDPTransport : public TransportLayer
{
  public:
    UDPTransport() noexcept;
    ~UDPTransport() override;

    UDPTransport(const UDPTransport&) = delete;
    UDPTransport& operator=(const UDPTransport&) = delete;
    UDPTransport(UDPTransport&&) = delete;
    UDPTransport& operator=(UDPTransport&&) = delete;

    void registerDiscoveryCallback(remoteDiscoveryCallback_t callback) noexcept override;
    void registerUserDataCallback(userDataCallback_t callback) noexcept override;

    void sendBroadcast(const void* data, size_t size) noexcept override;
    bool sendUserData(
        const void* data1, size_t size1, const void* data2, size_t size2, uint32_t deviceIndex) noexcept override;

    size_t maxMessageSize() const noexcept override;
    TransportType getType() const noexcept override;

  private:
    static constexpr uint16_t DATA_PORT = 9333U;
    static constexpr size_t MAX_DATAGRAM_SIZE = 32768U; // 32 kB

    void dataSocketCallback(asio::error_code ec, size_t bytes) noexcept;
    void dataAsyncReceive() noexcept;

    asio::io_service m_context;
    cxx::optional<asio::io_service::work> m_work;
    std::thread m_thread;

    std::mutex m_socketMutex;
    asio::ip::udp::socket m_dataSocket;
    UDPBroadcast m_broadcast;

    std::array<uint8_t, MAX_DATAGRAM_SIZE> m_outputBuffer;
    asio::ip::udp::endpoint m_outputEndpoint;

    userDataCallback_t m_userDataCallback;
};

} // namespace udp
} // namespace p3com
} // namespace iox

#endif // IOX_UDP_TRANSPORT_HPP
