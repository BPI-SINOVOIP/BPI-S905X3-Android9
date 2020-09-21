#include <gtest/gtest.h>
#include <poll.h>
#include <private/dvr/buffer_hub_client.h>
#include <private/dvr/bufferhub_rpc.h>
#include <private/dvr/detached_buffer.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <ui/DetachedBufferHandle.h>

#include <mutex>
#include <thread>

#define RETRY_EINTR(fnc_call)                 \
  ([&]() -> decltype(fnc_call) {              \
    decltype(fnc_call) result;                \
    do {                                      \
      result = (fnc_call);                    \
    } while (result == -1 && errno == EINTR); \
    return result;                            \
  })()

using android::GraphicBuffer;
using android::sp;
using android::dvr::BufferConsumer;
using android::dvr::BufferProducer;
using android::dvr::DetachedBuffer;
using android::dvr::BufferHubDefs::IsBufferAcquired;
using android::dvr::BufferHubDefs::IsBufferGained;
using android::dvr::BufferHubDefs::IsBufferPosted;
using android::dvr::BufferHubDefs::IsBufferReleased;
using android::dvr::BufferHubDefs::kConsumerStateMask;
using android::dvr::BufferHubDefs::kMetadataHeaderSize;
using android::dvr::BufferHubDefs::kProducerStateBit;
using android::pdx::LocalChannelHandle;
using android::pdx::LocalHandle;
using android::pdx::Status;

const int kWidth = 640;
const int kHeight = 480;
const int kLayerCount = 1;
const int kFormat = HAL_PIXEL_FORMAT_RGBA_8888;
const int kUsage = 0;
const size_t kUserMetadataSize = 0;
const uint64_t kContext = 42;
const size_t kMaxConsumerCount = 63;
const int kPollTimeoutMs = 100;

using LibBufferHubTest = ::testing::Test;

TEST_F(LibBufferHubTest, TestBasicUsage) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);
  // Check that consumers can spawn other consumers.
  std::unique_ptr<BufferConsumer> c2 =
      BufferConsumer::Import(c->CreateConsumer());
  ASSERT_TRUE(c2.get() != nullptr);

  // Producer state mask is unique, i.e. 1.
  EXPECT_EQ(p->buffer_state_bit(), kProducerStateBit);
  // Consumer state mask cannot have producer bit on.
  EXPECT_EQ(c->buffer_state_bit() & kProducerStateBit, 0U);
  // Consumer state mask must be a single, i.e. power of 2.
  EXPECT_NE(c->buffer_state_bit(), 0U);
  EXPECT_EQ(c->buffer_state_bit() & (c->buffer_state_bit() - 1), 0U);
  // Consumer state mask cannot have producer bit on.
  EXPECT_EQ(c2->buffer_state_bit() & kProducerStateBit, 0U);
  // Consumer state mask must be a single, i.e. power of 2.
  EXPECT_NE(c2->buffer_state_bit(), 0U);
  EXPECT_EQ(c2->buffer_state_bit() & (c2->buffer_state_bit() - 1), 0U);
  // Each consumer should have unique bit.
  EXPECT_EQ(c->buffer_state_bit() & c2->buffer_state_bit(), 0U);

  // Initial state: producer not available, consumers not available.
  EXPECT_EQ(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, RETRY_EINTR(c->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, RETRY_EINTR(c2->Poll(kPollTimeoutMs)));

  EXPECT_EQ(0, p->Post(LocalHandle(), kContext));

  // New state: producer not available, consumers available.
  EXPECT_EQ(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));
  EXPECT_EQ(1, RETRY_EINTR(c->Poll(kPollTimeoutMs)));
  EXPECT_EQ(1, RETRY_EINTR(c2->Poll(kPollTimeoutMs)));

  uint64_t context;
  LocalHandle fence;
  EXPECT_EQ(0, c->Acquire(&fence, &context));
  EXPECT_EQ(kContext, context);
  EXPECT_EQ(0, RETRY_EINTR(c->Poll(kPollTimeoutMs)));
  EXPECT_EQ(1, RETRY_EINTR(c2->Poll(kPollTimeoutMs)));

  EXPECT_EQ(0, c2->Acquire(&fence, &context));
  EXPECT_EQ(kContext, context);
  EXPECT_EQ(0, RETRY_EINTR(c2->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, RETRY_EINTR(c->Poll(kPollTimeoutMs)));

  EXPECT_EQ(0, c->Release(LocalHandle()));
  EXPECT_EQ(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, c2->Discard());

  EXPECT_EQ(1, RETRY_EINTR(p->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, p->Gain(&fence));
  EXPECT_EQ(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, RETRY_EINTR(c->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, RETRY_EINTR(c2->Poll(kPollTimeoutMs)));
}

TEST_F(LibBufferHubTest, TestEpoll) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);

  LocalHandle epoll_fd{epoll_create1(EPOLL_CLOEXEC)};
  ASSERT_TRUE(epoll_fd.IsValid());

  epoll_event event;
  std::array<epoll_event, 64> events;

  auto event_sources = p->GetEventSources();
  ASSERT_LT(event_sources.size(), events.size());

  for (const auto& event_source : event_sources) {
    event = {.events = event_source.event_mask | EPOLLET,
             .data = {.fd = p->event_fd()}};
    ASSERT_EQ(0, epoll_ctl(epoll_fd.Get(), EPOLL_CTL_ADD, event_source.event_fd,
                           &event));
  }

  event_sources = c->GetEventSources();
  ASSERT_LT(event_sources.size(), events.size());

  for (const auto& event_source : event_sources) {
    event = {.events = event_source.event_mask | EPOLLET,
             .data = {.fd = c->event_fd()}};
    ASSERT_EQ(0, epoll_ctl(epoll_fd.Get(), EPOLL_CTL_ADD, event_source.event_fd,
                           &event));
  }

  // No events should be signaled initially.
  ASSERT_EQ(0, epoll_wait(epoll_fd.Get(), events.data(), events.size(), 0));

  // Post the producer and check for consumer signal.
  EXPECT_EQ(0, p->Post({}, kContext));
  ASSERT_EQ(1, epoll_wait(epoll_fd.Get(), events.data(), events.size(),
                          kPollTimeoutMs));
  ASSERT_TRUE(events[0].events & EPOLLIN);
  ASSERT_EQ(c->event_fd(), events[0].data.fd);

  // Save the event bits to translate later.
  event = events[0];

  // Check for events again. Edge-triggered mode should prevent any.
  EXPECT_EQ(0, epoll_wait(epoll_fd.Get(), events.data(), events.size(),
                          kPollTimeoutMs));
  EXPECT_EQ(0, epoll_wait(epoll_fd.Get(), events.data(), events.size(),
                          kPollTimeoutMs));
  EXPECT_EQ(0, epoll_wait(epoll_fd.Get(), events.data(), events.size(),
                          kPollTimeoutMs));
  EXPECT_EQ(0, epoll_wait(epoll_fd.Get(), events.data(), events.size(),
                          kPollTimeoutMs));

  // Translate the events.
  auto event_status = c->GetEventMask(event.events);
  ASSERT_TRUE(event_status);
  ASSERT_TRUE(event_status.get() & EPOLLIN);

  // Check for events again. Edge-triggered mode should prevent any.
  EXPECT_EQ(0, epoll_wait(epoll_fd.Get(), events.data(), events.size(),
                          kPollTimeoutMs));
}

TEST_F(LibBufferHubTest, TestStateMask) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);

  // It's ok to create up to kMaxConsumerCount consumer buffers.
  uint64_t buffer_state_bits = p->buffer_state_bit();
  std::array<std::unique_ptr<BufferConsumer>, kMaxConsumerCount> cs;
  for (size_t i = 0; i < kMaxConsumerCount; i++) {
    cs[i] = BufferConsumer::Import(p->CreateConsumer());
    ASSERT_TRUE(cs[i].get() != nullptr);
    // Expect all buffers have unique state mask.
    EXPECT_EQ(buffer_state_bits & cs[i]->buffer_state_bit(), 0U);
    buffer_state_bits |= cs[i]->buffer_state_bit();
  }
  EXPECT_EQ(buffer_state_bits, kProducerStateBit | kConsumerStateMask);

  // The 64th creation will fail with out-of-memory error.
  auto state = p->CreateConsumer();
  EXPECT_EQ(state.error(), E2BIG);

  // Release any consumer should allow us to re-create.
  for (size_t i = 0; i < kMaxConsumerCount; i++) {
    buffer_state_bits &= ~cs[i]->buffer_state_bit();
    cs[i] = nullptr;
    cs[i] = BufferConsumer::Import(p->CreateConsumer());
    ASSERT_TRUE(cs[i].get() != nullptr);
    // The released state mask will be reused.
    EXPECT_EQ(buffer_state_bits & cs[i]->buffer_state_bit(), 0U);
    buffer_state_bits |= cs[i]->buffer_state_bit();
    EXPECT_EQ(buffer_state_bits, kProducerStateBit | kConsumerStateMask);
  }
}

TEST_F(LibBufferHubTest, TestStateTransitions) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);

  uint64_t context;
  LocalHandle fence;

  // The producer buffer starts in gained state.

  // Acquire, release, and gain in gained state should fail.
  EXPECT_EQ(-EBUSY, c->Acquire(&fence, &context));
  EXPECT_EQ(-EBUSY, c->Release(LocalHandle()));
  EXPECT_EQ(-EALREADY, p->Gain(&fence));

  // Post in gained state should succeed.
  EXPECT_EQ(0, p->Post(LocalHandle(), kContext));

  // Post, release, and gain in posted state should fail.
  EXPECT_EQ(-EBUSY, p->Post(LocalHandle(), kContext));
  EXPECT_EQ(-EBUSY, c->Release(LocalHandle()));
  EXPECT_EQ(-EBUSY, p->Gain(&fence));

  // Acquire in posted state should succeed.
  EXPECT_LE(0, c->Acquire(&fence, &context));

  // Acquire, post, and gain in acquired state should fail.
  EXPECT_EQ(-EBUSY, c->Acquire(&fence, &context));
  EXPECT_EQ(-EBUSY, p->Post(LocalHandle(), kContext));
  EXPECT_EQ(-EBUSY, p->Gain(&fence));

  // Release in acquired state should succeed.
  EXPECT_EQ(0, c->Release(LocalHandle()));
  EXPECT_LT(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));

  // Release, acquire, and post in released state should fail.
  EXPECT_EQ(-EBUSY, c->Release(LocalHandle()));
  EXPECT_EQ(-EBUSY, c->Acquire(&fence, &context));
  EXPECT_EQ(-EBUSY, p->Post(LocalHandle(), kContext));

  // Gain in released state should succeed.
  EXPECT_EQ(0, p->Gain(&fence));

  // Acquire, release, and gain in gained state should fail.
  EXPECT_EQ(-EBUSY, c->Acquire(&fence, &context));
  EXPECT_EQ(-EBUSY, c->Release(LocalHandle()));
  EXPECT_EQ(-EALREADY, p->Gain(&fence));
}

TEST_F(LibBufferHubTest, TestAsyncStateTransitions) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);

  DvrNativeBufferMetadata metadata;
  LocalHandle invalid_fence;

  // The producer buffer starts in gained state.

  // Acquire, release, and gain in gained state should fail.
  EXPECT_EQ(-EBUSY, c->AcquireAsync(&metadata, &invalid_fence));
  EXPECT_FALSE(invalid_fence.IsValid());
  EXPECT_EQ(-EBUSY, c->ReleaseAsync(&metadata, invalid_fence));
  EXPECT_EQ(-EALREADY, p->GainAsync(&metadata, &invalid_fence));
  EXPECT_FALSE(invalid_fence.IsValid());

  // Post in gained state should succeed.
  EXPECT_EQ(0, p->PostAsync(&metadata, invalid_fence));
  EXPECT_EQ(p->buffer_state(), c->buffer_state());
  EXPECT_TRUE(IsBufferPosted(p->buffer_state()));

  // Post, release, and gain in posted state should fail.
  EXPECT_EQ(-EBUSY, p->PostAsync(&metadata, invalid_fence));
  EXPECT_EQ(-EBUSY, c->ReleaseAsync(&metadata, invalid_fence));
  EXPECT_EQ(-EBUSY, p->GainAsync(&metadata, &invalid_fence));
  EXPECT_FALSE(invalid_fence.IsValid());

  // Acquire in posted state should succeed.
  EXPECT_LT(0, RETRY_EINTR(c->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, c->AcquireAsync(&metadata, &invalid_fence));
  EXPECT_FALSE(invalid_fence.IsValid());
  EXPECT_EQ(p->buffer_state(), c->buffer_state());
  EXPECT_TRUE(IsBufferAcquired(p->buffer_state()));

  // Acquire, post, and gain in acquired state should fail.
  EXPECT_EQ(-EBUSY, c->AcquireAsync(&metadata, &invalid_fence));
  EXPECT_FALSE(invalid_fence.IsValid());
  EXPECT_EQ(-EBUSY, p->PostAsync(&metadata, invalid_fence));
  EXPECT_EQ(-EBUSY, p->GainAsync(&metadata, &invalid_fence));
  EXPECT_FALSE(invalid_fence.IsValid());

  // Release in acquired state should succeed.
  EXPECT_EQ(0, c->ReleaseAsync(&metadata, invalid_fence));
  EXPECT_LT(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));
  EXPECT_EQ(p->buffer_state(), c->buffer_state());
  EXPECT_TRUE(IsBufferReleased(p->buffer_state()));

  // Release, acquire, and post in released state should fail.
  EXPECT_EQ(-EBUSY, c->ReleaseAsync(&metadata, invalid_fence));
  EXPECT_EQ(-EBUSY, c->AcquireAsync(&metadata, &invalid_fence));
  EXPECT_FALSE(invalid_fence.IsValid());
  EXPECT_EQ(-EBUSY, p->PostAsync(&metadata, invalid_fence));

  // Gain in released state should succeed.
  EXPECT_EQ(0, p->GainAsync(&metadata, &invalid_fence));
  EXPECT_FALSE(invalid_fence.IsValid());
  EXPECT_EQ(p->buffer_state(), c->buffer_state());
  EXPECT_TRUE(IsBufferGained(p->buffer_state()));

  // Acquire, release, and gain in gained state should fail.
  EXPECT_EQ(-EBUSY, c->AcquireAsync(&metadata, &invalid_fence));
  EXPECT_FALSE(invalid_fence.IsValid());
  EXPECT_EQ(-EBUSY, c->ReleaseAsync(&metadata, invalid_fence));
  EXPECT_EQ(-EALREADY, p->GainAsync(&metadata, &invalid_fence));
  EXPECT_FALSE(invalid_fence.IsValid());
}

TEST_F(LibBufferHubTest, TestZeroConsumer) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);

  DvrNativeBufferMetadata metadata;
  LocalHandle invalid_fence;

  // Newly created.
  EXPECT_TRUE(IsBufferGained(p->buffer_state()));
  EXPECT_EQ(0, p->PostAsync(&metadata, invalid_fence));
  EXPECT_TRUE(IsBufferPosted(p->buffer_state()));

  // The buffer should stay in posted stay until a consumer picks it up.
  EXPECT_GE(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));

  // A new consumer should still be able to acquire the buffer immediately.
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);
  EXPECT_EQ(0, c->AcquireAsync(&metadata, &invalid_fence));
  EXPECT_TRUE(IsBufferAcquired(c->buffer_state()));
}

TEST_F(LibBufferHubTest, TestMaxConsumers) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);

  std::array<std::unique_ptr<BufferConsumer>, kMaxConsumerCount> cs;
  for (size_t i = 0; i < kMaxConsumerCount; i++) {
    cs[i] = BufferConsumer::Import(p->CreateConsumer());
    ASSERT_TRUE(cs[i].get() != nullptr);
    EXPECT_TRUE(IsBufferGained(cs[i]->buffer_state()));
  }

  DvrNativeBufferMetadata metadata;
  LocalHandle invalid_fence;

  // Post the producer should trigger all consumers to be available.
  EXPECT_EQ(0, p->PostAsync(&metadata, invalid_fence));
  EXPECT_TRUE(IsBufferPosted(p->buffer_state()));
  for (size_t i = 0; i < kMaxConsumerCount; i++) {
    EXPECT_TRUE(
        IsBufferPosted(cs[i]->buffer_state(), cs[i]->buffer_state_bit()));
    EXPECT_LT(0, RETRY_EINTR(cs[i]->Poll(kPollTimeoutMs)));
    EXPECT_EQ(0, cs[i]->AcquireAsync(&metadata, &invalid_fence));
    EXPECT_TRUE(IsBufferAcquired(p->buffer_state()));
  }

  // All consumers have to release before the buffer is considered to be
  // released.
  for (size_t i = 0; i < kMaxConsumerCount; i++) {
    EXPECT_FALSE(IsBufferReleased(p->buffer_state()));
    EXPECT_EQ(0, cs[i]->ReleaseAsync(&metadata, invalid_fence));
  }

  EXPECT_LT(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));
  EXPECT_TRUE(IsBufferReleased(p->buffer_state()));

  // Buffer state cross all clients must be consistent.
  for (size_t i = 0; i < kMaxConsumerCount; i++) {
    EXPECT_EQ(p->buffer_state(), cs[i]->buffer_state());
  }
}

TEST_F(LibBufferHubTest, TestCreateConsumerWhenBufferGained) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);
  EXPECT_TRUE(IsBufferGained(p->buffer_state()));

  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);
  EXPECT_TRUE(IsBufferGained(c->buffer_state()));

  DvrNativeBufferMetadata metadata;
  LocalHandle invalid_fence;

  // Post the gained buffer should signal already created consumer.
  EXPECT_EQ(0, p->PostAsync(&metadata, invalid_fence));
  EXPECT_TRUE(IsBufferPosted(p->buffer_state()));
  EXPECT_LT(0, RETRY_EINTR(c->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, c->AcquireAsync(&metadata, &invalid_fence));
  EXPECT_TRUE(IsBufferAcquired(c->buffer_state()));
}

TEST_F(LibBufferHubTest, TestCreateConsumerWhenBufferPosted) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);
  EXPECT_TRUE(IsBufferGained(p->buffer_state()));

  DvrNativeBufferMetadata metadata;
  LocalHandle invalid_fence;

  // Post the gained buffer before any consumer gets created.
  EXPECT_EQ(0, p->PostAsync(&metadata, invalid_fence));
  EXPECT_TRUE(IsBufferPosted(p->buffer_state()));

  // Newly created consumer should be automatically sigalled.
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);
  EXPECT_TRUE(IsBufferPosted(c->buffer_state()));
  EXPECT_EQ(0, c->AcquireAsync(&metadata, &invalid_fence));
  EXPECT_TRUE(IsBufferAcquired(c->buffer_state()));
}

TEST_F(LibBufferHubTest, TestCreateConsumerWhenBufferReleased) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);

  std::unique_ptr<BufferConsumer> c1 =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c1.get() != nullptr);

  DvrNativeBufferMetadata metadata;
  LocalHandle invalid_fence;

  // Post, acquire, and release the buffer..
  EXPECT_EQ(0, p->PostAsync(&metadata, invalid_fence));
  EXPECT_LT(0, RETRY_EINTR(c1->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, c1->AcquireAsync(&metadata, &invalid_fence));
  EXPECT_EQ(0, c1->ReleaseAsync(&metadata, invalid_fence));

  // Note that the next PDX call is on the producer channel, which may be
  // executed before Release impulse gets executed by bufferhubd. Thus, here we
  // need to wait until the releasd is confirmed before creating another
  // consumer.
  EXPECT_LT(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));
  EXPECT_TRUE(IsBufferReleased(p->buffer_state()));

  // Create another consumer immediately after the release, should not make the
  // buffer un-released.
  std::unique_ptr<BufferConsumer> c2 =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c2.get() != nullptr);

  EXPECT_TRUE(IsBufferReleased(p->buffer_state()));
  EXPECT_EQ(0, p->GainAsync(&metadata, &invalid_fence));
  EXPECT_TRUE(IsBufferGained(p->buffer_state()));
}

TEST_F(LibBufferHubTest, TestWithCustomMetadata) {
  struct Metadata {
    int64_t field1;
    int64_t field2;
  };
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(Metadata));
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);

  Metadata m = {1, 3};
  EXPECT_EQ(0, p->Post(LocalHandle(), m));
  EXPECT_LE(0, RETRY_EINTR(c->Poll(kPollTimeoutMs)));

  LocalHandle fence;
  Metadata m2 = {};
  EXPECT_EQ(0, c->Acquire(&fence, &m2));
  EXPECT_EQ(m.field1, m2.field1);
  EXPECT_EQ(m.field2, m2.field2);

  EXPECT_EQ(0, c->Release(LocalHandle()));
  EXPECT_LT(0, RETRY_EINTR(p->Poll(0)));
}

TEST_F(LibBufferHubTest, TestPostWithWrongMetaSize) {
  struct Metadata {
    int64_t field1;
    int64_t field2;
  };
  struct OverSizedMetadata {
    int64_t field1;
    int64_t field2;
    int64_t field3;
  };
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(Metadata));
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);

  // It is illegal to post metadata larger than originally requested during
  // buffer allocation.
  OverSizedMetadata evil_meta = {};
  EXPECT_NE(0, p->Post(LocalHandle(), evil_meta));
  EXPECT_GE(0, RETRY_EINTR(c->Poll(kPollTimeoutMs)));

  // It is ok to post metadata smaller than originally requested during
  // buffer allocation.
  int64_t sequence = 42;
  EXPECT_EQ(0, p->Post(LocalHandle(), sequence));
}

TEST_F(LibBufferHubTest, TestAcquireWithWrongMetaSize) {
  struct Metadata {
    int64_t field1;
    int64_t field2;
  };
  struct OverSizedMetadata {
    int64_t field1;
    int64_t field2;
    int64_t field3;
  };
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(Metadata));
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);

  Metadata m = {1, 3};
  EXPECT_EQ(0, p->Post(LocalHandle(), m));

  LocalHandle fence;
  int64_t sequence;
  OverSizedMetadata e;

  // It is illegal to acquire metadata larger than originally requested during
  // buffer allocation.
  EXPECT_NE(0, c->Acquire(&fence, &e));

  // It is ok to acquire metadata smaller than originally requested during
  // buffer allocation.
  EXPECT_EQ(0, c->Acquire(&fence, &sequence));
  EXPECT_EQ(m.field1, sequence);
}

TEST_F(LibBufferHubTest, TestAcquireWithNoMeta) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);

  int64_t sequence = 3;
  EXPECT_EQ(0, p->Post(LocalHandle(), sequence));

  LocalHandle fence;
  EXPECT_EQ(0, c->Acquire(&fence));
}

TEST_F(LibBufferHubTest, TestWithNoMeta) {
  std::unique_ptr<BufferProducer> p =
      BufferProducer::Create(kWidth, kHeight, kFormat, kUsage);
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);

  LocalHandle fence;

  EXPECT_EQ(0, p->Post<void>(LocalHandle()));
  EXPECT_EQ(0, c->Acquire(&fence));
}

TEST_F(LibBufferHubTest, TestFailureToPostMetaFromABufferWithoutMeta) {
  std::unique_ptr<BufferProducer> p =
      BufferProducer::Create(kWidth, kHeight, kFormat, kUsage);
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);

  int64_t sequence = 3;
  EXPECT_NE(0, p->Post(LocalHandle(), sequence));
}

namespace {

int PollFd(int fd, int timeout_ms) {
  pollfd p = {fd, POLLIN, 0};
  return poll(&p, 1, timeout_ms);
}

}  // namespace

TEST_F(LibBufferHubTest, TestAcquireFence) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, /*metadata_size=*/0);
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c.get() != nullptr);

  DvrNativeBufferMetadata meta;
  LocalHandle f1(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK));

  // Post with unsignaled fence.
  EXPECT_EQ(0, p->PostAsync(&meta, f1));

  // Should acquire a valid fence.
  LocalHandle f2;
  EXPECT_LT(0, RETRY_EINTR(c->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, c->AcquireAsync(&meta, &f2));
  EXPECT_TRUE(f2.IsValid());
  // The original fence and acquired fence should have different fd number.
  EXPECT_NE(f1.Get(), f2.Get());
  EXPECT_GE(0, PollFd(f2.Get(), 0));

  // Signal the original fence will trigger the new fence.
  eventfd_write(f1.Get(), 1);
  // Now the original FD has been signaled.
  EXPECT_LT(0, PollFd(f2.Get(), kPollTimeoutMs));

  // Release the consumer with an invalid fence.
  EXPECT_EQ(0, c->ReleaseAsync(&meta, LocalHandle()));

  // Should gain an invalid fence.
  LocalHandle f3;
  EXPECT_LT(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, p->GainAsync(&meta, &f3));
  EXPECT_FALSE(f3.IsValid());

  // Post with a signaled fence.
  EXPECT_EQ(0, p->PostAsync(&meta, f1));

  // Should acquire a valid fence and it's already signalled.
  LocalHandle f4;
  EXPECT_LT(0, RETRY_EINTR(c->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, c->AcquireAsync(&meta, &f4));
  EXPECT_TRUE(f4.IsValid());
  EXPECT_LT(0, PollFd(f4.Get(), kPollTimeoutMs));

  // Release with an unsignalled fence and signal it immediately after release
  // without producer gainning.
  LocalHandle f5(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK));
  EXPECT_EQ(0, c->ReleaseAsync(&meta, f5));
  eventfd_write(f5.Get(), 1);

  // Should gain a valid fence, which is already signaled.
  LocalHandle f6;
  EXPECT_LT(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));
  EXPECT_EQ(0, p->GainAsync(&meta, &f6));
  EXPECT_TRUE(f6.IsValid());
  EXPECT_LT(0, PollFd(f6.Get(), kPollTimeoutMs));
}

TEST_F(LibBufferHubTest, TestOrphanedAcquire) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p.get() != nullptr);
  std::unique_ptr<BufferConsumer> c1 =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c1.get() != nullptr);
  const uint64_t consumer_state_bit1 = c1->buffer_state_bit();

  DvrNativeBufferMetadata meta;
  EXPECT_EQ(0, p->PostAsync(&meta, LocalHandle()));

  LocalHandle fence;
  EXPECT_LT(0, RETRY_EINTR(c1->Poll(kPollTimeoutMs)));
  EXPECT_LE(0, c1->AcquireAsync(&meta, &fence));
  // Destroy the consumer now will make it orphaned and the buffer is still
  // acquired.
  c1 = nullptr;
  EXPECT_GE(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));

  std::unique_ptr<BufferConsumer> c2 =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c2.get() != nullptr);
  const uint64_t consumer_state_bit2 = c2->buffer_state_bit();
  EXPECT_NE(consumer_state_bit1, consumer_state_bit2);

  // The new consumer is available for acquire.
  EXPECT_LT(0, RETRY_EINTR(c2->Poll(kPollTimeoutMs)));
  EXPECT_LE(0, c2->AcquireAsync(&meta, &fence));
  // Releasing the consumer makes the buffer gainable.
  EXPECT_EQ(0, c2->ReleaseAsync(&meta, LocalHandle()));

  // The buffer is now available for the producer to gain.
  EXPECT_LT(0, RETRY_EINTR(p->Poll(kPollTimeoutMs)));

  // But if another consumer is created in released state.
  std::unique_ptr<BufferConsumer> c3 =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(c3.get() != nullptr);
  const uint64_t consumer_state_bit3 = c3->buffer_state_bit();
  EXPECT_NE(consumer_state_bit2, consumer_state_bit3);
  // The consumer buffer is not acquirable.
  EXPECT_GE(0, RETRY_EINTR(c3->Poll(kPollTimeoutMs)));
  EXPECT_EQ(-EBUSY, c3->AcquireAsync(&meta, &fence));

  // Producer should be able to gain no matter what.
  EXPECT_EQ(0, p->GainAsync(&meta, &fence));
}

TEST_F(LibBufferHubTest, TestDetachBufferFromProducer) {
  std::unique_ptr<BufferProducer> p = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  std::unique_ptr<BufferConsumer> c =
      BufferConsumer::Import(p->CreateConsumer());
  ASSERT_TRUE(p.get() != nullptr);
  ASSERT_TRUE(c.get() != nullptr);

  DvrNativeBufferMetadata metadata;
  LocalHandle invalid_fence;
  int p_id = p->id();

  // Detach in posted state should fail.
  EXPECT_EQ(0, p->PostAsync(&metadata, invalid_fence));
  EXPECT_GT(RETRY_EINTR(c->Poll(kPollTimeoutMs)), 0);
  auto s1 = p->Detach();
  EXPECT_FALSE(s1);

  // Detach in acquired state should fail.
  EXPECT_EQ(0, c->AcquireAsync(&metadata, &invalid_fence));
  s1 = p->Detach();
  EXPECT_FALSE(s1);

  // Detach in released state should fail.
  EXPECT_EQ(0, c->ReleaseAsync(&metadata, invalid_fence));
  EXPECT_GT(RETRY_EINTR(p->Poll(kPollTimeoutMs)), 0);
  s1 = p->Detach();
  EXPECT_FALSE(s1);

  // Detach in gained state should succeed.
  EXPECT_EQ(0, p->GainAsync(&metadata, &invalid_fence));
  s1 = p->Detach();
  EXPECT_TRUE(s1);

  LocalChannelHandle handle = s1.take();
  EXPECT_TRUE(handle.valid());

  // Both producer and consumer should have hangup.
  EXPECT_GT(RETRY_EINTR(p->Poll(kPollTimeoutMs)), 0);
  auto s2 = p->GetEventMask(POLLHUP);
  EXPECT_TRUE(s2);
  EXPECT_EQ(s2.get(), POLLHUP);

  EXPECT_GT(RETRY_EINTR(c->Poll(kPollTimeoutMs)), 0);
  s2 = p->GetEventMask(POLLHUP);
  EXPECT_TRUE(s2);
  EXPECT_EQ(s2.get(), POLLHUP);

  auto s3 = p->CreateConsumer();
  EXPECT_FALSE(s3);
  // Note that here the expected error code is EOPNOTSUPP as the socket towards
  // ProducerChannel has been teared down.
  EXPECT_EQ(s3.error(), EOPNOTSUPP);

  s3 = c->CreateConsumer();
  EXPECT_FALSE(s3);
  // Note that here the expected error code is EPIPE returned from
  // ConsumerChannel::HandleMessage as the socket is still open but the producer
  // is gone.
  EXPECT_EQ(s3.error(), EPIPE);

  // Detached buffer handle can be use to construct a new DetachedBuffer object.
  auto d = DetachedBuffer::Import(std::move(handle));
  EXPECT_FALSE(handle.valid());
  EXPECT_TRUE(d->IsConnected());
  EXPECT_TRUE(d->IsValid());

  ASSERT_TRUE(d->buffer() != nullptr);
  EXPECT_EQ(d->buffer()->initCheck(), 0);
  EXPECT_EQ(d->id(), p_id);
}

TEST_F(LibBufferHubTest, TestCreateDetachedBufferFails) {
  // Buffer Creation will fail: BLOB format requires height to be 1.
  auto b1 = DetachedBuffer::Create(kWidth, /*height=2*/ 2, kLayerCount,
                                   /*format=*/HAL_PIXEL_FORMAT_BLOB, kUsage,
                                   kUserMetadataSize);

  EXPECT_FALSE(b1->IsConnected());
  EXPECT_FALSE(b1->IsValid());
  EXPECT_TRUE(b1->buffer() == nullptr);

  // Buffer Creation will fail: user metadata size too large.
  auto b2 = DetachedBuffer::Create(
      kWidth, kHeight, kLayerCount, kFormat, kUsage,
      /*user_metadata_size=*/std::numeric_limits<size_t>::max());

  EXPECT_FALSE(b2->IsConnected());
  EXPECT_FALSE(b2->IsValid());
  EXPECT_TRUE(b2->buffer() == nullptr);

  // Buffer Creation will fail: user metadata size too large.
  auto b3 = DetachedBuffer::Create(
      kWidth, kHeight, kLayerCount, kFormat, kUsage,
      /*user_metadata_size=*/std::numeric_limits<size_t>::max() -
          kMetadataHeaderSize);

  EXPECT_FALSE(b3->IsConnected());
  EXPECT_FALSE(b3->IsValid());
  EXPECT_TRUE(b3->buffer() == nullptr);
}

TEST_F(LibBufferHubTest, TestCreateDetachedBuffer) {
  auto b1 = DetachedBuffer::Create(kWidth, kHeight, kLayerCount, kFormat,
                                   kUsage, kUserMetadataSize);
  int b1_id = b1->id();

  EXPECT_TRUE(b1->IsConnected());
  EXPECT_TRUE(b1->IsValid());
  ASSERT_TRUE(b1->buffer() != nullptr);
  EXPECT_NE(b1->id(), 0);
  EXPECT_EQ(b1->buffer()->initCheck(), 0);
  EXPECT_FALSE(b1->buffer()->isDetachedBuffer());

  // Takes a standalone GraphicBuffer which still holds on an
  // PDX::LocalChannelHandle towards BufferHub.
  sp<GraphicBuffer> g1 = b1->TakeGraphicBuffer();
  ASSERT_TRUE(g1 != nullptr);
  EXPECT_TRUE(g1->isDetachedBuffer());

  EXPECT_FALSE(b1->IsConnected());
  EXPECT_FALSE(b1->IsValid());
  EXPECT_TRUE(b1->buffer() == nullptr);

  sp<GraphicBuffer> g2 = b1->TakeGraphicBuffer();
  ASSERT_TRUE(g2 == nullptr);

  auto h1 = g1->takeDetachedBufferHandle();
  ASSERT_TRUE(h1 != nullptr);
  ASSERT_TRUE(h1->isValid());
  EXPECT_FALSE(g1->isDetachedBuffer());

  auto b2 = DetachedBuffer::Import(std::move(h1->handle()));
  ASSERT_FALSE(h1->isValid());
  EXPECT_TRUE(b2->IsConnected());
  EXPECT_TRUE(b2->IsValid());

  ASSERT_TRUE(b2->buffer() != nullptr);
  EXPECT_EQ(b2->buffer()->initCheck(), 0);

  // The newly created DetachedBuffer should share the original buffer_id.
  EXPECT_EQ(b2->id(), b1_id);
  EXPECT_FALSE(b2->buffer()->isDetachedBuffer());
}

TEST_F(LibBufferHubTest, TestPromoteDetachedBuffer) {
  auto b1 = DetachedBuffer::Create(kWidth, kHeight, kLayerCount, kFormat,
                                   kUsage, kUserMetadataSize);
  int b1_id = b1->id();
  EXPECT_TRUE(b1->IsValid());

  auto status_or_handle = b1->Promote();
  EXPECT_TRUE(status_or_handle);

  // The detached buffer should have hangup.
  EXPECT_GT(RETRY_EINTR(b1->Poll(kPollTimeoutMs)), 0);
  auto status_or_int = b1->GetEventMask(POLLHUP);
  EXPECT_TRUE(status_or_int.ok());
  EXPECT_EQ(status_or_int.get(), POLLHUP);

  // The buffer client is still considered as connected but invalid.
  EXPECT_TRUE(b1->IsConnected());
  EXPECT_FALSE(b1->IsValid());

  // Gets the channel handle for the producer.
  LocalChannelHandle h1 = status_or_handle.take();
  EXPECT_TRUE(h1.valid());

  std::unique_ptr<BufferProducer> p1 = BufferProducer::Import(std::move(h1));
  EXPECT_FALSE(h1.valid());
  ASSERT_TRUE(p1 != nullptr);
  int p1_id = p1->id();

  // A newly promoted ProducerBuffer should inherit the same buffer id.
  EXPECT_EQ(b1_id, p1_id);
  EXPECT_TRUE(IsBufferGained(p1->buffer_state()));
}

TEST_F(LibBufferHubTest, TestDetachThenPromote) {
  std::unique_ptr<BufferProducer> p1 = BufferProducer::Create(
      kWidth, kHeight, kFormat, kUsage, sizeof(uint64_t));
  ASSERT_TRUE(p1.get() != nullptr);
  int p1_id = p1->id();

  // Detached the producer.
  auto status_or_handle = p1->Detach();
  EXPECT_TRUE(status_or_handle.ok());
  LocalChannelHandle h1 = status_or_handle.take();
  EXPECT_TRUE(h1.valid());

  // Detached buffer handle can be use to construct a new DetachedBuffer object.
  auto b1 = DetachedBuffer::Import(std::move(h1));
  EXPECT_FALSE(h1.valid());
  EXPECT_TRUE(b1->IsValid());
  int b1_id = b1->id();
  EXPECT_EQ(b1_id, p1_id);

  // Promote the detached buffer.
  status_or_handle = b1->Promote();
  // The buffer client is still considered as connected but invalid.
  EXPECT_TRUE(b1->IsConnected());
  EXPECT_FALSE(b1->IsValid());
  EXPECT_TRUE(status_or_handle.ok());

  // Gets the channel handle for the producer.
  LocalChannelHandle h2 = status_or_handle.take();
  EXPECT_TRUE(h2.valid());

  std::unique_ptr<BufferProducer> p2 = BufferProducer::Import(std::move(h2));
  EXPECT_FALSE(h2.valid());
  ASSERT_TRUE(p2 != nullptr);
  int p2_id = p2->id();

  // A newly promoted ProducerBuffer should inherit the same buffer id.
  EXPECT_EQ(b1_id, p2_id);
  EXPECT_TRUE(IsBufferGained(p2->buffer_state()));
}
