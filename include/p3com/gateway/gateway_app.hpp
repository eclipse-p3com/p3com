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

#ifndef P3COM_GATEWAY_APP_HPP
#define P3COM_GATEWAY_APP_HPP

#include "p3com/gateway/gateway_config.hpp"
#include "p3com/gateway/cmd_line.hpp"

namespace iox
{
namespace p3com
{
class GatewayApp
{
  public:
    GatewayApp(const CmdLineArgs_t& cmdLineArgs, const GatewayConfig_t& gwConfig) noexcept;

    GatewayApp(const GatewayApp&) = delete;
    GatewayApp(GatewayApp&&) = delete;
    GatewayApp& operator=(const GatewayApp&) = delete;
    GatewayApp& operator=(GatewayApp&&) = delete;

    ~GatewayApp();

    void run() noexcept;

  private:
    void enableTransports() noexcept;

    CmdLineArgs_t m_cmdLineArgs;
    GatewayConfig_t m_gwConfig;
};

} // namespace p3com
} // namespace iox

#endif
