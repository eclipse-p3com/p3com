// Copyright 2023 NXP

#include "p3com/transport/udp/udp_transport_broadcast.hpp"
#include "p3com/internal/log/logging.hpp"

#include <asio.hpp>

#include <cstdint>
#include <ifaddrs.h>
#include <stdexcept>

iox::p3com::udp::UDPBroadcast::UDPBroadcast(asio::io_service& context) noexcept
    : m_discoverySocket(context, asio::ip::udp::endpoint(asio::ip::address_v4::any(), DISCOVERY_PORT))
    , m_outputBuffer()
{
    discoverBroadcastAddresses();
    try
    {
        // This is taken from https://stackoverflow.com/questions/9310231/boostasio-udp-broadcasting
        m_discoverySocket.set_option(asio::ip::udp::socket::reuse_address(true));
        m_discoverySocket.set_option(asio::socket_base::broadcast(true));
    }
    catch (std::exception& e)
    {
        iox::p3com::LogError() << "[UDPBroadcast] " << e.what();
        setFailed();
        return;
    }
    discoveryAsyncReceive();
}

void iox::p3com::udp::UDPBroadcast::discoverBroadcastAddresses() noexcept
{
    struct ifaddrs* ifap;
    if (getifaddrs(&ifap) == 0)
    {
        uint32_t iface_n = 0;
        for (struct ifaddrs* iface = ifap; iface != nullptr && iface_n < MAX_NETWORK_IFACE_COUNT;
             iface = iface->ifa_next)
        {
            uint32_t ifaAddr = sockAddrToUint32(iface->ifa_addr);
            uint32_t maskAddr = sockAddrToUint32(iface->ifa_netmask);
            uint32_t dstAddr = sockAddrToUint32(iface->ifa_dstaddr);
            if (ifaAddr > 0)
            {
                auto ifaAddrStr = inetNtoA(ifaAddr);
                auto maskAddrStr = inetNtoA(maskAddr);
                auto dstAddrStr = inetNtoA(dstAddr);
                iox::p3com::LogInfo() << "[UDPBroadcast] Found interface:"
                                    << "name=[" << iface->ifa_name << "]"
                                    << "address=[" << ifaAddrStr << "]"
                                    << "netmask=[" << maskAddrStr << "]"
                                    << "broadcastAddr=[" << dstAddrStr << "]";
                auto ifEp = asio::ip::udp::endpoint(asio::ip::address_v4(ifaAddr), DISCOVERY_PORT);
                auto bcEp = asio::ip::udp::endpoint(asio::ip::address_v4(dstAddr), DISCOVERY_PORT);
                if (!bcEp.address().is_loopback())
                {
                    m_interfaceEndpoints.push_back(ifEp);
                    m_broadcastEndpoints.push_back(bcEp);
                    iface_n++;
                }
            }
        }
        freeifaddrs(ifap);
    }
}


uint32_t iox::p3com::udp::UDPBroadcast::sockAddrToUint32(struct sockaddr* address) noexcept
{
    if ((address != nullptr) && (address->sa_family == AF_INET))
    {
        return ntohl((reinterpret_cast<struct sockaddr_in*>(address))->sin_addr.s_addr);
    }
    else
    {
        return 0;
    }
}


// convert a numeric IP address into its string representation
std::string iox::p3com::udp::UDPBroadcast::inetNtoA(uint32_t addr) noexcept
{
    return iox::cxx::convert::toString((addr >> 24) & 0xFF) + "." + iox::cxx::convert::toString((addr >> 16) & 0xFF)
           + "." + iox::cxx::convert::toString((addr >> 8) & 0xFF) + "."
           + iox::cxx::convert::toString((addr >> 0) & 0xFF);
}

void iox::p3com::udp::UDPBroadcast::discoverySocketCallback(asio::error_code ec, size_t bytes) noexcept
{
    if (ec)
    {
        iox::p3com::LogError() << "[UDPBroadcast] " << ec.message();
        setFailed();
        return;
    }

    auto* local_it = std::find(m_interfaceEndpoints.begin(), m_interfaceEndpoints.end(), m_outputEndpoint);
    // We assume that the first received discovery message was actually from the
    // ego device. So, m_outputEndpoint has not to be amongst m_interfaceEndpoints => local endpoint is ignored
    if (local_it == m_interfaceEndpoints.end())
    {
        // Find index of the endpoint. If iter is not registered, add iter.
        auto* iter = std::find(m_devices.begin(), m_devices.end(), m_outputEndpoint);
        if (iter == m_devices.end() && m_devices.size() < MAX_DEVICE_COUNT)
        {
            m_devices.push_back(m_outputEndpoint);
            iter = &m_devices.back();
        }
        const auto device = static_cast<uint32_t>(std::distance(m_devices.begin(), iter));
        iox::p3com::LogDebug() << "[UDPBroadcast] Received discovery message from IP "
                             << m_outputEndpoint.address().to_string() << " with index " << device;

        if (m_remoteDiscoveryCallback)
        {
            m_remoteDiscoveryCallback(m_outputBuffer.data(), bytes, {iox::p3com::TransportType::UDP, device});
        }
    }
    else
    {
        iox::p3com::LogDebug() << "[UDPBroadcast] Received discovery message from myself: "
                             << m_outputEndpoint.address().to_string() << " ignoring";
    }
    discoveryAsyncReceive();
}

void iox::p3com::udp::UDPBroadcast::registerDiscoveryCallback(iox::p3com::remoteDiscoveryCallback_t callback) noexcept
{
    m_remoteDiscoveryCallback = std::move(callback);
}

void iox::p3com::udp::UDPBroadcast::discoveryAsyncReceive() noexcept
{
    try
    {
        m_discoverySocket.async_receive_from(
            asio::buffer(m_outputBuffer), m_outputEndpoint, [this](asio::error_code ec, size_t bytes) {
                discoverySocketCallback(ec, bytes);
            });
    }
    catch (std::exception& e)
    {
        iox::p3com::LogError() << "[UDPBroadcast] " << e.what();
        setFailed();
    }
}

void iox::p3com::udp::UDPBroadcast::sendBroadcast(const void* data, size_t size) noexcept
{
    try
    {
        // std::lock_guard<std::mutex> lock(m_socketMutex);
        for (auto&& m_broadcastEndpoint : m_broadcastEndpoints)
        {
            m_discoverySocket.send_to(asio::buffer(data, size), m_broadcastEndpoint);
        }

        iox::p3com::LogDebug() << "[UDPBroadcast] Broadcast discovery info";
    }
    catch (std::exception& e)
    {
        iox::p3com::LogError() << "[UDPBroadcast] " << e.what();
        setFailed();
    }
}

uint64_t iox::p3com::udp::UDPBroadcast::discoveredEndpoints() const noexcept
{
    return m_devices.size();
}

asio::ip::udp::endpoint iox::p3com::udp::UDPBroadcast::getEndpoint(uint32_t deviceIndex) noexcept
{
    if (deviceIndex >= discoveredEndpoints())
    {
        iox::p3com::LogError() << "[UDPBroadcast] Invalid device index when sending user data";
        setFailed();
        return asio::ip::udp::endpoint();
    }
    return m_devices[deviceIndex];
}

iox::cxx::optional<uint32_t> iox::p3com::udp::UDPBroadcast::getIndex(asio::ip::address address) const noexcept
{
    // Find index of the endpoint. If iter is not registered, add iter.
    auto* iter =
        std::find_if(m_devices.begin(), m_devices.end(), [&address](auto& dev) { return dev.address() == address; });
    if (iter != m_devices.end())
    {
        const auto device = static_cast<uint32_t>(std::distance(m_devices.begin(), iter));
        return {device};
    }
    else
    {
        return iox::cxx::nullopt;
    }
}
