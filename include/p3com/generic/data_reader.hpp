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
