// Copyright 2023 NXP

#ifndef IOX_TCP_SERVER_TRANSPORT_SESSION_HPP
#define IOX_TCP_SERVER_TRANSPORT_SESSION_HPP

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
class TCPServerTransportSession : public TCPTransportSession
{
  public:
    TCPServerTransportSession(asio::io_service& io_service,
                              const dataCallback_t& dataCallbackHandler,
                              const sessionClosedCallback_t& sessionClosedHandler) noexcept;

    asio::ip::tcp::endpoint remoteEndpoint() noexcept override;
    void start() noexcept override;

  private:
    asio::ip::tcp::endpoint m_remoteEndpoint;
};

} // namespace tcp
} // namespace p3com
} // namespace iox

#endif // IOX_TCP_SERVER_TRANSPORT_SESSION_HPP
