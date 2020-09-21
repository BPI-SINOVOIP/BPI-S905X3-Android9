/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "SoundTriggerHidlHalTest"
#include <stdlib.h>
#include <time.h>

#include <condition_variable>
#include <mutex>

#include <android/log.h>
#include <cutils/native_handle.h>
#include <log/log.h>

#include <android/hardware/audio/common/2.0/types.h>
#include <android/hardware/soundtrigger/2.0/ISoundTriggerHw.h>
#include <android/hardware/soundtrigger/2.0/types.h>
#include <android/hardware/soundtrigger/2.1/ISoundTriggerHw.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <hidlmemory/mapping.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

#define SHORT_TIMEOUT_PERIOD (1)

using ::android::sp;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::audio::common::V2_0::AudioDevice;
using ::android::hardware::soundtrigger::V2_0::PhraseRecognitionExtra;
using ::android::hardware::soundtrigger::V2_0::RecognitionMode;
using ::android::hardware::soundtrigger::V2_0::SoundModelHandle;
using ::android::hardware::soundtrigger::V2_0::SoundModelType;
using V2_0_ISoundTriggerHw = ::android::hardware::soundtrigger::V2_0::ISoundTriggerHw;
using V2_0_ISoundTriggerHwCallback =
    ::android::hardware::soundtrigger::V2_0::ISoundTriggerHwCallback;
using ::android::hardware::soundtrigger::V2_1::ISoundTriggerHw;
using ::android::hardware::soundtrigger::V2_1::ISoundTriggerHwCallback;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;

/**
 * Test code uses this class to wait for notification from callback.
 */
class Monitor {
   public:
    Monitor() : mCount(0) {}

    /**
     * Adds 1 to the internal counter and unblocks one of the waiting threads.
     */
    void notify() {
        std::unique_lock<std::mutex> lock(mMtx);
        mCount++;
        mCv.notify_one();
    }

    /**
     * Blocks until the internal counter becomes greater than 0.
     *
     * If notified, this method decreases the counter by 1 and returns true.
     * If timeout, returns false.
     */
    bool wait(int timeoutSeconds) {
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(timeoutSeconds);
        std::unique_lock<std::mutex> lock(mMtx);
        if (!mCv.wait_until(lock, deadline, [& count = mCount] { return count > 0; })) {
            return false;
        }
        mCount--;
        return true;
    }

   private:
    std::mutex mMtx;
    std::condition_variable mCv;
    int mCount;
};

// Test environment for SoundTrigger HIDL HAL.
class SoundTriggerHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static SoundTriggerHidlEnvironment* Instance() {
        static SoundTriggerHidlEnvironment* instance = new SoundTriggerHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<ISoundTriggerHw>(); }

   private:
    SoundTriggerHidlEnvironment() {}
};

// The main test class for Sound Trigger HIDL HAL.
class SoundTriggerHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        mSoundTriggerHal = ::testing::VtsHalHidlTargetTestBase::getService<ISoundTriggerHw>(
            SoundTriggerHidlEnvironment::Instance()->getServiceName<ISoundTriggerHw>());
        ASSERT_NE(nullptr, mSoundTriggerHal.get());
        mCallback = new SoundTriggerHwCallback(*this);
        ASSERT_NE(nullptr, mCallback.get());
    }

    static void SetUpTestCase() { srand(1234); }

    class SoundTriggerHwCallback : public ISoundTriggerHwCallback {
       private:
        SoundTriggerHidlTest& mParent;

       public:
        SoundTriggerHwCallback(SoundTriggerHidlTest& parent) : mParent(parent) {}

        Return<void> recognitionCallback(const V2_0_ISoundTriggerHwCallback::RecognitionEvent& event
                                             __unused,
                                         int32_t cookie __unused) override {
            ALOGI("%s", __FUNCTION__);
            return Void();
        };

        Return<void> phraseRecognitionCallback(
            const V2_0_ISoundTriggerHwCallback::PhraseRecognitionEvent& event __unused,
            int32_t cookie __unused) override {
            ALOGI("%s", __FUNCTION__);
            return Void();
        };

        Return<void> soundModelCallback(const V2_0_ISoundTriggerHwCallback::ModelEvent& event,
                                        int32_t cookie __unused) override {
            ALOGI("%s", __FUNCTION__);
            mParent.lastModelEvent_2_0 = event;
            mParent.monitor.notify();
            return Void();
        }

        Return<void> recognitionCallback_2_1(const ISoundTriggerHwCallback::RecognitionEvent& event
                                                 __unused,
                                             int32_t cookie __unused) override {
            ALOGI("%s", __FUNCTION__);
            return Void();
        }

        Return<void> phraseRecognitionCallback_2_1(
            const ISoundTriggerHwCallback::PhraseRecognitionEvent& event __unused,
            int32_t cookie __unused) override {
            ALOGI("%s", __FUNCTION__);
            return Void();
        }

        Return<void> soundModelCallback_2_1(const ISoundTriggerHwCallback::ModelEvent& event,
                                            int32_t cookie __unused) {
            ALOGI("%s", __FUNCTION__);
            mParent.lastModelEvent = event;
            mParent.monitor.notify();
            return Void();
        }
    };

    virtual void TearDown() override {}

    Monitor monitor;
    // updated by soundModelCallback()
    V2_0_ISoundTriggerHwCallback::ModelEvent lastModelEvent_2_0;
    // updated by soundModelCallback_2_1()
    ISoundTriggerHwCallback::ModelEvent lastModelEvent;

   protected:
    sp<ISoundTriggerHw> mSoundTriggerHal;
    sp<SoundTriggerHwCallback> mCallback;
};

/**
 * Test ISoundTriggerHw::getProperties() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the method returns 0 (no error)
 *  - the implementation supports at least one sound model and one key phrase
 *  - the implementation supports at least VOICE_TRIGGER recognition mode
 */
TEST_F(SoundTriggerHidlTest, GetProperties) {
    ISoundTriggerHw::Properties halProperties;
    Return<void> hidlReturn;
    int ret = -ENODEV;

    hidlReturn = mSoundTriggerHal->getProperties([&](int rc, auto res) {
        ret = rc;
        halProperties = res;
    });

    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_EQ(0, ret);
    EXPECT_GT(halProperties.maxSoundModels, 0u);
    EXPECT_GT(halProperties.maxKeyPhrases, 0u);
    EXPECT_NE(0u, (halProperties.recognitionModes & (uint32_t)RecognitionMode::VOICE_TRIGGER));
}

/**
 * Test ISoundTriggerHw::loadPhraseSoundModel() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the implementation returns an error when passed a malformed sound model
 *
 * There is no way to verify that implementation actually can load a sound model because each
 * sound model is vendor specific.
 */
TEST_F(SoundTriggerHidlTest, LoadInvalidModelFail) {
    Return<void> hidlReturn;
    int ret = -ENODEV;
    V2_0_ISoundTriggerHw::PhraseSoundModel model;
    SoundModelHandle handle;

    model.common.type = SoundModelType::UNKNOWN;

    hidlReturn =
        mSoundTriggerHal->loadPhraseSoundModel(model, mCallback, 0, [&](int32_t retval, auto res) {
            ret = retval;
            handle = res;
        });

    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_NE(0, ret);
    EXPECT_FALSE(monitor.wait(SHORT_TIMEOUT_PERIOD));
}

/**
 * Test ISoundTriggerHw::loadPhraseSoundModel_2_1() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the implementation returns an error when passed a malformed sound model
 *
 * There is no way to verify that implementation actually can load a sound model because each
 * sound model is vendor specific.
 */
TEST_F(SoundTriggerHidlTest, LoadInvalidModelFail_2_1) {
    Return<void> hidlReturn;
    int ret = -ENODEV;
    ISoundTriggerHw::PhraseSoundModel model;
    SoundModelHandle handle;

    model.common.header.type = SoundModelType::UNKNOWN;

    hidlReturn = mSoundTriggerHal->loadPhraseSoundModel_2_1(model, mCallback, 0,
                                                            [&](int32_t retval, auto res) {
                                                                ret = retval;
                                                                handle = res;
                                                            });

    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_NE(0, ret);
    EXPECT_FALSE(monitor.wait(SHORT_TIMEOUT_PERIOD));
}

/**
 * Test ISoundTriggerHw::loadSoundModel() method
 *
 * Verifies that:
 *  - the implementation returns an error when passed an empty sound model
 */
TEST_F(SoundTriggerHidlTest, LoadEmptyGenericSoundModelFail) {
    int ret = -ENODEV;
    V2_0_ISoundTriggerHw::SoundModel model;
    SoundModelHandle handle = 0;

    model.type = SoundModelType::GENERIC;

    Return<void> loadReturn =
        mSoundTriggerHal->loadSoundModel(model, mCallback, 0, [&](int32_t retval, auto res) {
            ret = retval;
            handle = res;
        });

    EXPECT_TRUE(loadReturn.isOk());
    EXPECT_NE(0, ret);
    EXPECT_FALSE(monitor.wait(SHORT_TIMEOUT_PERIOD));
}

/**
 * Test ISoundTriggerHw::loadSoundModel() method
 *
 * Verifies that:
 *  - the implementation returns error when passed a sound model with random data.
 */
TEST_F(SoundTriggerHidlTest, LoadGenericSoundModelFail) {
    int ret = -ENODEV;
    V2_0_ISoundTriggerHw::SoundModel model;
    SoundModelHandle handle = 0;

    model.type = SoundModelType::GENERIC;
    model.data.resize(100);
    for (auto& d : model.data) {
        d = rand();
    }

    Return<void> loadReturn =
        mSoundTriggerHal->loadSoundModel(model, mCallback, 0, [&](int32_t retval, auto res) {
            ret = retval;
            handle = res;
        });

    EXPECT_TRUE(loadReturn.isOk());
    EXPECT_NE(0, ret);
    EXPECT_FALSE(monitor.wait(SHORT_TIMEOUT_PERIOD));
}

/**
 * Test ISoundTriggerHw::loadSoundModel_2_1() method
 *
 * Verifies that:
 *  - the implementation returns error when passed a sound model with random data.
 */
TEST_F(SoundTriggerHidlTest, LoadEmptyGenericSoundModelFail_2_1) {
    int ret = -ENODEV;
    ISoundTriggerHw::SoundModel model;
    SoundModelHandle handle = 0;

    model.header.type = SoundModelType::GENERIC;

    Return<void> loadReturn =
        mSoundTriggerHal->loadSoundModel_2_1(model, mCallback, 0, [&](int32_t retval, auto res) {
            ret = retval;
            handle = res;
        });

    EXPECT_TRUE(loadReturn.isOk());
    EXPECT_NE(0, ret);
    EXPECT_FALSE(monitor.wait(SHORT_TIMEOUT_PERIOD));
}

/**
 * Test ISoundTriggerHw::loadSoundModel_2_1() method
 *
 * Verifies that:
 *  - the implementation returns error when passed a sound model with random data.
 */
TEST_F(SoundTriggerHidlTest, LoadGenericSoundModelFail_2_1) {
    int ret = -ENODEV;
    ISoundTriggerHw::SoundModel model;
    SoundModelHandle handle = 0;

    model.header.type = SoundModelType::GENERIC;
    sp<IAllocator> ashmem = IAllocator::getService("ashmem");
    ASSERT_NE(nullptr, ashmem.get());
    hidl_memory hmemory;
    int size = 100;
    Return<void> allocReturn = ashmem->allocate(size, [&](bool success, const hidl_memory& m) {
        ASSERT_TRUE(success);
        hmemory = m;
    });
    sp<IMemory> memory = ::android::hardware::mapMemory(hmemory);
    ASSERT_NE(nullptr, memory.get());
    memory->update();
    for (uint8_t *p = static_cast<uint8_t*>(static_cast<void*>(memory->getPointer())); size >= 0;
         p++, size--) {
        *p = rand();
    }

    Return<void> loadReturn =
        mSoundTriggerHal->loadSoundModel_2_1(model, mCallback, 0, [&](int32_t retval, auto res) {
            ret = retval;
            handle = res;
        });

    EXPECT_TRUE(loadReturn.isOk());
    EXPECT_NE(0, ret);
    EXPECT_FALSE(monitor.wait(SHORT_TIMEOUT_PERIOD));
}

/**
 * Test ISoundTriggerHw::unloadSoundModel() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the implementation returns an error when called without a valid loaded sound model
 *
 */
TEST_F(SoundTriggerHidlTest, UnloadModelNoModelFail) {
    Return<int32_t> hidlReturn(0);
    SoundModelHandle halHandle = 0;

    hidlReturn = mSoundTriggerHal->unloadSoundModel(halHandle);

    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_NE(0, hidlReturn);
}

/**
 * Test ISoundTriggerHw::startRecognition() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the implementation returns an error when called without a valid loaded sound model
 *
 * There is no way to verify that implementation actually starts recognition because no model can
 * be loaded.
 */
TEST_F(SoundTriggerHidlTest, StartRecognitionNoModelFail) {
    Return<int32_t> hidlReturn(0);
    SoundModelHandle handle = 0;
    PhraseRecognitionExtra phrase;
    V2_0_ISoundTriggerHw::RecognitionConfig config;

    config.captureHandle = 0;
    config.captureDevice = AudioDevice::IN_BUILTIN_MIC;
    phrase.id = 0;
    phrase.recognitionModes = (uint32_t)RecognitionMode::VOICE_TRIGGER;
    phrase.confidenceLevel = 0;

    config.phrases.setToExternal(&phrase, 1);

    hidlReturn = mSoundTriggerHal->startRecognition(handle, config, mCallback, 0);

    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_NE(0, hidlReturn);
}

/**
 * Test ISoundTriggerHw::startRecognition_2_1() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the implementation returns an error when called without a valid loaded sound model
 *
 * There is no way to verify that implementation actually starts recognition because no model can
 * be loaded.
 */
TEST_F(SoundTriggerHidlTest, StartRecognitionNoModelFail_2_1) {
    Return<int32_t> hidlReturn(0);
    SoundModelHandle handle = 0;
    PhraseRecognitionExtra phrase;
    ISoundTriggerHw::RecognitionConfig config;

    config.header.captureHandle = 0;
    config.header.captureDevice = AudioDevice::IN_BUILTIN_MIC;
    phrase.id = 0;
    phrase.recognitionModes = (uint32_t)RecognitionMode::VOICE_TRIGGER;
    phrase.confidenceLevel = 0;

    config.header.phrases.setToExternal(&phrase, 1);

    hidlReturn = mSoundTriggerHal->startRecognition_2_1(handle, config, mCallback, 0);

    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_NE(0, hidlReturn);
}

/**
 * Test ISoundTriggerHw::stopRecognition() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the implementation returns an error when called without an active recognition running
 *
 */
TEST_F(SoundTriggerHidlTest, StopRecognitionNoAStartFail) {
    Return<int32_t> hidlReturn(0);
    SoundModelHandle handle = 0;

    hidlReturn = mSoundTriggerHal->stopRecognition(handle);

    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_NE(0, hidlReturn);
}

/**
 * Test ISoundTriggerHw::stopAllRecognitions() method
 *
 * Verifies that:
 *  - the implementation implements this optional method or indicates it is not supported by
 *  returning -ENOSYS
 */
TEST_F(SoundTriggerHidlTest, stopAllRecognitions) {
    Return<int32_t> hidlReturn(0);

    hidlReturn = mSoundTriggerHal->stopAllRecognitions();

    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_TRUE(hidlReturn == 0 || hidlReturn == -ENOSYS);
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(SoundTriggerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    SoundTriggerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
