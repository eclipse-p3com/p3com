// Copyright 2023 NXP

#include "p3com/transport/tcp/tcp_transport_session.hpp"
#include "p3com/internal/log/logging.hpp"

#include <asio.hpp>

#include <cstdint>
#include <thread>

iox::p3com::tcp::TCPTransportSession::TCPTransportSession(asio::io_service& io_service,
                                                        const dataCallback_t& dataCallbackHandler,
                                                        const sessionClosedCallback_t& sessionClosedHandler) noexcept
    : m_sessionClosedCallback(std::move(sessionClosedHandler))
    , m_dataCallback(std::move(dataCallbackHandler))
    , m_dataSocket(io_service)
{
}

iox::p3com::tcp::TCPTransportSession::~TCPTransportSession()
{
}

asio::ip::tcp::socket& iox::p3com::tcp::TCPTransportSession::getSocket() noexcept
{
    return m_dataSocket;
}

std::string iox::p3com::tcp::TCPTransportSession::endpointToString() noexcept
{
    std::stringstream sstream;
    sstream << m_dataSocket.local_endpoint() << " <- " << remoteEndpoint();
    return sstream.str();
}

std::string iox::p3com::tcp::TCPTransportSession::remoteEndpointToString() noexcept
{
    std::stringstream sstream;
    sstream << remoteEndpoint();
    return sstream.str();
}

bool iox::p3com::tcp::TCPTransportSession::sendData(const void* data1,
                                                  size_t size1,
                                                  const void* data2,
                                                  size_t size2) noexcept
{
    try
    {
        // First, we write a size_t integer with the size of the following message
        const size_t totalSize = size1 + size2;
        const auto writtenSize = asio::write(m_dataSocket, asio::const_buffer{&totalSize, sizeof(totalSize)});
        cxx::Expects(writtenSize == sizeof(totalSize));

        // Next, we actually write the message
        constexpr uint32_t BUFFER_COUNT = 2U;
        std::array<asio::const_buffer, BUFFER_COUNT> buffers{asio::const_buffer{data1, size1},
                                                             asio::const_buffer{data2, size2}};
        const auto writtenData = asio::write(m_dataSocket, buffers);
        cxx::Expects(writtenData == totalSize);
    }
    catch (std::exception& e)
    {
        iox::p3com::LogWarn() << "[TCPTransport] " << e.what();
    }
    return false;
}

void iox::p3com::tcp::TCPTransportSession::receiveTcpData() noexcept
{
    const auto processErrorCode = [this](std::error_code ec) {
        if (ec)
        {
            iox::p3com::LogInfo() << "[TCPTransport] " << ec.message() << ", going to close socket "
                                << remoteEndpointToString();
            sessionClosedHandler();
            return true;
        }
        else
        {
            return false;
        }
    };

    // After we have read the size of the full message, we read the actual message payload
    auto readInner = [this, processErrorCode](std::error_code ec, std::size_t size) {
        if (!processErrorCode(ec))
        {
            iox::p3com::LogInfo() << "[TCPTransport] Received data from " << remoteEndpointToString();
            m_dataCallback(m_readBuffer.data(), size, remoteEndpoint());
            receiveTcpData();
        }
    };

    const auto readOuter = [this, processErrorCode, readInner = std::move(readInner)](std::error_code ec, std::size_t) {
        if (!processErrorCode(ec))
        {
            // m_readBufferSize now stores the size of the following message, we need to read exactly this many bytes
            asio::async_read(m_dataSocket, asio::buffer(m_readBuffer, m_readBufferSize), std::move(readInner));
        }
    };

    // We read a size_t integer with the size of the following message into m_readBufferSize
    asio::async_read(m_dataSocket, asio::buffer(&m_readBufferSize, sizeof(m_readBufferSize)), std::move(readOuter));
}

void iox::p3com::tcp::TCPTransportSession::sessionClosedHandler() noexcept
{
    m_dataSocket.close();
    m_sessionClosedCallback(this);
}
