// Copyright 2023 NXP

#ifndef P3COM_DATA_READER_HPP
#define P3COM_DATA_READER_HPP

#include "p3com/generic/types.hpp"

namespace iox
{
namespace p3com
{
/**
 * @brief Take the next available sample
 *
 * @param datagramHeader
 * @param data
 * @param userHeaderBuffer
 * @param userPayloadBuffer
 */
void takeNext(const IoxChunkDatagramHeader_t& datagramHeader,
              const void* data,
              uint8_t* userHeaderBuffer,
              uint8_t* userPayloadBuffer) noexcept;

} // namespace p3com
} // namespace iox

#endif // P3COM_DATA_READER_HPP
