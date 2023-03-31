// Copyright 2023 NXP

#include "p3com/gateway/gateway_config.hpp"
#include "iceoryx_hoofs/cxx/string.hpp"

#if defined(TOML_CONFIG)
#include "p3com/internal/log/logging.hpp"
#include "iceoryx_hoofs/internal/file_reader/file_reader.hpp"

#include <cpptoml.h>
#include <limits>
#endif

iox::p3com::GatewayConfig_t
iox::p3com::TomlGatewayConfigParser::parse(roudi::ConfigFilePathString_t path) noexcept
{
    iox::p3com::GatewayConfig_t config;
    static_cast<void>(path); // Unused
#if defined(TOML_CONFIG)

    bool wasEmpty = false;
    // Set defaults if no path provided.
    if (path.size() == 0)
    {
        constexpr const char DEFAULT_CONFIG_FILE_PATH[] = "/etc/iceoryx/p3com.toml";
        path = DEFAULT_CONFIG_FILE_PATH;
        wasEmpty = true;
    }

    iox::cxx::FileReader configFile(path, "", cxx::FileReader::ErrorMode::Ignore);
    if (!configFile.isOpen())
    {
        (wasEmpty ? iox::p3com::LogInfo() : iox::p3com::LogError())
            << "[GatewayConfig] p3com gateway config file not found at: '" << path
            << "'. Falling back to default config.";
        return config;
    }

    iox::p3com::LogInfo() << "[GatewayConfig] Using gateway config at: " << path;

    std::shared_ptr<cpptoml::table> parsedToml{nullptr};
    try
    {
        // Load the file
        parsedToml = cpptoml::parse_file(path.c_str());
    }
    catch (const std::exception& parserException)
    {
        iox::p3com::LogWarn() << "[GatewayConfig] TOML parsing failed with error: " << parserException.what()
                            << ". Falling back to default config.";
        return config;
    }

    constexpr const char PREFERRED_TRANSPORT_KEY[] = "preferred-transport";
    auto preferredTransport = parsedToml->get_as<std::string>(PREFERRED_TRANSPORT_KEY);
    if (preferredTransport)
    {
        const auto it = std::find(
            iox::p3com::TRANSPORT_TYPE_NAMES.begin(), iox::p3com::TRANSPORT_TYPE_NAMES.end(), *preferredTransport);
        if (it != iox::p3com::TRANSPORT_TYPE_NAMES.end())
        {
            const auto index = std::distance(iox::p3com::TRANSPORT_TYPE_NAMES.begin(), it);
            config.preferredTransport = iox::p3com::type(static_cast<uint32_t>(index));
            iox::p3com::LogInfo() << "[GatewayConfig] Read preferred gateway transport: " << *preferredTransport;
        }
    }

    constexpr const char FORWARDED_SERVICE_KEY[] = "forwarded-service";
    auto forwardedServices = parsedToml->get_table_array(FORWARDED_SERVICE_KEY);
    if (forwardedServices)
    {
        for (const auto& service : *forwardedServices)
        {
            constexpr const char SERVICE_KEY[] = "service";
            constexpr const char INSTANCE_KEY[] = "instance";
            constexpr const char EVENT_KEY[] = "event";
            const capro::IdString_t serviceValue{cxx::TruncateToCapacity, *service->get_as<std::string>(SERVICE_KEY)};
            const capro::IdString_t instanceValue{cxx::TruncateToCapacity, *service->get_as<std::string>(INSTANCE_KEY)};
            const capro::IdString_t eventValue{cxx::TruncateToCapacity, *service->get_as<std::string>(EVENT_KEY)};
            config.forwardedServices.push_back({serviceValue, instanceValue, eventValue});
        }
    }
#endif

    return config;
}
