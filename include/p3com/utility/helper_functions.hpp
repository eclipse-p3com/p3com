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

#ifndef P3COM_HELPER_FUNCTIONS_HPP
#define P3COM_HELPER_FUNCTIONS_HPP

#include "iceoryx_posh/capro/service_description.hpp"

#include "p3com/generic/types.hpp"

#include <algorithm>
#include <random>
#include <cstring>
#include <thread>

namespace iox
{
namespace p3com
{
inline bool operator==(const iox::p3com::DeviceIndex_t& left, const iox::p3com::DeviceIndex_t& right) noexcept
{
    return left.type == right.type && left.device == right.device;
}

/**
 * @brief delete an element in container
 * 
 * @tparam Container 
 * @tparam Element 
 * @param container 
 * @param element 
 */
template <typename Container, typename Element>
inline void deleteElement(Container& container, Element element) noexcept
{
    auto it = std::find(container.begin(), container.end(), element);
    if (it != container.end())
    {
        container.erase(it);
    }
}

/**
 * @brief Check element existed in vector
 * 
 * @tparam Container 
 * @tparam Element 
 * @param container 
 * @param element 
 * @return true 
 * @return false 
 */
template <typename Container, typename Element>
inline bool containsElement(const Container& container, const Element& element) noexcept
{
    auto it = std::find(container.begin(), container.end(), element);
    return it != container.end();
}

/**
 * @brief Check element existed in vector
 * 
 * @tparam  
 * @param container 
 * @param element 
 * @return true 
 * @return false 
 */
template<>
inline bool containsElement<ServiceVector_t, capro::ServiceDescription::ClassHash>(
    const ServiceVector_t& container, const capro::ServiceDescription::ClassHash& element) noexcept
{
    const auto* it = std::find_if(container.begin(), container.end(), [&element](const capro::ServiceDescription& s) {
        return s.getClassHash() == element;
    });
    return it != container.end();
}

/**
 * @brief Delete all element in a container in other container
 * 
 * @tparam Container 
 * @param a 
 * @param b 
 */
template <typename Container>
inline void deleteElements(Container& a, const Container& b) noexcept
{
    for (const auto& elem : b)
    {
        deleteElement(a, elem);
    }
}

/**
 * @brief Copy from a source container into a destination container the elements 
 * which are not already in the destination container.
 * 
 * @tparam Container 
 * @param a 
 * @param b 
 */
template <typename Container>
inline void pushUnique(Container& a, const Container& b) noexcept
{
    for (const auto& elem : b)
    {
        if(!containsElement(a, elem))
        {
            a.push_back(elem);
        }
    }
}

/**
 * @brief Generate hash
 * 
 * @return hash_t 
 */
inline hash_t generateHash() noexcept
{
    static std::random_device dev;
    static std::uniform_int_distribution<hash_t> dist;
    return dist(dev);
}

/**
 * @brief Neon supports one command to process multiple data, increase speed copy data
 * 
 * @param destination 
 * @param source 
 * @param size 
 */
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
inline void neonMemcpy(void* destination, const void* source, const uint64_t size) noexcept
{
    constexpr uint64_t ITERATION_SIZE{64U};
    if (size >= ITERATION_SIZE)
    {
        uint64_t iterations{size / ITERATION_SIZE};

        // Volatile has to be used in inline assembly.
        // coverity[autosar_cpp14_a2_11_1_violation]
        // coverity[autosar_cpp14_a7_4_1_violation]
        __asm volatile("neon_memcpy_loop%=: \n\t"
                       "LD1 {V0.16B, V1.16B, V2.16B, V3.16B}, [%[src_local]], #64  \n\t"
                       "ST1 {V0.16B, V1.16B, V2.16B, V3.16B}, [%[dst_local]], #64  \n\t"

                       "subs %[iterations],%[iterations],#1 \n\t"
                       "bne neon_memcpy_loop%= \n\t"

                       : [src_local] "+r"(source), [dst_local] "+r"(destination), [iterations] "+r"(iterations)
                       :);
    }

    const uint64_t remainingSize{size % ITERATION_SIZE};
    if (remainingSize != 0U)
    {
        std::memcpy(destination, source, remainingSize);
    }
}
#else

inline void neonMemcpy(void* destination, const void* source, const uint64_t size) noexcept
{
    std::memcpy(destination, source, size);
}
#endif

} // namespace p3com
} // namespace iox

#endif
