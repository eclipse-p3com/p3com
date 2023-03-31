// Copyright 2023 NXP

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
