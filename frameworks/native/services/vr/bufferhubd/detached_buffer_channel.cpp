#include "detached_buffer_channel.h"
#include "producer_channel.h"

using android::pdx::BorrowedHandle;
using android::pdx::ErrorStatus;
using android::pdx::Message;
using android::pdx::RemoteChannelHandle;
using android::pdx::Status;
using android::pdx::rpc::DispatchRemoteMethod;

namespace android {
namespace dvr {

DetachedBufferChannel::DetachedBufferChannel(BufferHubService* service,
                                             int buffer_id, int channel_id,
                                             IonBuffer buffer,
                                             IonBuffer metadata_buffer,
                                             size_t user_metadata_size)
    : BufferHubChannel(service, buffer_id, channel_id, kDetachedBufferType),
      buffer_(std::move(buffer)),
      metadata_buffer_(std::move(metadata_buffer)),
      user_metadata_size_(user_metadata_size) {
}

DetachedBufferChannel::DetachedBufferChannel(BufferHubService* service,
                                             int buffer_id, uint32_t width,
                                             uint32_t height,
                                             uint32_t layer_count,
                                             uint32_t format, uint64_t usage,
                                             size_t user_metadata_size)
    : BufferHubChannel(service, buffer_id, buffer_id, kDetachedBufferType),
      user_metadata_size_(user_metadata_size) {
  // The size the of metadata buffer is used as the "width" parameter during
  // allocation. Thus it cannot overflow uint32_t.
  if (user_metadata_size_ >= (std::numeric_limits<uint32_t>::max() -
                              BufferHubDefs::kMetadataHeaderSize)) {
    ALOGE(
        "DetachedBufferChannel::DetachedBufferChannel: metadata size too big.");
    return;
  }

  if (int ret = buffer_.Alloc(width, height, layer_count, format, usage)) {
    ALOGE(
        "DetachedBufferChannel::DetachedBufferChannel: Failed to allocate "
        "buffer: %s",
        strerror(-ret));
    return;
  }

  // Buffer metadata has two parts: 1) a fixed sized metadata header; and 2)
  // user requested metadata.
  const size_t size = BufferHubDefs::kMetadataHeaderSize + user_metadata_size_;
  if (int ret = metadata_buffer_.Alloc(size,
                                       /*height=*/1,
                                       /*layer_count=*/1,
                                       BufferHubDefs::kMetadataFormat,
                                       BufferHubDefs::kMetadataUsage)) {
    ALOGE(
        "DetachedBufferChannel::DetachedBufferChannel: Failed to allocate "
        "metadata: %s",
        strerror(-ret));
    return;
  }
}

DetachedBufferChannel::~DetachedBufferChannel() {
  ALOGD_IF(TRACE,
           "DetachedBufferChannel::~DetachedBufferChannel: channel_id=%d "
           "buffer_id=%d.",
           channel_id(), buffer_id());
  Hangup();
}

BufferHubChannel::BufferInfo DetachedBufferChannel::GetBufferInfo() const {
  return BufferInfo(buffer_id(), /*consumer_count=*/0, buffer_.width(),
                    buffer_.height(), buffer_.layer_count(), buffer_.format(),
                    buffer_.usage(), /*pending_count=*/0, /*state=*/0,
                    /*signaled_mask=*/0, /*index=*/0);
}

void DetachedBufferChannel::HandleImpulse(Message& /*message*/) {
  ATRACE_NAME("DetachedBufferChannel::HandleImpulse");
}

bool DetachedBufferChannel::HandleMessage(Message& message) {
  ATRACE_NAME("DetachedBufferChannel::HandleMessage");
  switch (message.GetOp()) {
    case DetachedBufferRPC::Import::Opcode:
      DispatchRemoteMethod<DetachedBufferRPC::Import>(
          *this, &DetachedBufferChannel::OnImport, message);
      return true;

    case DetachedBufferRPC::Promote::Opcode:
      DispatchRemoteMethod<DetachedBufferRPC::Promote>(
          *this, &DetachedBufferChannel::OnPromote, message);
      return true;

    default:
      return false;
  }
}

Status<BufferDescription<BorrowedHandle>> DetachedBufferChannel::OnImport(
    Message& /*message*/) {
  ATRACE_NAME("DetachedBufferChannel::OnGetBuffer");
  ALOGD_IF(TRACE, "DetachedBufferChannel::OnGetBuffer: buffer=%d.",
           buffer_id());

  return BufferDescription<BorrowedHandle>{buffer_,
                                           metadata_buffer_,
                                           buffer_id(),
                                           /*buffer_state_bit=*/0,
                                           BorrowedHandle{},
                                           BorrowedHandle{}};
}

Status<RemoteChannelHandle> DetachedBufferChannel::OnPromote(
    Message& message) {
  ATRACE_NAME("DetachedBufferChannel::OnPromote");
  ALOGD_IF(TRACE, "DetachedBufferChannel::OnPromote: buffer_id=%d",
           buffer_id());

  // Note that the new ProducerChannel will have different channel_id, but
  // inherits the buffer_id from the DetachedBuffer.
  int channel_id;
  auto status = message.PushChannel(0, nullptr, &channel_id);
  if (!status) {
    ALOGE(
        "DetachedBufferChannel::OnPromote: Failed to push ProducerChannel: %s.",
        status.GetErrorMessage().c_str());
    return ErrorStatus(ENOMEM);
  }

  std::unique_ptr<ProducerChannel> channel = ProducerChannel::Create(
      service(), buffer_id(), channel_id, std::move(buffer_),
      std::move(metadata_buffer_), user_metadata_size_);
  if (!channel) {
    ALOGE(
        "DetachedBufferChannel::OnPromote: Failed to create ProducerChannel "
        "from a DetachedBufferChannel, buffer_id=%d.",
        buffer_id());
  }

  const auto channel_status =
      service()->SetChannel(channel_id, std::move(channel));
  if (!channel_status) {
    // Technically, this should never fail, as we just pushed the channel. Note
    // that LOG_FATAL will be stripped out in non-debug build.
    LOG_FATAL(
        "DetachedBufferChannel::OnPromote: Failed to set new producer buffer "
        "channel: %s.",
        channel_status.GetErrorMessage().c_str());
  }

  return status;
}

}  // namespace dvr
}  // namespace android
