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

#ifndef IOX_UDP_TRANSPORT_BROADCAST_HPP
#define IOX_UDP_TRANSPORT_BROADCAST_HPP

#include "p3com/generic/config.hpp"
#include "p3com/transport/transport.hpp"

#include <asio.hpp>

#include <cstdint>

namespace iox
{
namespace p3com
{
namespace udp
{
class UDPBroadcast : public TransportLayerDiscovery
{
  public:
    explicit UDPBroadcast(asio::io_service& context) noexcept;

    UDPBroadcast(const UDPBroadcast&) = delete;
    UDPBroadcast& operator=(const UDPBroadcast&) = delete;
    UDPBroadcast(UDPBroadcast&&) = delete;
    UDPBroadcast& operator=(UDPBroadcast&&) = delete;
    ~UDPBroadcast() = default;

    void registerDiscoveryCallback(remoteDiscoveryCallback_t callback) noexcept override;
    void sendBroadcast(const void* data, size_t size) noexcept override;

    uint64_t discoveredEndpoints() const noexcept;
    asio::ip::udp::endpoint getEndpoint(uint32_t deviceIndex) noexcept;
    cxx::optional<uint32_t> getIndex(asio::ip::address address) const noexcept;

  private:
    static constexpr uint16_t DISCOVERY_PORT = 9332U;
    static constexpr uint32_t MAX_DATAGRAM_SIZE = 32768U; // 32 kB

    static uint32_t sockAddrToUint32(struct sockaddr* address) noexcept;
    static std::string inetNtoA(uint32_t addr) noexcept;

    void discoverySocketCallback(asio::error_code ec, size_t bytes) noexcept;
    void discoveryAsyncReceive() noexcept;
    void discoverBroadcastAddresses() noexcept;

    asio::ip::udp::socket m_discoverySocket;
    cxx::vector<asio::ip::udp::endpoint, MAX_NETWORK_IFACE_COUNT> m_interfaceEndpoints;
    cxx::vector<asio::ip::udp::endpoint, MAX_NETWORK_IFACE_COUNT> m_broadcastEndpoints;
    std::array<uint8_t, MAX_DATAGRAM_SIZE> m_outputBuffer;
    asio::ip::udp::endpoint m_outputEndpoint;
    remoteDiscoveryCallback_t m_remoteDiscoveryCallback;
    cxx::vector<asio::ip::udp::endpoint, MAX_DEVICE_COUNT> m_devices;
};

} // namespace udp
} // namespace p3com
} // namespace iox

#endif // IOX_UDP_TRANSPORT_BROADCAST_HPP
