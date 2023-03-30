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

#include "p3com/transport/transport_info.hpp"
#include "p3com/generic/config.hpp"

std::array<std::unique_ptr<iox::p3com::TransportLayer>, iox::p3com::TRANSPORT_TYPE_COUNT> iox::p3com::TransportInfo::s_transports;
iox::p3com::bitset_t iox::p3com::TransportInfo::s_bitset;
iox::p3com::TransportInfo::transportFailCallback_t iox::p3com::TransportInfo::s_failCallback;

void iox::p3com::TransportInfo::enable(iox::p3com::TransportType type) noexcept
{
    const auto i = index(type);
    s_bitset[i] = true;

    iox::p3com::LogInfo() << "[TransportInfo] Enabling transport type: " << iox::p3com::TRANSPORT_TYPE_NAMES[i];
    switch (type)
    {
    case iox::p3com::TransportType::PCIE:
#if defined(PCIE_GATEWAY)
        s_transports[i] = std::make_unique<iox::p3com::pcie::PCIeTransport>();
#else
        iox::p3com::LogError() << "[TransportInfo] Invalid transport type: PCIe";
#endif
        break;
    case iox::p3com::TransportType::UDP:
#if defined(UDP_GATEWAY)
        s_transports[i] = std::make_unique<iox::p3com::udp::UDPTransport>();
#else
        iox::p3com::LogError() << "[TransportInfo] Invalid transport type: UDP";
#endif
        break;
    case iox::p3com::TransportType::TCP:
#if defined(TCP_GATEWAY)
        s_transports[i] = std::make_unique<iox::p3com::tcp::TCPTransport>();
#else
        iox::p3com::LogError() << "[TransportInfo] Invalid transport type: TCP";
#endif
        break;
    default:
        iox::p3com::LogError() << "[TransportInfo] Invalid transport type";
        return;
    }
}

void iox::p3com::TransportInfo::enableAll() noexcept
{
#if defined(PCIE_GATEWAY)
    enable(iox::p3com::TransportType::PCIE);
#endif
#if defined(UDP_GATEWAY) && !defined(TCP_GATEWAY)
    enable(iox::p3com::TransportType::UDP);
#endif
#if defined(TCP_GATEWAY)
    enable(iox::p3com::TransportType::TCP);
#endif
}

void iox::p3com::TransportInfo::disable(iox::p3com::TransportType type) noexcept
{
    const auto i = index(type);
    s_bitset[i] = false;

    iox::p3com::LogWarn() << "[TransportInfo] Disabled transport type: " << iox::p3com::TRANSPORT_TYPE_NAMES[i];

    // We shouldn't actually destroy the transport object for two reasons:
    // * Other threads can concurrently call its methods (and we want to avoid
    //   introducing another mutex).
    // * In case of failure, it might not even be safe to call the destruction
    //   of the transport (graceful closing of the resources might fail, too).
    // s_transports[i].reset();
}

void iox::p3com::TransportInfo::registerTransportFailCallback(
    iox::p3com::TransportInfo::transportFailCallback_t callback) noexcept
{
    s_failCallback = callback;
}
