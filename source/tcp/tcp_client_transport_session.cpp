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

#include "p3com/transport/tcp/tcp_client_transport_session.hpp"

constexpr std::chrono::seconds iox::p3com::tcp::TCPClientTransportSession::M_RETRY_TIMEOUT;

iox::p3com::tcp::TCPClientTransportSession::TCPClientTransportSession(
    asio::io_service& io_service,
    const iox::p3com::tcp::TCPTransportSession::dataCallback_t& dataCallbackHandler,
    const iox::p3com::tcp::TCPTransportSession::sessionClosedCallback_t& sessionClosedHandler,
    const sessionOpenCallback_t& sessionOpenHandler,
    asio::ip::tcp::endpoint remote_endpoint,
    int max_reconnection_attempts) noexcept
    : TCPTransportSession(io_service, dataCallbackHandler, sessionClosedHandler)
    , m_resolver(io_service)
    , m_remoteEndpoint(remote_endpoint)
    , m_retriesLeft(max_reconnection_attempts)
    , m_retryTimer(io_service, M_RETRY_TIMEOUT)
    , m_sessionOpenCallback(std::move(sessionOpenHandler))
{
}

asio::ip::tcp::endpoint iox::p3com::tcp::TCPClientTransportSession::remoteEndpoint() noexcept
{
    return m_remoteEndpoint;
}

void iox::p3com::tcp::TCPClientTransportSession::start() noexcept
{
    connectToEndpoint();
}

void iox::p3com::tcp::TCPClientTransportSession::connectToEndpoint() noexcept
{
    getSocket().async_connect(m_remoteEndpoint, [this](std::error_code ec) {
        if (ec)
        {
            iox::p3com::LogError() << "[TCPTransport] " << ec.message();
            connectionFailedHandler();
        }
        else
        {
            m_sessionOpenCallback(this);
            receiveTcpData();
        }
    });
}

void iox::p3com::tcp::TCPClientTransportSession::connectionFailedHandler() noexcept
{
    asio::error_code ec;
    getSocket().close(ec);
    if (ec)
    {
        iox::p3com::LogError() << "[TCPTransport] " << ec.message();
    }

    if (m_retriesLeft > 0)
    {
        m_retriesLeft--;
        // Retry connection after a short time
        m_retryTimer.async_wait([this](std::error_code ec) {
            if (ec)
            {
                iox::p3com::LogError() << "[TCPTransport] " << ec.message();
                sessionClosedHandler();
            }
            else
            {
                connectToEndpoint();
            }
        });
    }
    else
    {
        sessionClosedHandler();
    }
}
