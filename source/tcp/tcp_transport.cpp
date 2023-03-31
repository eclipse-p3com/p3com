// Copyright 2023 NXP

#include "p3com/transport/tcp/tcp_transport.hpp"
#include "p3com/generic/types.hpp"
#include "p3com/internal/log/logging.hpp"
#include "p3com/transport/transport.hpp"
#include <p3com/transport/tcp/tcp_client_transport_session.hpp>
#include <p3com/transport/tcp/tcp_server_transport_session.hpp>

#include <asio.hpp>

#include <cstdint>
#include <stdexcept>
#include <thread>

iox::p3com::tcp::TCPTransport::TCPTransport() noexcept
    : m_context()
    , m_dataAcceptor(m_context)
    , m_broadcast(m_context)
    , m_serverListeningSession(nullptr)
{
    auto endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), DATA_PORT);
    try
    {
        m_dataAcceptor.open(endpoint.protocol());
        m_dataAcceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
        m_dataAcceptor.bind(endpoint);
        m_dataAcceptor.listen(asio::socket_base::max_listen_connections);
    }
    catch (std::exception& e)
    {
        iox::p3com::LogError() << "[TCPTransport] " << e.what();
        setFailed();
        return;
    }
    m_thread = std::thread([this]() {
        try
        {
            m_work.emplace(m_context);
            m_context.run();

            iox::p3com::LogInfo() << "[TCPTransport] Worker thread has exited";
        }
        catch (std::exception& e)
        {
            iox::p3com::LogError() << "[TCPTransport] " << e.what();
            setFailed();
            return;
        }
    });

    startAccept();
    m_broadcast.registerDiscoveryCallback([this](const void* data, size_t size, DeviceIndex_t deviceIndex) {
        udpDiscoveryCallback(data, size, deviceIndex);
    });

    iox::p3com::LogInfo() << "[TCPTransport] Created TCP listener and waiting for clients on endpoint "
                        << endpoint.address().to_string() << ":" << std::to_string(endpoint.port());
}

iox::p3com::tcp::TCPTransport::~TCPTransport()
{
    m_work.reset();
    m_context.stop();
    m_thread.join();
}


void iox::p3com::tcp::TCPTransport::addSession(asio::ip::tcp::endpoint& endpoint) noexcept
{
    // Add the session to the session list
    m_transportSessions.emplace_back(std::make_unique<TCPClientTransportSession>(
        m_context, handleUserDataCallback(this), handleSessionClose(this), handleSessionOpen(this), endpoint));
    m_transportSessions.back()->start();
}

void iox::p3com::tcp::TCPTransport::addSession() noexcept
{
    m_transportSessions.push_back(std::move(m_serverListeningSession));
    m_transportSessions.back()->start();
    handleSessionOpen(this)(m_transportSessions.back().get());
}

void iox::p3com::tcp::TCPTransport::startAccept() noexcept
{
    m_serverListeningSession =
        std::make_unique<TCPServerTransportSession>(m_context, handleUserDataCallback(this), handleSessionClose(this));


    m_dataAcceptor.async_accept(m_serverListeningSession->getSocket(), [this](asio::error_code ec) {
        if (ec)
        {
            iox::p3com::LogError() << "[TCPTransport] " << ec.message();
            setFailed();
        }
        else
        {
            iox::p3com::LogInfo() << "[TCPTransport] New connection request from client "
                                << m_serverListeningSession->remoteEndpointToString();
            std::lock_guard<std::mutex> transportSessionsLock(m_transportSessionsMutex);

            auto iter = findSession(m_serverListeningSession->remoteEndpoint());
            if (iter == m_transportSessions.end())
            {
                // device not found, add it
                addSession();
            }
            else
            {
                iox::p3com::LogWarn() << "[TCPTransport] Already connected to "
                                    << m_serverListeningSession->remoteEndpointToString()
                                    << " as client, closing server connection.";
            }
            startAccept();
        }
    });
}


iox::p3com::tcp::TCPServerTransportSession::sessionClosedCallback_t
iox::p3com::tcp::TCPTransport::handleSessionClose(iox::p3com::tcp::TCPTransport* self) noexcept
{
    return [self](TCPTransportSession* session) {
        std::lock_guard<std::mutex> transportSessionsLock(self->m_transportSessionsMutex);
        auto session_it = std::find_if(self->m_transportSessions.begin(),
                                       self->m_transportSessions.end(),
                                       [session](const auto& iter) { return iter.get() == session; });
        if (session_it != self->m_transportSessions.end())
        {
            iox::p3com::LogInfo() << "[TCPTransport] Successfully removed session " << session->remoteEndpointToString();
            self->m_transportSessions.erase(session_it);
        }
    };
}

void iox::p3com::tcp::TCPTransport::udpDiscoveryCallback(const void* data,
                                                       size_t size,
                                                       DeviceIndex_t deviceIndex) noexcept
{
    auto endpoint = m_broadcast.getEndpoint(deviceIndex.device);
    std::lock_guard<std::mutex> transportSessionsLock(m_transportSessionsMutex);
    auto tcpEndpoint = asio::ip::tcp::endpoint(endpoint.address(), DATA_PORT);
    auto iter = findSession(tcpEndpoint);
    if (iter == m_transportSessions.end())
    {
        // device not found, add it
        iox::p3com::LogInfo() << "[TCPTransport] Discovered remote GW, not yet registered, adding "
                            << endpoint.address().to_string();
        iox::cxx::vector<uint8_t, iox::p3com::maxPubSubInfoSerializationSize()> serializedInfo(size);
        std::memcpy(serializedInfo.data(), data, size);
        m_infoToReport.emplace_back(tcpEndpoint, serializedInfo);
        addSession(tcpEndpoint);
    }
    else
    {
        remoteDiscoveryHandler(data, size, static_cast<uint32_t>(std::distance(m_transportSessions.begin(), iter)));
    }
}
std::unique_ptr<iox::p3com::tcp::TCPTransportSession>*
iox::p3com::tcp::TCPTransport::findSession(const asio::ip::tcp::endpoint& endpoint) noexcept
{
    return std::find_if(m_transportSessions.begin(), m_transportSessions.end(), [&endpoint](auto& iter) {
        return iter->remoteEndpoint().address() == endpoint.address();
    });
}
void iox::p3com::tcp::TCPTransport::remoteDiscoveryHandler(const void* data,
                                                         size_t size,
                                                         const uint32_t device) const noexcept
{
    if (m_remoteDiscoveryCallback)
    {
        m_remoteDiscoveryCallback(data, size, {TransportType::TCP, device});
    }
}

void iox::p3com::tcp::TCPTransport::registerDiscoveryCallback(iox::p3com::remoteDiscoveryCallback_t callback) noexcept
{
    m_remoteDiscoveryCallback = std::move(callback);
}

void iox::p3com::tcp::TCPTransport::registerUserDataCallback(iox::p3com::userDataCallback_t callback) noexcept
{
    m_userDataCallback = std::move(callback);
}


void iox::p3com::tcp::TCPTransport::sendBroadcast(const void* data, size_t size) noexcept
{
    m_broadcast.sendBroadcast(data, size);
}

bool iox::p3com::tcp::TCPTransport::sendUserData(
    const void* data1, size_t size1, const void* data2, size_t size2, uint32_t deviceIndex) noexcept
{
    if (deviceIndex >= m_transportSessions.size())
    {
        iox::p3com::LogWarn() << "[TCPTransport] Invalid device index when sending user data";
        return false;
    }
    iox::p3com::LogInfo() << "[TCPTransport] Sending data to "
                        << m_transportSessions[deviceIndex]->remoteEndpointToString();
    return m_transportSessions[deviceIndex]->sendData(data1, size1, data2, size2);
}

size_t iox::p3com::tcp::TCPTransport::maxMessageSize() const noexcept
{
    return TCPTransportSession::MAX_PACKET_SIZE;
}

iox::p3com::TransportType iox::p3com::tcp::TCPTransport::getType() const noexcept
{
    return iox::p3com::TransportType::TCP;
}

iox::p3com::tcp::TCPTransportSession::dataCallback_t
iox::p3com::tcp::TCPTransport::handleUserDataCallback(iox::p3com::tcp::TCPTransport* self) noexcept
{
    return [self](const void* data, size_t size, asio::ip::tcp::endpoint endpoint) {
        const auto index = self->m_broadcast.getIndex(endpoint.address());
        if (index.has_value())
        {
            if (self->m_userDataCallback)
            {
                self->m_userDataCallback(data, size, {iox::p3com::TransportType::TCP, *index});
            }
        }
        else
        {
            iox::p3com::LogError() << "[TCPTransport] Received user data message from an unknown device! Discarding!";
        }
    };
}

iox::p3com::tcp::TCPClientTransportSession::sessionOpenCallback_t
iox::p3com::tcp::TCPTransport::handleSessionOpen(iox::p3com::tcp::TCPTransport* self) noexcept
{
    return [self](TCPTransportSession* session) {
        LogInfo() << "[TCPTransport] New connection established: " << session->endpointToString()
                  << " Sessions: " << self->m_transportSessions.size();
        auto infoToReportIt =
            std::find_if(self->m_infoToReport.begin(), self->m_infoToReport.end(), [session](const auto& iter) {
                return iter.first.address() == session->remoteEndpoint().address();
            });

        if (infoToReportIt != self->m_infoToReport.end())
        {
            auto iter = self->findSession(session->remoteEndpoint());
            const auto deviceIdx = static_cast<uint32_t>(std::distance(self->m_transportSessions.begin(), iter));
            const auto& vec = infoToReportIt->second;
            self->remoteDiscoveryHandler(vec.data(), vec.size(), deviceIdx);
            self->m_infoToReport.erase(infoToReportIt);
        }
    };
}
