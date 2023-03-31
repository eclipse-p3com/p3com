// Copyright 2023 NXP

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
