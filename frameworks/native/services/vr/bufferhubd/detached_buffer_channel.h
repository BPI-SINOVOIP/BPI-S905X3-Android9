#ifndef ANDROID_DVR_BUFFERHUBD_DETACHED_BUFFER_CHANNEL_H_
#define ANDROID_DVR_BUFFERHUBD_DETACHED_BUFFER_CHANNEL_H_

#include "buffer_hub.h"

#include <pdx/channel_handle.h>
#include <pdx/file_handle.h>

namespace android {
namespace dvr {

class DetachedBufferChannel : public BufferHubChannel {
 public:
  ~DetachedBufferChannel() override;

  template <typename... Args>
  static std::unique_ptr<DetachedBufferChannel> Create(Args&&... args) {
    auto buffer = std::unique_ptr<DetachedBufferChannel>(
        new DetachedBufferChannel(std::forward<Args>(args)...));
    return buffer->IsValid() ? std::move(buffer) : nullptr;
  }

  // Returns whether the object holds a valid graphic buffer.
  bool IsValid() const {
    return buffer_.IsValid() && metadata_buffer_.IsValid();
  }

  size_t user_metadata_size() const { return user_metadata_size_; }

  // Captures buffer info for use by BufferHubService::DumpState().
  BufferInfo GetBufferInfo() const override;

  bool HandleMessage(pdx::Message& message) override;
  void HandleImpulse(pdx::Message& message) override;

 private:
  // Creates a detached buffer from existing IonBuffers.
  DetachedBufferChannel(BufferHubService* service, int buffer_id,
                        int channel_id, IonBuffer buffer,
                        IonBuffer metadata_buffer, size_t user_metadata_size);

  // Allocates a new detached buffer.
  DetachedBufferChannel(BufferHubService* service, int buffer_id,
                        uint32_t width, uint32_t height, uint32_t layer_count,
                        uint32_t format, uint64_t usage,
                        size_t user_metadata_size);

  pdx::Status<BufferDescription<pdx::BorrowedHandle>> OnImport(
      pdx::Message& message);
  pdx::Status<pdx::RemoteChannelHandle> OnPromote(pdx::Message& message);

  // Gralloc buffer handles.
  IonBuffer buffer_;
  IonBuffer metadata_buffer_;

  // Size of user requested metadata.
  const size_t user_metadata_size_;
};

}  // namespace dvr
}  // namespace android

#endif  // ANDROID_DVR_BUFFERHUBD_DETACHED_BUFFER_CHANNEL_H_
