// Copyright 2023 NXP

#include "iceoryx_hoofs/log/logmanager.hpp"
#include "iceoryx_hoofs/posix_wrapper/signal_watcher.hpp"
#include "p3com/transport/transport_type.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

#include "p3com/generic/config.hpp"
#include "p3com/generic/pending_messages.hpp"
#include "p3com/generic/segmented_messages.hpp"
#include "p3com/generic/transport_forwarder.hpp"
#include "p3com/gateway/gateway_app.hpp"
#include "p3com/gateway/iox_to_transport.hpp"
#include "p3com/gateway/transport_to_iox.hpp"
#include "p3com/transport/transport.hpp"
#include "p3com/transport/transport_info.hpp"

iox::p3com::GatewayApp::GatewayApp(const CmdLineArgs_t& cmdLineArgs, const GatewayConfig_t& gwConfig) noexcept
    : m_cmdLineArgs(cmdLineArgs)
    , m_gwConfig(gwConfig)
{
    // Set log level
    iox::log::LogManager::GetLogManager().SetDefaultLogLevel(m_cmdLineArgs.logLevel,
                                                             iox::log::LogLevelOutput::kHideLogLevel);

    // Enable transports
    enableTransports();

    // Verify config
    if (m_gwConfig.preferredTransport != iox::p3com::TransportType::NONE
        && !iox::p3com::TransportInfo::bitset()[iox::p3com::index(m_gwConfig.preferredTransport)])
    {
        iox::p3com::LogWarn() << "[p3comGateway] Preferred transport type is not enabled!";
        m_gwConfig.preferredTransport = iox::p3com::TransportType::NONE;
    }
}

iox::p3com::GatewayApp::~GatewayApp()
{
}

void iox::p3com::GatewayApp::enableTransports() noexcept
{
    if (m_cmdLineArgs.enabledTransportSpecified)
    {
        for (uint32_t i = 0U; i < iox::p3com::TRANSPORT_TYPE_COUNT; ++i)
        {
            if (m_cmdLineArgs.enabledTransports[i])
            {
                iox::p3com::TransportInfo::enable(iox::p3com::type(i));
            }
        }
    }
    else
    {
        iox::p3com::TransportInfo::enableAll();
    }
}

void iox::p3com::GatewayApp::run() noexcept
{
    // Allocate dynamically to avoid a stack overflow
    auto discovery = std::make_unique<iox::p3com::DiscoveryManager>(m_gwConfig);
    auto pendingMessageManager = std::make_unique<iox::p3com::PendingMessageManager>();
    auto segmentedMessageManager = std::make_unique<iox::p3com::SegmentedMessageManager>();
    auto transportForwarder = std::make_unique<iox::p3com::TransportForwarder>(
        *discovery, *pendingMessageManager, m_gwConfig.forwardedServices);

    // Initialize gateways in both directions
    iox::p3com::Transport2Iceoryx tr2iox(*discovery, *transportForwarder, *segmentedMessageManager);
    iox::p3com::Iceoryx2Transport iox2tr(*discovery, *pendingMessageManager);

    // Initialize discovery system
    auto updateCallback = [&](const iox::p3com::ServiceVector_t& neededChannels) {
        tr2iox.updateChannels(neededChannels);
        iox2tr.updateChannels(neededChannels);
    };
    discovery->initialize(updateCallback);

    // Run thread that monitors periodic updates
    std::chrono::steady_clock::time_point lastLossyDiscovery{std::chrono::steady_clock::now()};
#if defined(__FREERTOS__)
    while (true)
#else
    while (!iox::posix::hasTerminationRequested())
#endif
    {
        // Check the currently saved segmented messages for timeouts
        segmentedMessageManager->checkSegmentedMessages();

        // Send the discovery information to lossy transports, with certain period
        const auto now = std::chrono::steady_clock::now();
        if (now > (lastLossyDiscovery + iox::p3com::LOSSY_TRANSPORT_DISCOVERY_PERIOD))
        {
            for (auto type : iox::p3com::LOSSY_TRANSPORT_TYPES)
            {
                discovery->resendDiscoveryInfoToTransport(type);
            }
            lastLossyDiscovery = now;
        }

        std::this_thread::sleep_for(iox::p3com::DISCOVERY_PERIOD);
    }

    // Need to deinitialize early, to make sure that local discovery events
    // dont trigger updates to iox2tr or tr2iox, which are destructed before
    // the discovery manager.
    discovery->deinitialize();

    // The only remaining places which can instigate sending over transport
    // layers are iox2tr and transportForwarder. We need to disable them now
    // so that we can destruct transport layers then.
    iox2tr.join();
    transportForwarder->join();

    // Now, no additional transport messages should be sent, so it is safe to
    // terminate transport layers.
    iox::p3com::TransportInfo::terminate();
}
