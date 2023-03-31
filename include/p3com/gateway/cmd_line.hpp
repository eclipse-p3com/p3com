// Copyright 2023 NXP

#ifndef P3COM_CMD_CONFIG_HPP
#define P3COM_CMD_CONFIG_HPP

#include "p3com/generic/types.hpp"

namespace iox
{
namespace p3com
{
struct CmdLineArgs_t
{
    iox::log::LogLevel logLevel{log::LogLevel::kWarn};
    std::array<bool, iox::p3com::TRANSPORT_TYPE_COUNT> enabledTransports{};
    bool enabledTransportSpecified{false};
    roudi::ConfigFilePathString_t configFile{};
    bool run{true};
};

class CmdLineParser
{
  public:
    static CmdLineArgs_t parse(int argc, char** argv) noexcept;
};

} // namespace p3com
} // namespace iox

#endif
