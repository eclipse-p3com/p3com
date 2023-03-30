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

#include "p3com/transport/tcp/tcp_server_transport_session.hpp"
#include "p3com/internal/log/logging.hpp"

iox::p3com::tcp::TCPServerTransportSession::TCPServerTransportSession(
    asio::io_service& io_service,
    const dataCallback_t& dataCallbackHandler,
    const sessionClosedCallback_t& sessionClosedHandler) noexcept
    : TCPTransportSession(io_service, dataCallbackHandler, sessionClosedHandler)
{
}

asio::ip::tcp::endpoint iox::p3com::tcp::TCPServerTransportSession::remoteEndpoint() noexcept
{
    if (m_remoteEndpoint.address() == asio::ip::address_v4::any())
    {
        asio::error_code ec;
        m_remoteEndpoint = getSocket().remote_endpoint(ec);
        if (ec)
        {
            iox::p3com::LogError() << "[TCPTransport] " << ec.message();
        }
    }
    return m_remoteEndpoint;
}

void iox::p3com::tcp::TCPServerTransportSession::start() noexcept
{
    try
    {
        // server side of connection
        getSocket().set_option(asio::ip::tcp::no_delay(true));
        receiveTcpData();
    }
    catch (std::exception& e)
    {
        iox::p3com::LogError() << "[TCPTransport] " << e.what();
    }
}
