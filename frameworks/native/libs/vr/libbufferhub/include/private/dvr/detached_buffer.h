#ifndef ANDROID_DVR_DETACHED_BUFFER_H_
#define ANDROID_DVR_DETACHED_BUFFER_H_

#include <private/dvr/buffer_hub_client.h>

namespace android {
namespace dvr {

class DetachedBuffer {
 public:
  // Allocates a standalone DetachedBuffer not associated with any producer
  // consumer set.
  static std::unique_ptr<DetachedBuffer> Create(uint32_t width, uint32_t height,
                                                uint32_t layer_count,
                                                uint32_t format, uint64_t usage,
                                                size_t user_metadata_size) {
    return std::unique_ptr<DetachedBuffer>(new DetachedBuffer(
        width, height, layer_count, format, usage, user_metadata_size));
  }

  // Imports the given channel handle to a DetachedBuffer, taking ownership.
  static std::unique_ptr<DetachedBuffer> Import(
      pdx::LocalChannelHandle channel_handle) {
    return std::unique_ptr<DetachedBuffer>(
        new DetachedBuffer(std::move(channel_handle)));
  }

  DetachedBuffer(const DetachedBuffer&) = delete;
  void operator=(const DetachedBuffer&) = delete;

  const sp<GraphicBuffer>& buffer() const { return buffer_.buffer(); }

  int id() const { return id_; }

  // Returns true if the buffer holds an open PDX channels towards bufferhubd.
  bool IsConnected() const { return client_.IsValid(); }

  // Returns true if the buffer holds an valid gralloc buffer handle that's
  // availble for the client to read from and/or write into.
  bool IsValid() const { return buffer_.IsValid(); }

  // Returns the event mask for all the events that are pending on this buffer
  // (see sys/poll.h for all possible bits).
  pdx::Status<int> GetEventMask(int events) {
    if (auto* channel = client_.GetChannel()) {
      return channel->GetEventMask(events);
    } else {
      return pdx::ErrorStatus(EINVAL);
    }
  }

  // Polls the fd for |timeout_ms| milliseconds (-1 for infinity).
  int Poll(int timeout_ms);

  // Promotes a DetachedBuffer to become a ProducerBuffer. Once promoted the
  // DetachedBuffer channel will be closed automatically on successful IPC
  // return. Further IPCs towards this channel will return error.
  pdx::Status<pdx::LocalChannelHandle> Promote();

  // Takes the underlying graphic buffer out of this DetachedBuffer. This call
  // immediately invalidates this DetachedBuffer object and transfers the
  // underlying pdx::LocalChannelHandle into the GraphicBuffer.
  sp<GraphicBuffer> TakeGraphicBuffer();

 private:
  DetachedBuffer(uint32_t width, uint32_t height, uint32_t layer_count,
                 uint32_t format, uint64_t usage, size_t user_metadata_size);

  DetachedBuffer(pdx::LocalChannelHandle channel_handle);

  int ImportGraphicBuffer();

  // Global id for the buffer that is consistent across processes.
  int id_;
  IonBuffer buffer_;
  BufferHubClient client_;
};

}  // namespace dvr
}  // namespace android

#endif  // ANDROID_DVR_DETACHED_BUFFER_H_
