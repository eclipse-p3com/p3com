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

#ifndef IOX_TCP_TRANSPORT_HPP
#define IOX_TCP_TRANSPORT_HPP

#include "p3com/transport/tcp/tcp_transport_session.hpp"
#include "p3com/transport/tcp/tcp_client_transport_session.hpp"
#include "p3com/transport/transport.hpp"
#include "p3com/transport/udp/udp_transport_broadcast.hpp"
#include "p3com/generic/serialization.hpp"

#include <asio.hpp>

#include <thread>

namespace iox
{
namespace p3com
{
namespace tcp
{
class TCPTransport : public TransportLayer
{
  public:
    TCPTransport() noexcept;
    TCPTransport(const TCPTransport&) = delete;
    TCPTransport(TCPTransport&&) = delete;
    TCPTransport& operator=(const TCPTransport&) = delete;
    TCPTransport& operator=(TCPTransport&&) = delete;

    ~TCPTransport() override;

    void registerDiscoveryCallback(remoteDiscoveryCallback_t callback) noexcept override;
    void registerUserDataCallback(userDataCallback_t callback) noexcept override;

    void sendBroadcast(const void* data, size_t size) noexcept override;
    bool sendUserData(
        const void* data1, size_t size1, const void* data2, size_t size2, uint32_t deviceIndex) noexcept override;

    size_t maxMessageSize() const noexcept override;
    TransportType getType() const noexcept override;

  private:
    static constexpr uint16_t DATA_PORT = 9333U;

    asio::io_service m_context;
    cxx::optional<asio::io_service::work> m_work;
    std::thread m_thread;

    asio::ip::tcp::acceptor m_dataAcceptor;
    udp::UDPBroadcast m_broadcast;
    mutable std::mutex m_transportSessionsMutex;

    cxx::vector<std::unique_ptr<TCPTransportSession>, MAX_DEVICE_COUNT> m_transportSessions;
    cxx::vector<std::pair<asio::ip::tcp::endpoint, cxx::vector<uint8_t, maxPubSubInfoSerializationSize()>>,
                MAX_DEVICE_COUNT>
        m_infoToReport;
    std::unique_ptr<TCPTransportSession> m_serverListeningSession;

    userDataCallback_t m_userDataCallback;
    remoteDiscoveryCallback_t m_remoteDiscoveryCallback;

    void startAccept() noexcept;
    void addSession() noexcept;
    void addSession(asio::ip::tcp::endpoint& endpoint) noexcept;

    void udpDiscoveryCallback(const void* epIter, size_t size, DeviceIndex_t deviceIndex) noexcept;
    void remoteDiscoveryHandler(const void* data, size_t size, const uint32_t device) const noexcept;

    static TCPTransportSession::dataCallback_t handleUserDataCallback(TCPTransport* self) noexcept;
    static TCPTransportSession::sessionClosedCallback_t handleSessionClose(TCPTransport* self) noexcept;
    static TCPClientTransportSession::sessionOpenCallback_t handleSessionOpen(TCPTransport* self) noexcept;
    std::unique_ptr<tcp::TCPTransportSession>* findSession(const asio::ip::tcp::endpoint& endpoint) noexcept;
};

} // namespace tcp
} // namespace p3com
} // namespace iox

#endif // IOX_TCP_TRANSPORT_HPP
