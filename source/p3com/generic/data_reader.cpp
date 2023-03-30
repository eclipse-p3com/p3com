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

#include "p3com/generic/data_reader.hpp"
#include "p3com/generic/types.hpp"
#include "p3com/utility/helper_functions.hpp"

/**
 * @brief Take next message
 *
 * @param datagramHeader
 * @param data
 * @param userHeaderBuffer
 * @param userPayloadBuffer
 */
void iox::p3com::takeNext(const iox::p3com::IoxChunkDatagramHeader_t& datagramHeader,
                        const void* data,
                        uint8_t* userHeaderBuffer,
                        uint8_t* userPayloadBuffer) noexcept
{
    if (datagramHeader.submessageOffset < datagramHeader.userHeaderSize)
    {
        if (userHeaderBuffer != nullptr)
        {
            iox::p3com::neonMemcpy(
                userHeaderBuffer + datagramHeader.submessageOffset, data, datagramHeader.submessageSize);
        }
    }
    else
    {
        if (userPayloadBuffer != nullptr)
        {
            iox::p3com::neonMemcpy((userPayloadBuffer + datagramHeader.submessageOffset) - datagramHeader.userHeaderSize,
                                 data,
                                 static_cast<size_t>(datagramHeader.submessageSize));
        }
    }
}