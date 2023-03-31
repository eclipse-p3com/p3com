// Copyright 2023 NXP

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
