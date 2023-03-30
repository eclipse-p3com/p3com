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
