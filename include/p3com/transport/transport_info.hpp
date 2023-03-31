// Copyright 2023 NXP

#ifndef IOX_TRANSPORT_INFO_HPP
#define IOX_TRANSPORT_INFO_HPP

#include "iceoryx_hoofs/cxx/vector.hpp"

#include "p3com/generic/types.hpp"
#include "p3com/transport/transport.hpp"
#include "p3com/transport/transport_type.hpp"
#include "p3com/internal/log/logging.hpp"

#if defined(PCIE_GATEWAY)
#include "p3com/transport/pcie/pcie_transport.hpp"
#endif
#if defined(UDP_GATEWAY)
#include "p3com/transport/udp/udp_transport.hpp"
#endif
#if defined(TCP_GATEWAY)
#include "p3com/transport/tcp/tcp_transport.hpp"
#endif

#include <array>
#include <cstdint>
#include <functional>
#include <memory>

namespace iox
{
namespace p3com
{
constexpr auto PCIE_FLAG = "--pcie";
constexpr auto PCIE_FLAG_SHORT = "-p";
constexpr auto UDP_FLAG = "--udp";
constexpr auto UDP_FLAG_SHORT = "-u";
constexpr auto TCP_FLAG = "--tcp";
constexpr auto TCP_FLAG_SHORT = "-t";

class TransportInfo
{
  public:
    using transportFailCallback_t = std::function<void()>;

    static void enable(TransportType type) noexcept;
    static void enableAll() noexcept;

    static void registerTransportFailCallback(transportFailCallback_t callback) noexcept;

    template <typename F>
    static void doFor(TransportType type, F&& fn) noexcept
    {
        auto& t = s_transports[index(type)];
        if (t && t->isGood())
        {
            fn(*t);
        }

        if (t && t->hasFailedSetDisabled())
        {
            disable(t->getType());
            if (s_failCallback)
            {
                s_failCallback();
            }
        }
    }

    template <typename F>
    static void doForAllEnabled(F&& fn) noexcept
    {
        for (auto& t : s_transports)
        {
            if (t && t->isGood())
            {
                fn(*t);
            }
        }

        for (auto& t : s_transports)
        {
            if (t && t->hasFailedSetDisabled())
            {
                disable(t->getType());
                if (s_failCallback)
                {
                    s_failCallback();
                }
            }
        }
    }

    static TransportType findMatchingType(bitset_t remoteBitset, TransportType preferredType) noexcept
    {
        const auto preferredTypeIndex = index(preferredType);
        if (preferredType != TransportType::NONE && remoteBitset[preferredTypeIndex] && s_bitset[preferredTypeIndex])
        {
            return preferredType;
        }

        for (auto& t : s_transports)
        {
            if (t && t->isGood())
            {
                const auto type = t->getType();
                if (remoteBitset[index(type)])
                {
                    return type;
                }
            }
        }

        return TransportType::NONE;
    }

    static bitset_t bitset() noexcept
    {
        return s_bitset;
    }

    static void terminate() noexcept
    {
        for (auto& t : s_transports)
        {
            t.reset();
        }
    }
     
  private:
    static void disable(TransportType type) noexcept;

    static std::array<std::unique_ptr<TransportLayer>, TRANSPORT_TYPE_COUNT> s_transports;
    static bitset_t s_bitset;
    static transportFailCallback_t s_failCallback;
};

} // namespace p3com
} // namespace iox

#endif // IOX_TRANSPORT_INFO_HPP
