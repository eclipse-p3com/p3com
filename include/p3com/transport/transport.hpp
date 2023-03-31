// Copyright 2023 NXP

#ifndef IOX_TRANSPORT_HPP
#define IOX_TRANSPORT_HPP

#include "p3com/generic/types.hpp"
#include "p3com/transport/transport_type.hpp"

#include <atomic>
#include <cstdint>
#include <functional>

namespace iox
{
namespace p3com
{
/**
 * @brief Transport layer status
 */
enum struct TransportStatus
{
    /**
     * @brief Status "good" means that the transport layer is working as expected.
     */
    GOOD = 0,
    /**
     * @brief Status "failed" means that there was an unrecoverable failure in the transport layer but it hasnt been
     * disabled yet. This status is usually achieved when the transport layer implementation calls the `setFailed`
     * protected member function, which should be done in the case of unrecoverable failure such as an exception when
     * opening a network socket. When a transport layer is in the "failed" status, it will be automatically disabled by
     * the p3com gateway infrastructure after the transport layer method returns.
     */
    FAILED = 1,
    /**
     * @brief Status "disabled" means that the transport layer is now disabled and shouldnt be used anymore.
     */
    DISABLED = 2
};

/**
 * @note A message is "pending" if it cannot be released immediately after sending with the `sendUserData` method of a
 * transport layer, but it can only be released after the corresponding "buffer sent callback" (i.e., with the matching
 * message payload pointer) has been called. Such postponing of releasing the buffer is needed for example in the PCIe
 * transport layer, when a DMA is triggered asynchronously.
 */

/**
 * @brief To be called when a discovery message is received from the transport.
 *
 * @note First argument is the received serialized datagram header followed by the payload, second argument is the size
 * of the received message (the sum of the serialized datagram header size and the payload size) and third argument is
 * the index of the device that sent the message.
 *
 * @note Has to be implemented by every transport layer.
 */
using remoteDiscoveryCallback_t = std::function<void(const void*, size_t, DeviceIndex_t)>;

/**
 * @brief To be called when user data message is received from the transport.
 *
 * @note First argument is the received serialized datagram header followed by the payload, second argument is the size
 * of the received message (the sum of the serialized datagram header size and the payload size) and third argument is
 * the index of the device that sent the message.
 *
 * @note Has to be implemented by every transport layer.
 */
using userDataCallback_t = std::function<void(const void*, size_t, DeviceIndex_t)>;

/**
 * @brief To be called when loaning a buffer is needed before a message can be received.
 *
 * @note The first argument is the serialized datagram header, the second argument is the serialized datagram header
 * size.
 *
 * @note Should return a valid pointer to the loaned buffer or `nullptr` in the case that a buffer of this size cannot
 * be loaned for some reason.
 *
 * @note Currently only used in PCIe transport to implement DMA data transfers.
 */
using bufferNeededCallback_t = std::function<void*(const void*, size_t)>;

/**
 * @brief To be called when a buffer that was previously loaned via the bufferNeededCallback callback has now been
 * filled and can be either published or released.
 *
 * @note The first parameter is the serialized datagram header, the second argument is the serialized datagram header
 * size, the third argument is whether the buffer should be published and the fourth argument is the index of the device
 * that sent the message.
 *
 * @note Currently only used in PCIe transport to implement DMA data transfers.
 */
using bufferReleasedCallback_t = std::function<void(const void*, size_t, bool, DeviceIndex_t)>;

/**
 * @brief To be called when a pending buffer that was previously sent with the `sendUserData` method has now finished
 * sending and can be released.
 *
 * @note The first parameter is the payload of the message that can now be released.
 *
 * @note Currently only used in PCIe transport to implement DMA data transfers.
 */
using bufferSentCallback_t = std::function<void(const void*)>;

/**
 * @brief Transport layer base class
 */
class TransportLayerBase
{
  public:
    /**
     * @brief Is this transport layer alright?
     *
     * @return
     */
    bool isGood() const noexcept
    {
        return m_status.load() == TransportStatus::GOOD;
    }

    /**
     * @brief If the transport layer failed, set the status to disabled.
     *
     * @return
     */
    bool hasFailedSetDisabled() noexcept
    {
        // Atomically test whether it is FAILED, if so, set to DISABLED. Otherwise, do nothing.
        TransportStatus expected{TransportStatus::FAILED};
        return m_status.compare_exchange_strong(expected, TransportStatus::DISABLED);
    }

  protected:
    /**
     * @brief Set the status to failed.
     */
    void setFailed() noexcept
    {
        // Atomically test whether it is still GOOD, if so, set to FAILED. Otherwise, do nothing.
        TransportStatus expected{TransportStatus::GOOD};
        m_status.compare_exchange_strong(expected, TransportStatus::FAILED);
    }

  private:
    std::atomic<TransportStatus> m_status{TransportStatus::GOOD};
};

/**
 * @brief Transport layer discovery functionality
 */
class TransportLayerDiscovery : virtual public TransportLayerBase
{
  public:
    /**
     * @brief Register the remote discovery callback.
     *
     * @param remoteDiscoveryCallback_t
     */
    virtual void registerDiscoveryCallback(remoteDiscoveryCallback_t) noexcept = 0;

    /**
     * @brief Send broadcast to all connected devices with the given discovery info.
     *
     * @param serializedDiscoveryData
     * @param serializedDiscoveryDataSize
     */
    virtual void sendBroadcast(const void* serializedDiscoveryData, size_t serializedDiscoveryDataSize) noexcept = 0;
};

/**
 * @brief Transport layer user data functionality
 */
class TransportLayerUserData : virtual public TransportLayerBase
{
  public:
    /**
     * @brief Register the user data received callback.
     *
     * @param userDataCallback_t
     */
    virtual void registerUserDataCallback(userDataCallback_t) noexcept = 0;

    /**
     * @brief Register the buffer needed callback.
     *
     * @param bufferNeededCallback_t
     *
     * @note Currently only used in PCIe transport to implement DMA data transfers.
     */
    virtual void registerBufferNeededCallback(bufferNeededCallback_t) noexcept
    {
    }

    /**
     * @brief Register the buffer sent callback.
     *
     * @param bufferSentCallback_t
     *
     * @note Currently only used in PCIe transport to implement DMA data transfers.
     */
    virtual void registerBufferSentCallback(bufferSentCallback_t) noexcept
    {
    }

    /**
     * @brief Register the buffer released callback.
     *
     * @param bufferReleasedCallback_t
     *
     * @note Currently only used in PCIe transport to implement DMA data transfers.
     */
    virtual void registerBufferReleasedCallback(bufferReleasedCallback_t) noexcept
    {
    }

    /**
     * @brief Send a user data message.
     * In a normal case, the user payload data should be copied into the user payload right after the serialized
     * datagram header data. Since the seralized datagram header contains information about its size, the receiving size
     * is able to separate them. In case of pending message, the serialized datagram header needs to be sent along with
     * the buffer loaning request so that loaning with the corresponding subscriber on the destination buffer can be
     * done.
     *
     * @param serializedDatagramHeader
     * @param serializedDatagramHeaderSize
     * @param userPayload
     * @param userPayloadSize
     * @param deviceIndex
     *
     * @return True if the message is pending, so it shouldnt be released yet. False otherwise.
     *
     * @note If you dont implement the "buffer needed", "buffer sent" and "buffer released" callbacks, you should always
     * return false from this function.
     */
    virtual bool sendUserData(const void* serializedDatagramHeader,
                              size_t serializedDatagramHeaderSize,
                              const void* userPayload,
                              size_t userPayloadSize,
                              uint32_t deviceIndex) noexcept = 0;

    /**
     * @brief Will a message with this size be pending?
     *
     * @param size_t
     *
     * @return
     */
    virtual bool willBePending(size_t userPayloadSize) const noexcept
    {
        static_cast<void>(userPayloadSize);
        return false;
    }

    /**
     * @brief Maximum single message size possible to send with this transport.
     *
     * @return size
     */
    virtual size_t maxMessageSize() const noexcept = 0;
};

/**
 * @brief Transport layer
 */
class TransportLayer : public TransportLayerUserData, public TransportLayerDiscovery
{
  public:
    TransportLayer() = default;
    TransportLayer(const TransportLayer&) = default;
    TransportLayer(TransportLayer&&) = default;

    TransportLayer& operator=(const TransportLayer&) = delete;
    TransportLayer& operator=(TransportLayer&&) = delete;

    virtual ~TransportLayer() = default;

    /**
     * @brief Gateway type enumeration of this transport.
     *
     * @return type
     */
    virtual TransportType getType() const noexcept = 0;
};

} // namespace p3com
} // namespace iox

#endif // IOX_TRANSPORT_HPP
