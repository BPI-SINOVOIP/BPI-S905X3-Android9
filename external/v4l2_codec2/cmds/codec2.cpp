// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#define LOG_NDEBUG 0
#define LOG_TAG "codec2"

#include <C2VDAComponent.h>

#include <C2Buffer.h>
#include <C2BufferPriv.h>
#include <C2Component.h>
#include <C2PlatformSupport.h>
#include <C2Work.h>
#include <SimpleC2Interface.h>

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gui/GLConsumer.h>
#include <gui/IProducerListener.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <media/DataSource.h>
#include <media/ICrypto.h>
#include <media/IMediaHTTPService.h>
#include <media/MediaExtractor.h>
#include <media/MediaSource.h>
#include <media/stagefright/DataSourceFactory.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaExtractorFactory.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AUtils.h>

#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <thread>

using namespace android;
using namespace std::chrono_literals;

namespace {

const std::string kH264DecoderName = "c2.vda.avc.decoder";
const std::string kVP8DecoderName = "c2.vda.vp8.decoder";
const std::string kVP9DecoderName = "c2.vda.vp9.decoder";

const int kWidth = 416;
const int kHeight = 240;  // BigBuckBunny.mp4
//const int kWidth = 560;
//const int kHeight = 320;  // small.mp4
const std::string kComponentName = kH264DecoderName;

class C2VDALinearBuffer : public C2Buffer {
public:
    explicit C2VDALinearBuffer(const std::shared_ptr<C2LinearBlock>& block)
          : C2Buffer({block->share(block->offset(), block->size(), ::C2Fence())}) {}
};

class Listener;

class SimplePlayer {
public:
    SimplePlayer();
    ~SimplePlayer();

    void onWorkDone(std::weak_ptr<C2Component> component,
                    std::list<std::unique_ptr<C2Work>> workItems);
    void onTripped(std::weak_ptr<C2Component> component,
                   std::vector<std::shared_ptr<C2SettingResult>> settingResult);
    void onError(std::weak_ptr<C2Component> component, uint32_t errorCode);

    status_t play(const sp<IMediaSource>& source);

private:
    typedef std::unique_lock<std::mutex> ULock;

    enum {
        kInputBufferCount = 8,
        kDefaultInputBufferSize = 1024 * 1024,
    };

    std::shared_ptr<Listener> mListener;

    sp<IProducerListener> mProducerListener;

    // Allocators
    std::shared_ptr<C2Allocator> mLinearAlloc;
    std::shared_ptr<C2BlockPool> mLinearBlockPool;

    std::mutex mQueueLock;
    std::condition_variable mQueueCondition;
    std::list<std::unique_ptr<C2Work>> mWorkQueue;

    std::mutex mProcessedLock;
    std::condition_variable mProcessedCondition;
    std::list<std::unique_ptr<C2Work>> mProcessedWork;

    sp<Surface> mSurface;
    sp<SurfaceComposerClient> mComposerClient;
    sp<SurfaceControl> mControl;
};

class Listener : public C2Component::Listener {
public:
    explicit Listener(SimplePlayer* thiz) : mThis(thiz) {}
    virtual ~Listener() = default;

    virtual void onWorkDone_nb(std::weak_ptr<C2Component> component,
                               std::list<std::unique_ptr<C2Work>> workItems) override {
        mThis->onWorkDone(component, std::move(workItems));
    }

    virtual void onTripped_nb(
            std::weak_ptr<C2Component> component,
            std::vector<std::shared_ptr<C2SettingResult>> settingResult) override {
        mThis->onTripped(component, settingResult);
    }

    virtual void onError_nb(std::weak_ptr<C2Component> component, uint32_t errorCode) override {
        mThis->onError(component, errorCode);
    }

private:
    SimplePlayer* const mThis;
};

SimplePlayer::SimplePlayer()
      : mListener(new Listener(this)),
        mProducerListener(new DummyProducerListener),
        mComposerClient(new SurfaceComposerClient) {
    CHECK_EQ(mComposerClient->initCheck(), OK);

    std::shared_ptr<C2AllocatorStore> store = GetCodec2PlatformAllocatorStore();
    CHECK_EQ(store->fetchAllocator(C2AllocatorStore::DEFAULT_LINEAR, &mLinearAlloc), C2_OK);

    mLinearBlockPool = std::make_shared<C2BasicLinearBlockPool>(mLinearAlloc);

    mControl = mComposerClient->createSurface(String8("A Surface"), kWidth, kHeight,
                                              HAL_PIXEL_FORMAT_YV12);

    CHECK(mControl != nullptr);
    CHECK(mControl->isValid());

    SurfaceComposerClient::Transaction{}.setLayer(mControl, INT_MAX).show(mControl).apply();

    mSurface = mControl->getSurface();
    CHECK(mSurface != nullptr);
    mSurface->connect(NATIVE_WINDOW_API_CPU, mProducerListener);
}

SimplePlayer::~SimplePlayer() {
    mComposerClient->dispose();
}

void SimplePlayer::onWorkDone(std::weak_ptr<C2Component> component,
                              std::list<std::unique_ptr<C2Work>> workItems) {
    (void)component;
    ULock l(mProcessedLock);
    for (auto& item : workItems) {
        mProcessedWork.emplace_back(std::move(item));
    }
    mProcessedCondition.notify_all();
}

void SimplePlayer::onTripped(std::weak_ptr<C2Component> component,
                             std::vector<std::shared_ptr<C2SettingResult>> settingResult) {
    (void)component;
    (void)settingResult;
    // TODO
}

void SimplePlayer::onError(std::weak_ptr<C2Component> component, uint32_t errorCode) {
    (void)component;
    (void)errorCode;
    // TODO
}

status_t SimplePlayer::play(const sp<IMediaSource>& source) {
    std::deque<sp<ABuffer>> csds;
    if (kComponentName == kH264DecoderName) {
        sp<AMessage> format;
        (void)convertMetaDataToMessage(source->getFormat(), &format);

        csds.resize(2);
        format->findBuffer("csd-0", &csds[0]);
        format->findBuffer("csd-1", &csds[1]);
    }

    status_t err = source->start();

    if (err != OK) {
        ALOGE("source returned error %d (0x%08x)", err, err);
        fprintf(stderr, "source returned error %d (0x%08x)\n", err, err);
        return err;
    }

    std::shared_ptr<C2Component> component(std::make_shared<C2VDAComponent>(
            kComponentName, 0, std::make_shared<C2ReflectorHelper>()));

    component->setListener_vb(mListener, C2_DONT_BLOCK);
    std::unique_ptr<C2PortBlockPoolsTuning::output> pools =
            C2PortBlockPoolsTuning::output::AllocUnique(
                    {static_cast<uint64_t>(C2BlockPool::BASIC_GRAPHIC)});
    std::vector<std::unique_ptr<C2SettingResult>> result;
    (void)component->intf()->config_vb({pools.get()}, C2_DONT_BLOCK, &result);
    component->start();

    mProcessedWork.clear();
    for (int i = 0; i < kInputBufferCount; ++i) {
        mWorkQueue.emplace_back(new C2Work);
    }

    std::atomic_bool running(true);
    std::thread surfaceThread([this, &running]() {
        const sp<IGraphicBufferProducer>& igbp = mSurface->getIGraphicBufferProducer();
        std::vector<std::shared_ptr<C2Buffer>> pendingDisplayBuffers;
        pendingDisplayBuffers.resize(BufferQueue::NUM_BUFFER_SLOTS);
        while (running) {
            std::unique_ptr<C2Work> work;
            {
                ULock l(mProcessedLock);
                if (mProcessedWork.empty()) {
                    mProcessedCondition.wait_for(l, 100ms);
                    if (mProcessedWork.empty()) {
                        continue;
                    }
                }
                work = std::move(mProcessedWork.front());
                mProcessedWork.pop_front();
            }

            CHECK_EQ(work->worklets.size(), 1u);
            if (work->worklets.front()->output.buffers.size() == 1u) {
                int slot;
                sp<Fence> fence;
                std::shared_ptr<C2Buffer> output = work->worklets.front()->output.buffers[0];
                C2ConstGraphicBlock graphic_block = output->data().graphicBlocks().front();

                sp<GraphicBuffer> buffer(new GraphicBuffer(
                        graphic_block.handle(), GraphicBuffer::CLONE_HANDLE, graphic_block.width(),
                        graphic_block.height(), HAL_PIXEL_FORMAT_YCbCr_420_888, 1 /* layerCount */,
                        GRALLOC_USAGE_SW_READ_OFTEN, graphic_block.width()));

                CHECK_EQ(igbp->attachBuffer(&slot, buffer), OK);
                ALOGV("attachBuffer slot=%d ts=%lld", slot,
                      (work->worklets.front()->output.ordinal.timestamp * 1000ll).peekll());

                IGraphicBufferProducer::QueueBufferInput qbi(
                        (work->worklets.front()->output.ordinal.timestamp * 1000ll).peekll(), false,
                        HAL_DATASPACE_UNKNOWN, Rect(graphic_block.width(), graphic_block.height()),
                        NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW, 0, Fence::NO_FENCE, 0);
                IGraphicBufferProducer::QueueBufferOutput qbo;
                CHECK_EQ(igbp->queueBuffer(slot, qbi, &qbo), OK);

                // If the slot is reused then we can make sure the previous graphic buffer is
                // displayed (consumed), so we could returned the graphic buffer.
                pendingDisplayBuffers[slot].swap(output);
            }

            bool eos = work->worklets.front()->output.flags & C2FrameData::FLAG_END_OF_STREAM;
            // input buffer should be reset in component side.
            CHECK_EQ(work->input.buffers.size(), 1u);
            CHECK(work->input.buffers.front() == nullptr);
            work->worklets.clear();
            work->workletsProcessed = 0;

            if (eos) {
                running.store(false);  // stop the thread
            }

            ULock l(mQueueLock);
            mWorkQueue.emplace_back(std::move(work));
            mQueueCondition.notify_all();
        }
    });

    long numFrames = 0;

    for (;;) {
        size_t size = 0u;
        void* data = nullptr;
        int64_t timestamp = 0u;
        MediaBufferBase* buffer = nullptr;
        sp<ABuffer> csd;
        if (!csds.empty()) {
            csd = std::move(csds.front());
            csds.pop_front();
            size = csd->size();
            data = csd->data();
        } else {
            status_t err = source->read(&buffer);
            if (err != OK) {
                CHECK(buffer == nullptr);

                if (err == INFO_FORMAT_CHANGED) {
                    continue;
                }

                break;
            }
            MetaDataBase& meta = buffer->meta_data();
            CHECK(meta.findInt64(kKeyTime, &timestamp));

            size = buffer->size();
            data = buffer->data();
        }

        // Prepare C2Work

        std::unique_ptr<C2Work> work;
        while (!work) {
            ULock l(mQueueLock);
            if (!mWorkQueue.empty()) {
                work = std::move(mWorkQueue.front());
                mWorkQueue.pop_front();
            } else {
                mQueueCondition.wait_for(l, 100ms);
            }
        }
        work->input.flags = static_cast<C2FrameData::flags_t>(0);
        work->input.ordinal.timestamp = timestamp;
        work->input.ordinal.frameIndex = numFrames;

        // Allocate input buffer.
        std::shared_ptr<C2LinearBlock> block;
        mLinearBlockPool->fetchLinearBlock(
                size, {C2MemoryUsage::CPU_READ, C2MemoryUsage::CPU_WRITE}, &block);
        C2WriteView view = block->map().get();
        if (view.error() != C2_OK) {
            fprintf(stderr, "C2LinearBlock::map() failed : %d\n", view.error());
            break;
        }
        memcpy(view.base(), data, size);

        work->input.buffers.clear();
        work->input.buffers.emplace_back(new C2VDALinearBuffer(std::move(block)));
        work->worklets.clear();
        work->worklets.emplace_back(new C2Worklet);

        std::list<std::unique_ptr<C2Work>> items;
        items.push_back(std::move(work));

        // DO THE DECODING
        component->queue_nb(&items);

        if (buffer) {
            buffer->release();
        }
        ++numFrames;
    }
    component->drain_nb(C2Component::DRAIN_COMPONENT_WITH_EOS);

    surfaceThread.join();

    source->stop();
    component->stop();
    printf("finished...\n");
    return OK;
}

}  // namespace

static bool getMediaSourceFromFile(const char* filename, sp<IMediaSource>* source) {
    source->clear();

    sp<DataSource> dataSource =
            DataSourceFactory::CreateFromURI(nullptr /* httpService */, filename);

    if (dataSource == nullptr) {
        fprintf(stderr, "Unable to create data source.\n");
        return false;
    }

    sp<IMediaExtractor> extractor = MediaExtractorFactory::Create(dataSource);
    if (extractor == nullptr) {
        fprintf(stderr, "could not create extractor.\n");
        return false;
    }

    std::string expectedMime;
    if (kComponentName == kH264DecoderName) {
        expectedMime = "video/avc";
    } else if (kComponentName == kVP8DecoderName) {
        expectedMime = "video/x-vnd.on2.vp8";
    } else if (kComponentName == kVP9DecoderName) {
        expectedMime = "video/x-vnd.on2.vp9";
    } else {
        fprintf(stderr, "unrecognized component name: %s\n", kComponentName.c_str());
        return false;
    }

    for (size_t i = 0, numTracks = extractor->countTracks(); i < numTracks; ++i) {
        sp<MetaData> meta =
                extractor->getTrackMetaData(i, MediaExtractor::kIncludeExtensiveMetaData);
        if (meta == nullptr) {
            continue;
        }
        const char* mime;
        meta->findCString(kKeyMIMEType, &mime);
        if (!strcasecmp(mime, expectedMime.c_str())) {
            *source = extractor->getTrack(i);
            if (*source == nullptr) {
                fprintf(stderr, "It's nullptr track for track %zu.\n", i);
                return false;
            }
            return true;
        }
    }
    fprintf(stderr, "No track found.\n");
    return false;
}

static void usage(const char* me) {
    fprintf(stderr, "usage: %s [options] [input_filename]...\n", me);
    fprintf(stderr, "       -h(elp)\n");
}

int main(int argc, char** argv) {
    android::ProcessState::self()->startThreadPool();

    int res;
    while ((res = getopt(argc, argv, "h")) >= 0) {
        switch (res) {
        case 'h':
        default: {
            usage(argv[0]);
            exit(1);
            break;
        }
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 1) {
        fprintf(stderr, "No input file specified\n");
        return 1;
    }

    SimplePlayer player;

    for (int k = 0; k < argc; ++k) {
        sp<IMediaSource> mediaSource;
        if (!getMediaSourceFromFile(argv[k], &mediaSource)) {
            fprintf(stderr, "Unable to get media source from file: %s\n", argv[k]);
            return -1;
        }
        if (player.play(mediaSource) != OK) {
            fprintf(stderr, "Player failed to play media source: %s\n", argv[k]);
            return -1;
        }
    }

    return 0;
}
