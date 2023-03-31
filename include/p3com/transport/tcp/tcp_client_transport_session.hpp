// Copyright 2023 NXP

#ifndef IOX_TCP_CLIENT_TRANSPORT_SESSION_HPP
#define IOX_TCP_CLIENT_TRANSPORT_SESSION_HPP

#include "p3com/internal/log/logging.hpp"
#include "p3com/transport/tcp/tcp_transport_session.hpp"

#include <asio.hpp>

#include <functional>
#include <string>

namespace iox
{
namespace p3com
{
namespace tcp
{
class TCPClientTransportSession : public TCPTransportSession
{
  public:
    using sessionOpenCallback_t = std::function<void(TCPTransportSession *)>;

    TCPClientTransportSession(asio::io_service& io_service,
                              const dataCallback_t& dataCallbackHandler,
                              const sessionClosedCallback_t& sessionClosedHandler,
                              const sessionOpenCallback_t& sessionOpenHandler,
                              asio::ip::tcp::endpoint remote_endpoint = asio::ip::tcp::endpoint(),
                              int max_reconnection_attempts = 3) noexcept;

    asio::ip::tcp::endpoint remoteEndpoint() noexcept override;
    void start() noexcept override;

  private:
    static constexpr std::chrono::seconds M_RETRY_TIMEOUT{1};

    asio::ip::tcp::resolver m_resolver;
    asio::ip::tcp::endpoint m_remoteEndpoint;
    int m_retriesLeft;
    asio::steady_timer m_retryTimer;
    const sessionOpenCallback_t m_sessionOpenCallback;

    void connectToEndpoint() noexcept;
    void connectionFailedHandler() noexcept;
};

} // namespace tcp
} // namespace p3com
} // namespace iox

#endif // IOX_TCP_CLIENT_TRANSPORT_SESSION_HPP
