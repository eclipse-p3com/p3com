// Copyright 2023 NXP

#include "iceoryx_hoofs/platform/getopt.hpp"

#include "p3com/gateway/cmd_line.hpp"
#include "p3com/internal/log/logging.hpp"

namespace
{
void printHelp() noexcept
{
    std::cout << "p3com gateway application\n"
              << "usage: ./p3com-gw [options]\n"
              << "  options:\n"
              << "    -h, --help                Print help and exit\n"
              << "    -l, --log-level <LEVEL>   Set log level\n"
#if defined(PCIE_GATEWAY)
              << "    -p, --pcie                Enable PCIe transport\n"
#endif
#if defined(UDP_GATEWAY)
              << "    -u, --udp                 Enable UDP transport\n"
#endif
#if defined(TCP_GATEWAY)
              << "    -t, --tcp                 Enable TCP transport\n"
#endif
              << "    -c, --config-file <PATH>  Path to the gateway config file\n";
}

}

iox::p3com::CmdLineArgs_t iox::p3com::CmdLineParser::parse(int argc, char** argv) noexcept
{
    iox::p3com::CmdLineArgs_t config;
    constexpr option LONG_OPTIONS[] = {{"help", no_argument, nullptr, 'h'},
                                       {"pcie", no_argument, nullptr, 'p'},
                                       {"udp", no_argument, nullptr, 'u'},
                                       {"tcp", no_argument, nullptr, 't'},
                                       {"log-level", required_argument, nullptr, 'l'},
                                       {"config", required_argument, nullptr, 'c'},
                                       {nullptr, 0, nullptr, 0}};

    // colon after shortOption means it requires an argument, two colons mean optional argument
    constexpr const char* SHORT_OPTIONS = "hpuitLl:c:";
    int32_t index;
    int32_t opt{-1};

    bool wasUdp = false;
    bool wasTcp = false;
    while ((opt = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, &index), opt != -1))
    {
        switch (opt)
        {
        case 'h':
            printHelp();
            config.run = false;
            break;
        case 'p':
            config.enabledTransports[iox::p3com::index(iox::p3com::TransportType::PCIE)] = true;
            config.enabledTransportSpecified = true;
            break;
        case 'u':
            if (wasTcp)
            {
                iox::p3com::LogError() << "[TransportInfo] UDP & TCP flags can't be used together, choose only one.";
                config.run = false;
                break;
            }
            config.enabledTransports[iox::p3com::index(iox::p3com::TransportType::UDP)] = true;
            config.enabledTransportSpecified = true;
            wasUdp = true;
            break;
        case 't':
            if (wasUdp)
            {
                iox::p3com::LogError() << "[TransportInfo] UDP & TCP flags can't be used together, choose only one.";
                config.run = false;
                break;
            }
            config.enabledTransports[iox::p3com::index(iox::p3com::TransportType::TCP)] = true;
            config.enabledTransportSpecified = true;
            wasTcp = true;
            break;
        case 'l':
            if (strcmp(optarg, "off") == 0)
            {
                config.logLevel = iox::log::LogLevel::kOff;
            }
            else if (strcmp(optarg, "fatal") == 0)
            {
                config.logLevel = iox::log::LogLevel::kFatal;
            }
            else if (strcmp(optarg, "error") == 0)
            {
                config.logLevel = iox::log::LogLevel::kError;
            }
            else if (strcmp(optarg, "warning") == 0)
            {
                config.logLevel = iox::log::LogLevel::kWarn;
            }
            else if (strcmp(optarg, "info") == 0)
            {
                config.logLevel = iox::log::LogLevel::kInfo;
            }
            else if (strcmp(optarg, "debug") == 0)
            {
                config.logLevel = iox::log::LogLevel::kDebug;
            }
            else if (strcmp(optarg, "verbose") == 0)
            {
                config.logLevel = iox::log::LogLevel::kVerbose;
            }
            else
            {
                config.run = false;
                iox::p3com::LogError() << "Options for log-level are 'off', 'fatal', 'error', 'warning', 'info', 'debug' and "
                              "'verbose'!";
            }
            break;
        case 'L':
            // Legacy, keep the -L flag to mean '-l info'
            config.logLevel = iox::log::LogLevel::kInfo;
            break;
        case 'c':
            config.configFile = iox::roudi::ConfigFilePathString_t{iox::cxx::TruncateToCapacity, optarg};
            break;
        default:
            config.run = false;
            break;
        }
    }
    return config;
}

