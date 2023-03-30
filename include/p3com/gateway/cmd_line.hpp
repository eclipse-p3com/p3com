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
