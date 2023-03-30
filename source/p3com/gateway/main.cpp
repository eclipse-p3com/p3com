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

#include "iceoryx_posh/runtime/posh_runtime.hpp"

#include "p3com/gateway/gateway_app.hpp"
#include "p3com/gateway/gateway_config.hpp"
#include "p3com/gateway/cmd_line.hpp"

int main(int argc, char** argv)
{
    const iox::p3com::CmdLineArgs_t cmdLineArgs{iox::p3com::CmdLineParser::parse(argc, argv)};
    if (cmdLineArgs.run)
    {
        // Set log level to avoid output from starting posh runtime
        iox::log::LogManager::GetLogManager().SetDefaultLogLevel(cmdLineArgs.logLevel,
                                                                 iox::log::LogLevelOutput::kHideLogLevel);

        // Read gateway config
        const iox::p3com::GatewayConfig_t gwConfig{iox::p3com::TomlGatewayConfigParser::parse(cmdLineArgs.configFile)};

        // Start posh runtime
        iox::runtime::PoshRuntime::initRuntime(iox::p3com::IOX_GW_RUNTIME_NAME);

        // Run
        iox::p3com::GatewayApp gateway(cmdLineArgs, gwConfig);
        gateway.run();
    }

    return 0;
}
