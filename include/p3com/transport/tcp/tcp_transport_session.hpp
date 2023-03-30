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

#ifndef IOX_TCP_TRANSPORT_SESSION_HPP
#define IOX_TCP_TRANSPORT_SESSION_HPP

#include <asio.hpp>

#include <functional>
#include <string>

namespace iox
{
namespace p3com
{
namespace tcp
{
class TCPTransportSession
{
  public:
    using dataCallback_t = std::function<void(const void*, size_t, asio::ip::tcp::endpoint)>;
    using sessionClosedCallback_t = std::function<void(TCPTransportSession*)>;

    TCPTransportSession(asio::io_service& io_service,
                        const dataCallback_t& dataCallbackHandler,
                        const sessionClosedCallback_t& sessionClosedHandler) noexcept;

    // Copy
    TCPTransportSession(const TCPTransportSession&) = delete;
    TCPTransportSession& operator=(const TCPTransportSession&) = delete;

    // Move
    TCPTransportSession& operator=(TCPTransportSession&&) = delete;
    TCPTransportSession(TCPTransportSession&&) = delete;

    virtual ~TCPTransportSession();

    virtual asio::ip::tcp::endpoint remoteEndpoint() noexcept = 0;
    virtual void start() noexcept = 0;

    asio::ip::tcp::socket& getSocket() noexcept;
    std::string endpointToString() noexcept;
    std::string remoteEndpointToString() noexcept;
    bool sendData(const void* data1, size_t size1, const void* data2, size_t size2) noexcept;

    static constexpr size_t MAX_PACKET_SIZE = 65535U; // 64 kB

  private:
    const sessionClosedCallback_t m_sessionClosedCallback;
    const dataCallback_t m_dataCallback;
    asio::ip::tcp::socket m_dataSocket;
    size_t m_readBufferSize;
    std::array<uint8_t, MAX_PACKET_SIZE> m_readBuffer;

  protected:
    void receiveTcpData() noexcept;
    void sessionClosedHandler() noexcept;
};

} // namespace tcp
} // namespace p3com
} // namespace iox

#endif // IOX_TCP_TRANSPORT_SESSION_HPP
