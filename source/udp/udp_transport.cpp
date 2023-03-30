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

#include "p3com/generic/types.hpp"
#include "p3com/internal/log/logging.hpp"
#include "p3com/transport/transport.hpp"

#include "p3com/transport/udp/udp_transport.hpp"

#include <asio.hpp>

#include <cstdint>
#include <stdexcept>
#include <thread>


iox::p3com::udp::UDPTransport::UDPTransport() noexcept
    : m_context()
    , m_dataSocket(m_context, asio::ip::udp::endpoint(asio::ip::address_v4::any(), DATA_PORT))
    , m_broadcast(m_context)
{
    try
    {
        // TODO: What are the best values for send and receive buffer sizes?
        constexpr uint32_t SEND_BUFFER_SIZE = 16 * 1024 * 1024;
        constexpr uint32_t RECEIVE_BUFFER_SIZE = 32 * 1024 * 1024;
        m_dataSocket.set_option(asio::socket_base::send_buffer_size(SEND_BUFFER_SIZE));
        m_dataSocket.set_option(asio::socket_base::receive_buffer_size(RECEIVE_BUFFER_SIZE));
    }
    catch (std::exception& e)
    {
        iox::p3com::LogError() << "[UDPTransport] " << e.what();
        setFailed();
        return;
    }

    m_thread = std::thread([this]() {
        try
        {
            m_work.emplace(m_context);
            m_context.run();

            iox::p3com::LogInfo() << "[UDPTransport] Worker thread has exited";
        }
        catch (std::exception& e)
        {
            iox::p3com::LogError() << "[UDPTransport] " << e.what();
            setFailed();
            return;
        }
    });

    dataAsyncReceive();
}

iox::p3com::udp::UDPTransport::~UDPTransport()
{
    m_work.reset();
    m_context.stop();
    m_thread.join();
}

void iox::p3com::udp::UDPTransport::dataSocketCallback(asio::error_code ec, size_t bytes) noexcept
{
    if (ec)
    {
        iox::p3com::LogError() << "[UDPTransport] " << ec.message();
        setFailed();
        return;
    }
    iox::p3com::LogInfo() << "[UDPTransport] Received user data message from IP "
                        << m_outputEndpoint.address().to_string();
    const auto index = m_broadcast.getIndex(m_outputEndpoint.address());
    if (index.has_value())
    {
        if (m_userDataCallback)
        {
            m_userDataCallback(m_outputBuffer.data(), bytes, {iox::p3com::TransportType::UDP, *index});
        }
    }
    else
    {
        iox::p3com::LogError() << "[UDPTransport] Received user data message from an unknown device! Discarding!";
    }

    dataAsyncReceive();
}

void iox::p3com::udp::UDPTransport::registerDiscoveryCallback(iox::p3com::remoteDiscoveryCallback_t callback) noexcept
{
    m_broadcast.registerDiscoveryCallback(callback);
}

void iox::p3com::udp::UDPTransport::registerUserDataCallback(iox::p3com::userDataCallback_t callback) noexcept
{
    m_userDataCallback = std::move(callback);
}

void iox::p3com::udp::UDPTransport::dataAsyncReceive() noexcept
{
    try
    {
        m_dataSocket.async_receive_from(
            asio::buffer(m_outputBuffer), m_outputEndpoint, [this](asio::error_code ec, size_t bytes) {
                dataSocketCallback(ec, bytes);
            });
    }
    catch (std::exception& e)
    {
        iox::p3com::LogError() << "[UDPTransport] " << e.what();
        setFailed();
    }
}

void iox::p3com::udp::UDPTransport::sendBroadcast(const void* data, size_t size) noexcept
{
    m_broadcast.sendBroadcast(data, size);
}

bool iox::p3com::udp::UDPTransport::sendUserData(
    const void* data1, size_t size1, const void* data2, size_t size2, uint32_t deviceIndex) noexcept
{
    auto endpoint = m_broadcast.getEndpoint(deviceIndex);
    endpoint.port(DATA_PORT);
    try
    {
        constexpr uint32_t BUFFER_COUNT = 2U;
        std::array<asio::const_buffer, BUFFER_COUNT> buffers{asio::const_buffer{data1, size1},
                                                             asio::const_buffer{data2, size2}};
        std::lock_guard<std::mutex> lock(m_socketMutex);
        m_dataSocket.send_to(buffers, endpoint);

        iox::p3com::LogInfo() << "[UDPTransport] Sent user data message to IP " << endpoint.address().to_string()
                            << " with index " << deviceIndex;
    }
    catch (std::exception& e)
    {
        iox::p3com::LogError() << "[UDPTransport] " << e.what();
        setFailed();
    }
    return false;
}

size_t iox::p3com::udp::UDPTransport::maxMessageSize() const noexcept
{
    return MAX_DATAGRAM_SIZE;
}

iox::p3com::TransportType iox::p3com::udp::UDPTransport::getType() const noexcept
{
    return iox::p3com::TransportType::UDP;
}
