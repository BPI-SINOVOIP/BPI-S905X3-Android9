// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#define LOG_NDEBUG 0
#define LOG_TAG "C2VDAComponent_test"

#include <C2VDAComponent.h>

#include <C2Buffer.h>
#include <C2BufferPriv.h>
#include <C2Component.h>
#include <C2PlatformSupport.h>
#include <C2Work.h>
#include <SimpleC2Interface.h>

#include <base/files/file.h>
#include <base/files/file_path.h>
#include <base/md5.h>
#include <base/strings/string_piece.h>
#include <base/strings/string_split.h>

#include <gtest/gtest.h>
#include <media/DataSource.h>
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
#include <utils/Log.h>

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <algorithm>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace {

const int kMD5StringLength = 32;

// Read in golden MD5s for the sanity play-through check of this video
void readGoldenMD5s(const std::string& videoFile, std::vector<std::string>* md5Strings) {
    base::FilePath filepath(videoFile + ".md5");
    std::string allMD5s;
    base::ReadFileToString(filepath, &allMD5s);
    *md5Strings = base::SplitString(allMD5s, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    // Check these are legitimate MD5s.
    for (const std::string& md5String : *md5Strings) {
        // Ignore the empty string added by SplitString. Ignore comments.
        if (!md5String.length() || md5String.at(0) == '#') {
            continue;
        }
        if (static_cast<int>(md5String.length()) != kMD5StringLength) {
            fprintf(stderr, "MD5 length error: %s\n", md5String.c_str());
        }
        if (std::count_if(md5String.begin(), md5String.end(), isxdigit) != kMD5StringLength) {
            fprintf(stderr, "MD5 includes non-hex char: %s\n", md5String.c_str());
        }
    }
    if (md5Strings->empty()) {
        fprintf(stderr, "MD5 checksum file (%s) missing or empty.\n",
                filepath.MaybeAsASCII().c_str());
    }
}

// Get file path name of recording raw YUV
base::FilePath getRecordOutputPath(const std::string& videoFile, int width, int height) {
    base::FilePath filepath(videoFile);
    filepath = filepath.RemoveExtension();
    std::string suffix = "_output_" + std::to_string(width) + "x" + std::to_string(height) + ".yuv";
    return base::FilePath(filepath.value() + suffix);
}
}  // namespace

namespace android {

// Input video data parameters. This could be overwritten by user argument [-i].
// The syntax of each column is:
//  filename:componentName:width:height:numFrames:numFragments
// - |filename| is the file path to mp4 (h264) or webm (VP8/9) video.
// - |componentName| specifies the name of decoder component.
// - |width| and |height| are for video size (in pixels).
// - |numFrames| is the number of picture frames.
// - |numFragments| is the NALU (h264) or frame (VP8/9) count by MediaExtractor.
const char* gTestVideoData = "bear.mp4:c2.vda.avc.decoder:640:360:82:84";
//const char* gTestVideoData = "bear-vp8.webm:c2.vda.vp8.decoder:640:360:82:82";
//const char* gTestVideoData = "bear-vp9.webm:c2.vda.vp9.decoder:320:240:82:82";

// Record decoded output frames as raw YUV format.
// The recorded file will be named as "<video_name>_output_<width>x<height>.yuv" under the same
// folder of input video file.
bool gRecordOutputYUV = false;

const std::string kH264DecoderName = "c2.vda.avc.decoder";
const std::string kVP8DecoderName = "c2.vda.vp8.decoder";
const std::string kVP9DecoderName = "c2.vda.vp9.decoder";

// Magic constants for indicating the timing of flush being called.
enum FlushPoint : int { END_OF_STREAM_FLUSH = -3, MID_STREAM_FLUSH = -2, NO_FLUSH = -1 };

struct TestVideoFile {
    enum class CodecType { UNKNOWN, H264, VP8, VP9 };

    std::string mFilename;
    std::string mComponentName;
    CodecType mCodec = CodecType::UNKNOWN;
    int mWidth = -1;
    int mHeight = -1;
    int mNumFrames = -1;
    int mNumFragments = -1;
    sp<IMediaSource> mData;
};

class C2VDALinearBuffer : public C2Buffer {
public:
    explicit C2VDALinearBuffer(const std::shared_ptr<C2LinearBlock>& block)
          : C2Buffer({block->share(block->offset(), block->size(), C2Fence())}) {}
};

class C2VDADummyLinearBuffer : public C2Buffer {
public:
    explicit C2VDADummyLinearBuffer(const std::shared_ptr<C2LinearBlock>& block)
          : C2Buffer({block->share(0, 0, C2Fence())}) {}
};

class Listener;

class C2VDAComponentTest : public ::testing::Test {
public:
    void onWorkDone(std::weak_ptr<C2Component> component,
                    std::list<std::unique_ptr<C2Work>> workItems);
    void onTripped(std::weak_ptr<C2Component> component,
                   std::vector<std::shared_ptr<C2SettingResult>> settingResult);
    void onError(std::weak_ptr<C2Component> component, uint32_t errorCode);

protected:
    C2VDAComponentTest();
    void SetUp() override;

    void parseTestVideoData(const char* testVideoData);

protected:
    using ULock = std::unique_lock<std::mutex>;

    enum {
        kWorkCount = 16,
    };

    std::shared_ptr<Listener> mListener;

    // Allocators
    std::shared_ptr<C2Allocator> mLinearAlloc;
    std::shared_ptr<C2BlockPool> mLinearBlockPool;

    // The array of output video frame counters which will be counted in listenerThread. The array
    // length equals to iteration time of stream play.
    std::vector<int> mOutputFrameCounts;
    // The array of work counters returned from component which will be counted in listenerThread.
    // The array length equals to iteration time of stream play.
    std::vector<int> mFinishedWorkCounts;
    // The array of output frame MD5Sum which will be computed in listenerThread. The array length
    // equals to iteration time of stream play.
    std::vector<std::string> mMD5Strings;

    // Mutex for |mWorkQueue| among main and listenerThread.
    std::mutex mQueueLock;
    std::condition_variable mQueueCondition;
    std::list<std::unique_ptr<C2Work>> mWorkQueue;

    // Mutex for |mProcessedWork| among main and listenerThread.
    std::mutex mProcessedLock;
    std::condition_variable mProcessedCondition;
    std::list<std::unique_ptr<C2Work>> mProcessedWork;

    // Mutex for |mFlushDone| among main and listenerThread.
    std::mutex mFlushDoneLock;
    std::condition_variable mFlushDoneCondition;
    bool mFlushDone;

    std::unique_ptr<TestVideoFile> mTestVideoFile;
};

class Listener : public C2Component::Listener {
public:
    explicit Listener(C2VDAComponentTest* thiz) : mThis(thiz) {}
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
    C2VDAComponentTest* const mThis;
};

C2VDAComponentTest::C2VDAComponentTest() : mListener(new Listener(this)) {
    std::shared_ptr<C2AllocatorStore> store = GetCodec2PlatformAllocatorStore();
    CHECK_EQ(store->fetchAllocator(C2AllocatorStore::DEFAULT_LINEAR, &mLinearAlloc), C2_OK);

    mLinearBlockPool = std::make_shared<C2BasicLinearBlockPool>(mLinearAlloc);
}

void C2VDAComponentTest::onWorkDone(std::weak_ptr<C2Component> component,
                                    std::list<std::unique_ptr<C2Work>> workItems) {
    (void)component;
    ULock l(mProcessedLock);
    for (auto& item : workItems) {
        mProcessedWork.emplace_back(std::move(item));
    }
    mProcessedCondition.notify_all();
}

void C2VDAComponentTest::onTripped(std::weak_ptr<C2Component> component,
                                   std::vector<std::shared_ptr<C2SettingResult>> settingResult) {
    (void)component;
    (void)settingResult;
    // no-ops
}

void C2VDAComponentTest::onError(std::weak_ptr<C2Component> component, uint32_t errorCode) {
    (void)component;
    // fail the test
    FAIL() << "Get error code from component: " << errorCode;
}

void C2VDAComponentTest::SetUp() {
    parseTestVideoData(gTestVideoData);

    mWorkQueue.clear();
    for (int i = 0; i < kWorkCount; ++i) {
        mWorkQueue.emplace_back(new C2Work);
    }
    mProcessedWork.clear();
    mFlushDone = false;
}

static bool getMediaSourceFromFile(const std::string& filename,
                                   const TestVideoFile::CodecType codec, sp<IMediaSource>* source) {
    source->clear();

    sp<DataSource> dataSource =
            DataSourceFactory::CreateFromURI(nullptr /* httpService */, filename.c_str());

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
    if (codec == TestVideoFile::CodecType::H264) {
        expectedMime = "video/avc";
    } else if (codec == TestVideoFile::CodecType::VP8) {
        expectedMime = "video/x-vnd.on2.vp8";
    } else if (codec == TestVideoFile::CodecType::VP9) {
        expectedMime = "video/x-vnd.on2.vp9";
    } else {
        fprintf(stderr, "unsupported codec type.\n");
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
                fprintf(stderr, "It's NULL track for track %zu.\n", i);
                return false;
            }
            return true;
        }
    }
    fprintf(stderr, "No track found.\n");
    return false;
}

void C2VDAComponentTest::parseTestVideoData(const char* testVideoData) {
    ALOGV("videoDataStr: %s", testVideoData);
    mTestVideoFile = std::make_unique<TestVideoFile>();

    auto splitString = [](const std::string& input, const char delim) {
        std::vector<std::string> splits;
        auto beg = input.begin();
        while (beg != input.end()) {
            auto pos = std::find(beg, input.end(), delim);
            splits.emplace_back(beg, pos);
            beg = pos != input.end() ? pos + 1 : pos;
        }
        return splits;
    };
    auto tokens = splitString(testVideoData, ':');
    ASSERT_EQ(tokens.size(), 6u);
    mTestVideoFile->mFilename = tokens[0];
    ASSERT_GT(mTestVideoFile->mFilename.length(), 0u);

    mTestVideoFile->mComponentName = tokens[1];
    if (mTestVideoFile->mComponentName == kH264DecoderName) {
        mTestVideoFile->mCodec = TestVideoFile::CodecType::H264;
    } else if (mTestVideoFile->mComponentName == kVP8DecoderName) {
        mTestVideoFile->mCodec = TestVideoFile::CodecType::VP8;
    } else if (mTestVideoFile->mComponentName == kVP9DecoderName) {
        mTestVideoFile->mCodec = TestVideoFile::CodecType::VP9;
    }
    ASSERT_NE(mTestVideoFile->mCodec, TestVideoFile::CodecType::UNKNOWN);

    mTestVideoFile->mWidth = std::stoi(tokens[2]);
    mTestVideoFile->mHeight = std::stoi(tokens[3]);
    mTestVideoFile->mNumFrames = std::stoi(tokens[4]);
    mTestVideoFile->mNumFragments = std::stoi(tokens[5]);

    ALOGV("mTestVideoFile: %s, %s, %d, %d, %d, %d", mTestVideoFile->mFilename.c_str(),
          mTestVideoFile->mComponentName.c_str(), mTestVideoFile->mWidth, mTestVideoFile->mHeight,
          mTestVideoFile->mNumFrames, mTestVideoFile->mNumFragments);
}

static void getFrameStringPieces(const C2GraphicView& constGraphicView,
                                 std::vector<::base::StringPiece>* framePieces) {
    const uint8_t* const* constData = constGraphicView.data();
    ASSERT_NE(constData, nullptr);
    const C2PlanarLayout& layout = constGraphicView.layout();
    ASSERT_EQ(layout.type, C2PlanarLayout::TYPE_YUV) << "Only support YUV plane format";

    framePieces->clear();
    framePieces->push_back(
            ::base::StringPiece(reinterpret_cast<const char*>(constData[C2PlanarLayout::PLANE_Y]),
                                constGraphicView.width() * constGraphicView.height()));
    if (layout.planes[C2PlanarLayout::PLANE_U].colInc == 2) {  // semi-planar mode
        framePieces->push_back(::base::StringPiece(
                reinterpret_cast<const char*>(std::min(constData[C2PlanarLayout::PLANE_U],
                                                       constData[C2PlanarLayout::PLANE_V])),
                constGraphicView.width() * constGraphicView.height() / 2));
    } else {
        framePieces->push_back(::base::StringPiece(
                reinterpret_cast<const char*>(constData[C2PlanarLayout::PLANE_U]),
                constGraphicView.width() * constGraphicView.height() / 4));
        framePieces->push_back(::base::StringPiece(
                reinterpret_cast<const char*>(constData[C2PlanarLayout::PLANE_V]),
                constGraphicView.width() * constGraphicView.height() / 4));
    }
}

// Test parameters:
// - Flush after work index. If this value is not negative, test will signal flush to component
//   after queueing the work frame index equals to this value in the first iteration. Negative
//   values may be magic constants, please refer to FlushPoint enum.
// - Number of play through. This value specifies the iteration time for playing entire video. If
//   |mFlushAfterWorkIndex| is not negative, the first iteration will perform flush, then repeat
//   times as this value for playing entire video.
// - Sanity check. If this is true, decoded content sanity check is enabled. Test will compute the
//   MD5Sum for output frame data for a play-though iteration (not flushed), and compare to golden
//   MD5Sums which should be stored in the file |video_filename|.md5
// - Use dummy EOS work. If this is true, test will queue a dummy work with end-of-stream flag in
//   the end of all input works. On the contrary, test will call drain_nb() to component.
class C2VDAComponentParamTest
      : public C2VDAComponentTest,
        public ::testing::WithParamInterface<std::tuple<int, uint32_t, bool, bool>> {
protected:
    int mFlushAfterWorkIndex;
    uint32_t mNumberOfPlaythrough;
    bool mSanityCheck;
    bool mUseDummyEOSWork;
};

TEST_P(C2VDAComponentParamTest, SimpleDecodeTest) {
    mFlushAfterWorkIndex = std::get<0>(GetParam());
    if (mFlushAfterWorkIndex == FlushPoint::MID_STREAM_FLUSH) {
        mFlushAfterWorkIndex = mTestVideoFile->mNumFragments / 2;
    } else if (mFlushAfterWorkIndex == FlushPoint::END_OF_STREAM_FLUSH) {
        mFlushAfterWorkIndex = mTestVideoFile->mNumFragments - 1;
    }
    ASSERT_LT(mFlushAfterWorkIndex, mTestVideoFile->mNumFragments);
    mNumberOfPlaythrough = std::get<1>(GetParam());

    if (mFlushAfterWorkIndex >= 0) {
        mNumberOfPlaythrough++;  // add the first iteration for perform mid-stream flushing.
    }

    mSanityCheck = std::get<2>(GetParam());
    mUseDummyEOSWork = std::get<3>(GetParam());

    // Reset counters and determine the expected answers for all iterations.
    mOutputFrameCounts.resize(mNumberOfPlaythrough, 0);
    mFinishedWorkCounts.resize(mNumberOfPlaythrough, 0);
    mMD5Strings.resize(mNumberOfPlaythrough);
    std::vector<int> expectedOutputFrameCounts(mNumberOfPlaythrough, mTestVideoFile->mNumFrames);
    auto expectedWorkCount = mTestVideoFile->mNumFragments;
    if (mUseDummyEOSWork) {
        expectedWorkCount += 1;  // plus one dummy EOS work
    }
    std::vector<int> expectedFinishedWorkCounts(mNumberOfPlaythrough, expectedWorkCount);
    if (mFlushAfterWorkIndex >= 0) {
        // First iteration performs the mid-stream flushing.
        expectedOutputFrameCounts[0] = mFlushAfterWorkIndex + 1;
        expectedFinishedWorkCounts[0] = mFlushAfterWorkIndex + 1;
    }

    std::shared_ptr<C2Component> component(std::make_shared<C2VDAComponent>(
            mTestVideoFile->mComponentName, 0, std::make_shared<C2ReflectorHelper>()));

    ASSERT_EQ(component->setListener_vb(mListener, C2_DONT_BLOCK), C2_OK);
    ASSERT_EQ(component->start(), C2_OK);

    std::atomic_bool running(true);
    std::thread listenerThread([this, &running]() {
        uint32_t iteration = 0;
        ::base::MD5Context md5Ctx;
        ::base::MD5Init(&md5Ctx);
        ::base::File recordFile;
        if (gRecordOutputYUV) {
            auto recordFilePath = getRecordOutputPath(
                    mTestVideoFile->mFilename, mTestVideoFile->mWidth, mTestVideoFile->mHeight);
            fprintf(stdout, "record output file: %s\n", recordFilePath.value().c_str());
            recordFile = ::base::File(recordFilePath,
                                      ::base::File::FLAG_OPEN_ALWAYS | ::base::File::FLAG_WRITE);
            ASSERT_TRUE(recordFile.IsValid());
        }
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
            mFinishedWorkCounts[iteration]++;
            ALOGV("Output: frame index: %llu result: %d flags: 0x%x buffers: %zu",
                  work->input.ordinal.frameIndex.peekull(), work->result,
                  work->worklets.front()->output.flags,
                  work->worklets.front()->output.buffers.size());

            ASSERT_EQ(work->worklets.size(), 1u);
            if (work->worklets.front()->output.buffers.size() == 1u) {
                std::shared_ptr<C2Buffer> output = work->worklets.front()->output.buffers[0];
                C2ConstGraphicBlock graphicBlock = output->data().graphicBlocks().front();

                // check graphic buffer size (coded size) is not less than given video size.
                ASSERT_LE(mTestVideoFile->mWidth, static_cast<int>(graphicBlock.width()));
                ASSERT_LE(mTestVideoFile->mHeight, static_cast<int>(graphicBlock.height()));

                // check visible rect equals to given video size.
                ASSERT_EQ(mTestVideoFile->mWidth, static_cast<int>(graphicBlock.crop().width));
                ASSERT_EQ(mTestVideoFile->mHeight, static_cast<int>(graphicBlock.crop().height));
                ASSERT_EQ(0u, graphicBlock.crop().left);
                ASSERT_EQ(0u, graphicBlock.crop().top);

                // Intended behavior for Intel libva driver (crbug.com/148546):
                // The 5ms latency is laid here to make sure surface content is finished processed
                // processed by libva.
                std::this_thread::sleep_for(std::chrono::milliseconds(5));

                const C2GraphicView& constGraphicView = graphicBlock.map().get();
                ASSERT_EQ(C2_OK, constGraphicView.error());
                std::vector<::base::StringPiece> framePieces;
                getFrameStringPieces(constGraphicView, &framePieces);
                ASSERT_FALSE(framePieces.empty());
                if (mSanityCheck) {
                    for (const auto& piece : framePieces) {
                        ::base::MD5Update(&md5Ctx, piece);
                    }
                }
                if (gRecordOutputYUV) {
                    for (const auto& piece : framePieces) {
                        ASSERT_EQ(static_cast<int>(piece.length()),
                                  recordFile.WriteAtCurrentPos(piece.data(), piece.length()))
                                << "Failed to write file for yuv recording...";
                    }
                }

                work->worklets.front()->output.buffers.clear();
                mOutputFrameCounts[iteration]++;
            }

            bool iteration_end =
                    work->worklets.front()->output.flags & C2FrameData::FLAG_END_OF_STREAM;

            // input buffer should be reset in component side.
            ASSERT_EQ(work->input.buffers.size(), 1u);
            ASSERT_TRUE(work->input.buffers.front() == nullptr);
            work->worklets.clear();
            work->workletsProcessed = 0;

            if (iteration == 0 && work->input.ordinal.frameIndex.peeku() ==
                                          static_cast<uint64_t>(mFlushAfterWorkIndex)) {
                ULock l(mFlushDoneLock);
                mFlushDone = true;
                mFlushDoneCondition.notify_all();
                iteration_end = true;
            }

            ULock l(mQueueLock);
            mWorkQueue.emplace_back(std::move(work));
            mQueueCondition.notify_all();

            if (iteration_end) {
                // record md5sum
                ::base::MD5Digest digest;
                ::base::MD5Final(&digest, &md5Ctx);
                mMD5Strings[iteration] = ::base::MD5DigestToBase16(digest);
                ::base::MD5Init(&md5Ctx);

                iteration++;
                if (iteration == mNumberOfPlaythrough) {
                    running.store(false);  // stop the thread
                }
            }
        }
    });

    for (uint32_t iteration = 0; iteration < mNumberOfPlaythrough; ++iteration) {
        ASSERT_TRUE(getMediaSourceFromFile(mTestVideoFile->mFilename, mTestVideoFile->mCodec,
                                           &mTestVideoFile->mData));

        std::deque<sp<ABuffer>> csds;
        if (mTestVideoFile->mCodec == TestVideoFile::CodecType::H264) {
            // Get csd buffers for h264.
            sp<AMessage> format;
            (void)convertMetaDataToMessage(mTestVideoFile->mData->getFormat(), &format);
            csds.resize(2);
            format->findBuffer("csd-0", &csds[0]);
            format->findBuffer("csd-1", &csds[1]);
            ASSERT_TRUE(csds[0] != nullptr && csds[1] != nullptr);
        }

        ASSERT_EQ(mTestVideoFile->mData->start(), OK);

        int numWorks = 0;
        while (true) {
            size_t size = 0u;
            void* data = nullptr;
            int64_t timestamp = 0u;
            MediaBufferBase* buffer = nullptr;
            sp<ABuffer> csd;
            bool queueDummyEOSWork = false;
            if (!csds.empty()) {
                csd = std::move(csds.front());
                csds.pop_front();
                size = csd->size();
                data = csd->data();
            } else {
                if (mTestVideoFile->mData->read(&buffer) != OK) {
                    ASSERT_TRUE(buffer == nullptr);
                    if (mUseDummyEOSWork) {
                        ALOGV("Meet end of stream. Put a dummy EOS work.");
                        queueDummyEOSWork = true;
                    } else {
                        ALOGV("Meet end of stream. Now drain the component.");
                        ASSERT_EQ(component->drain_nb(C2Component::DRAIN_COMPONENT_WITH_EOS),
                                  C2_OK);
                        break;
                    }
                    // TODO(johnylin): add test with drain with DRAIN_COMPONENT_NO_EOS when we know
                    //                 the actual use case of it.
                } else {
                    MetaDataBase& meta = buffer->meta_data();
                    ASSERT_TRUE(meta.findInt64(kKeyTime, &timestamp));
                    size = buffer->size();
                    data = buffer->data();
                }
            }

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

            work->input.ordinal.frameIndex = static_cast<uint64_t>(numWorks);
            work->input.buffers.clear();

            std::shared_ptr<C2LinearBlock> block;
            if (queueDummyEOSWork) {
                work->input.flags = C2FrameData::FLAG_END_OF_STREAM;
                work->input.ordinal.timestamp = 0;  // timestamp is invalid for dummy EOS work

                // Create a dummy input buffer by allocating minimal size of buffer from block pool.
                mLinearBlockPool->fetchLinearBlock(
                        1, {C2MemoryUsage::CPU_READ, C2MemoryUsage::CPU_WRITE}, &block);
                work->input.buffers.emplace_back(new C2VDADummyLinearBuffer(std::move(block)));
                ALOGV("Input: (Dummy EOS) id: %llu", work->input.ordinal.frameIndex.peekull());
            } else {
                work->input.flags = static_cast<C2FrameData::flags_t>(0);
                work->input.ordinal.timestamp = static_cast<uint64_t>(timestamp);

                // Allocate an input buffer with data size.
                mLinearBlockPool->fetchLinearBlock(
                        size, {C2MemoryUsage::CPU_READ, C2MemoryUsage::CPU_WRITE}, &block);
                C2WriteView view = block->map().get();
                ASSERT_EQ(view.error(), C2_OK);
                memcpy(view.base(), data, size);
                work->input.buffers.emplace_back(new C2VDALinearBuffer(std::move(block)));
                ALOGV("Input: bitstream id: %llu timestamp: %llu size: %zu",
                      work->input.ordinal.frameIndex.peekull(),
                      work->input.ordinal.timestamp.peekull(), size);
            }

            work->worklets.clear();
            work->worklets.emplace_back(new C2Worklet);

            std::list<std::unique_ptr<C2Work>> items;
            items.push_back(std::move(work));

            // Queue the work.
            ASSERT_EQ(component->queue_nb(&items), C2_OK);
            numWorks++;

            if (buffer) {
                buffer->release();
            }

            if (iteration == 0 && numWorks == mFlushAfterWorkIndex + 1) {
                // Perform flush.
                // Note: C2VDAComponent does not return work via |flushedWork|.
                ASSERT_EQ(component->flush_sm(C2Component::FLUSH_COMPONENT,
                                              nullptr /* flushedWork */),
                          C2_OK);
                break;
            }

            if (queueDummyEOSWork) {
                break;
            }
        }

        if (iteration == 0 && mFlushAfterWorkIndex >= 0) {
            // Wait here until client get all flushed works.
            while (true) {
                ULock l(mFlushDoneLock);
                if (mFlushDone) {
                    break;
                }
                mFlushDoneCondition.wait_for(l, 100ms);
            }
            ALOGV("Got flush done signal");
            EXPECT_EQ(numWorks, mFlushAfterWorkIndex + 1);
        } else {
            EXPECT_EQ(numWorks, expectedWorkCount);
        }
        ASSERT_EQ(mTestVideoFile->mData->stop(), OK);
    }

    listenerThread.join();
    ASSERT_EQ(running, false);
    ASSERT_EQ(component->stop(), C2_OK);

    // Finally check the decoding want as expected.
    for (uint32_t i = 0; i < mNumberOfPlaythrough; ++i) {
        if (mFlushAfterWorkIndex >= 0 && i == 0) {
            EXPECT_LE(mOutputFrameCounts[i], expectedOutputFrameCounts[i]) << "At iteration: " << i;
        } else {
            EXPECT_EQ(mOutputFrameCounts[i], expectedOutputFrameCounts[i]) << "At iteration: " << i;
        }
        EXPECT_EQ(mFinishedWorkCounts[i], expectedFinishedWorkCounts[i]) << "At iteration: " << i;
    }

    if (mSanityCheck) {
        std::vector<std::string> goldenMD5s;
        readGoldenMD5s(mTestVideoFile->mFilename, &goldenMD5s);
        for (uint32_t i = 0; i < mNumberOfPlaythrough; ++i) {
            if (mFlushAfterWorkIndex >= 0 && i == 0) {
                continue;  // do not compare the iteration with flushing
            }
            bool matched = std::find(goldenMD5s.begin(), goldenMD5s.end(), mMD5Strings[i]) !=
                           goldenMD5s.end();
            EXPECT_TRUE(matched) << "Unknown MD5: " << mMD5Strings[i] << " at iter: " << i;
        }
    }
}

// Play input video once, end by draining.
INSTANTIATE_TEST_CASE_P(SinglePlaythroughTest, C2VDAComponentParamTest,
                        ::testing::Values(std::make_tuple(static_cast<int>(FlushPoint::NO_FLUSH),
                                                          1u, false, false)));
// Play input video once, end by dummy EOS work.
INSTANTIATE_TEST_CASE_P(DummyEOSWorkTest, C2VDAComponentParamTest,
                        ::testing::Values(std::make_tuple(static_cast<int>(FlushPoint::NO_FLUSH),
                                                          1u, false, true)));

// Play 5 times of input video, and check sanity by MD5Sum.
INSTANTIATE_TEST_CASE_P(MultiplePlaythroughSanityTest, C2VDAComponentParamTest,
                        ::testing::Values(std::make_tuple(static_cast<int>(FlushPoint::NO_FLUSH),
                                                          5u, true, false)));

// Test mid-stream flush then play once entirely.
INSTANTIATE_TEST_CASE_P(FlushPlaythroughTest, C2VDAComponentParamTest,
                        ::testing::Values(std::make_tuple(40, 1u, true, false)));

// Test mid-stream flush then stop.
INSTANTIATE_TEST_CASE_P(FlushStopTest, C2VDAComponentParamTest,
                        ::testing::Values(std::make_tuple(
                                static_cast<int>(FlushPoint::MID_STREAM_FLUSH), 0u, false, false)));

// Test early flush (after a few works) then stop.
INSTANTIATE_TEST_CASE_P(EarlyFlushStopTest, C2VDAComponentParamTest,
                        ::testing::Values(std::make_tuple(0, 0u, false, false),
                                          std::make_tuple(1, 0u, false, false),
                                          std::make_tuple(2, 0u, false, false),
                                          std::make_tuple(3, 0u, false, false)));

// Test end-of-stream flush then stop.
INSTANTIATE_TEST_CASE_P(
        EndOfStreamFlushStopTest, C2VDAComponentParamTest,
        ::testing::Values(std::make_tuple(static_cast<int>(FlushPoint::END_OF_STREAM_FLUSH), 0u,
                                          false, false)));

}  // namespace android

static void usage(const char* me) {
    fprintf(stderr, "usage: %s [-i test_video_data] [-r(ecord YUV)] [gtest options]\n", me);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    int res;
    while ((res = getopt(argc, argv, "i:r")) >= 0) {
        switch (res) {
        case 'i': {
            android::gTestVideoData = optarg;
            break;
        }
        case 'r': {
            android::gRecordOutputYUV = true;
            break;
        }
        default: {
            usage(argv[0]);
            exit(1);
            break;
        }
        }
    }

    return RUN_ALL_TESTS();
}
