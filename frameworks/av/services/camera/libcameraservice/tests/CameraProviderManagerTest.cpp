/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "CameraProviderManagerTest"

#include "../common/CameraProviderManager.h"
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <android/hidl/manager/1.0/IServiceNotification.h>
#include <android/hardware/camera/device/3.2/ICameraDeviceCallback.h>
#include <android/hardware/camera/device/3.2/ICameraDeviceSession.h>
#include <camera_metadata_hidden.h>
#include <gtest/gtest.h>

using namespace android;
using namespace android::hardware::camera;
using android::hardware::camera::common::V1_0::Status;
using android::hardware::camera::common::V1_0::VendorTag;
using android::hardware::camera::common::V1_0::VendorTagSection;
using android::hardware::camera::common::V1_0::CameraMetadataType;
using android::hardware::camera::device::V3_2::ICameraDeviceCallback;
using android::hardware::camera::device::V3_2::ICameraDeviceSession;

/**
 * Basic test implementation of a camera ver. 3.2 device interface
 */
struct TestDeviceInterface : public device::V3_2::ICameraDevice {
    std::vector<hardware::hidl_string> mDeviceNames;
    TestDeviceInterface(std::vector<hardware::hidl_string> deviceNames) :
        mDeviceNames(deviceNames) {}
    using getResourceCost_cb = std::function<void(
            hardware::camera::common::V1_0::Status status,
            const hardware::camera::common::V1_0::CameraResourceCost& resourceCost)>;
    virtual ::android::hardware::Return<void> getResourceCost(
            getResourceCost_cb _hidl_cb) override {
        hardware::camera::common::V1_0::CameraResourceCost resourceCost = {100,
                mDeviceNames};
        _hidl_cb(Status::OK, resourceCost);
        return hardware::Void();
    }

    using getCameraCharacteristics_cb = std::function<void(
            hardware::camera::common::V1_0::Status status,
            const hardware::hidl_vec<uint8_t>& cameraCharacteristics)>;
    hardware::Return<void> getCameraCharacteristics(
            getCameraCharacteristics_cb _hidl_cb) override {
        hardware::hidl_vec<uint8_t> cameraCharacteristics;
        _hidl_cb(Status::OK, cameraCharacteristics);
        return hardware::Void();
    }

    hardware::Return<hardware::camera::common::V1_0::Status> setTorchMode(
            ::android::hardware::camera::common::V1_0::TorchMode) override {
        return Status::OK;
    }

    using open_cb = std::function<void(
            ::android::hardware::camera::common::V1_0::Status status,
             const ::android::sp<ICameraDeviceSession>& session)>;
    hardware::Return<void> open(
            const ::android::sp<ICameraDeviceCallback>&,
            open_cb _hidl_cb) override {
        sp<ICameraDeviceSession> deviceSession = nullptr;
        _hidl_cb(Status::OK, deviceSession);
        return hardware::Void();
    }

    hardware::Return<void> dumpState(
            const ::android::hardware::hidl_handle&) override {
        return hardware::Void();
    }
};

/**
 * Basic test implementation of a camera provider
 */
struct TestICameraProvider : virtual public provider::V2_4::ICameraProvider {
    sp<provider::V2_4::ICameraProviderCallback> mCallbacks;
    std::vector<hardware::hidl_string> mDeviceNames;
    sp<device::V3_2::ICameraDevice> mDeviceInterface;
    hardware::hidl_vec<common::V1_0::VendorTagSection> mVendorTagSections;

    TestICameraProvider(const std::vector<hardware::hidl_string> &devices,
            const hardware::hidl_vec<common::V1_0::VendorTagSection> &vendorSection) :
        mDeviceNames(devices),
        mDeviceInterface(new TestDeviceInterface(devices)),
        mVendorTagSections (vendorSection) {}

    virtual hardware::Return<Status> setCallback(
            const sp<provider::V2_4::ICameraProviderCallback>& callbacks) override {
        mCallbacks = callbacks;
        return hardware::Return<Status>(Status::OK);
    }

    using getVendorTags_cb = std::function<void(Status status,
            const hardware::hidl_vec<common::V1_0::VendorTagSection>& sections)>;
    hardware::Return<void> getVendorTags(getVendorTags_cb _hidl_cb) override {
        _hidl_cb(Status::OK, mVendorTagSections);
        return hardware::Void();
    }

    using isSetTorchModeSupported_cb = std::function<void(
            ::android::hardware::camera::common::V1_0::Status status,
             bool support)>;
    virtual ::hardware::Return<void> isSetTorchModeSupported(
            isSetTorchModeSupported_cb _hidl_cb) override {
        _hidl_cb(Status::OK, false);
        return hardware::Void();
    }

    using getCameraIdList_cb = std::function<void(Status status,
            const hardware::hidl_vec<hardware::hidl_string>& cameraDeviceNames)>;
    virtual hardware::Return<void> getCameraIdList(getCameraIdList_cb _hidl_cb) override {
        _hidl_cb(Status::OK, mDeviceNames);
        return hardware::Void();
    }

    using getCameraDeviceInterface_V1_x_cb = std::function<void(Status status,
            const sp<device::V1_0::ICameraDevice>& device)>;
    virtual hardware::Return<void> getCameraDeviceInterface_V1_x(
            const hardware::hidl_string& cameraDeviceName,
            getCameraDeviceInterface_V1_x_cb _hidl_cb) override {
        (void) cameraDeviceName;
        _hidl_cb(Status::OK, nullptr); //TODO: impl. of ver. 1.0 device interface
                                       //      otherwise enumeration will fail.
        return hardware::Void();
    }

    using getCameraDeviceInterface_V3_x_cb = std::function<void(Status status,
            const sp<device::V3_2::ICameraDevice>& device)>;
    virtual hardware::Return<void> getCameraDeviceInterface_V3_x(
            const hardware::hidl_string&,
            getCameraDeviceInterface_V3_x_cb _hidl_cb) override {
        _hidl_cb(Status::OK, mDeviceInterface);
        return hardware::Void();
    }

};

/**
 * Simple test version of the interaction proxy, to use to inject onRegistered calls to the
 * CameraProviderManager
 */
struct TestInteractionProxy : public CameraProviderManager::ServiceInteractionProxy {
    sp<hidl::manager::V1_0::IServiceNotification> mManagerNotificationInterface;
    sp<TestICameraProvider> mTestCameraProvider;

    TestInteractionProxy() {}
    void setProvider(sp<TestICameraProvider> provider) {
        mTestCameraProvider = provider;
    }

    std::string mLastRequestedServiceName;

    virtual ~TestInteractionProxy() {}

    virtual bool registerForNotifications(
            const std::string &serviceName,
            const sp<hidl::manager::V1_0::IServiceNotification> &notification) override {
        (void) serviceName;
        mManagerNotificationInterface = notification;
        return true;
    }

    virtual sp<hardware::camera::provider::V2_4::ICameraProvider> getService(
            const std::string &serviceName) override {
        mLastRequestedServiceName = serviceName;
        return mTestCameraProvider;
    }

};

struct TestStatusListener : public CameraProviderManager::StatusListener {
    ~TestStatusListener() {}

    void onDeviceStatusChanged(const String8 &,
            hardware::camera::common::V1_0::CameraDeviceStatus) override {}
    void onTorchStatusChanged(const String8 &,
            hardware::camera::common::V1_0::TorchModeStatus) override {}
    void onNewProviderRegistered() override {}
};

TEST(CameraProviderManagerTest, InitializeTest) {
    std::vector<hardware::hidl_string> deviceNames;
    deviceNames.push_back("device@3.2/test/0");
    deviceNames.push_back("device@1.0/test/0");
    deviceNames.push_back("device@3.2/test/1");
    hardware::hidl_vec<common::V1_0::VendorTagSection> vendorSection;
    status_t res;
    sp<CameraProviderManager> providerManager = new CameraProviderManager();
    sp<TestStatusListener> statusListener = new TestStatusListener();
    TestInteractionProxy serviceProxy;
    sp<TestICameraProvider> provider =  new TestICameraProvider(deviceNames,
            vendorSection);
    serviceProxy.setProvider(provider);

    res = providerManager->initialize(statusListener, &serviceProxy);
    ASSERT_EQ(res, OK) << "Unable to initialize provider manager";

    hardware::hidl_string legacyInstanceName = "legacy/0";
    ASSERT_EQ(serviceProxy.mLastRequestedServiceName, legacyInstanceName) <<
            "Legacy instance not requested from service manager";

    hardware::hidl_string testProviderFqInterfaceName =
            "android.hardware.camera.provider@2.4::ICameraProvider";
    hardware::hidl_string testProviderInstanceName = "test/0";
    serviceProxy.mManagerNotificationInterface->onRegistration(
            testProviderFqInterfaceName,
            testProviderInstanceName, false);

    ASSERT_EQ(serviceProxy.mLastRequestedServiceName, testProviderInstanceName) <<
            "Incorrect instance requested from service manager";
}

TEST(CameraProviderManagerTest, MultipleVendorTagTest) {
    hardware::hidl_string sectionName = "VendorTestSection";
    hardware::hidl_string tagName = "VendorTestTag";
    uint32_t tagId = VENDOR_SECTION << 16;
    hardware::hidl_vec<common::V1_0::VendorTagSection> vendorSection;
    CameraMetadataType tagType = CameraMetadataType::BYTE;
    vendorSection.resize(1);
    vendorSection[0].sectionName = sectionName;
    vendorSection[0].tags.resize(1);
    vendorSection[0].tags[0].tagId = tagId;
    vendorSection[0].tags[0].tagName = tagName;
    vendorSection[0].tags[0].tagType = tagType;
    std::vector<hardware::hidl_string> deviceNames = {"device@3.2/test/0"};

    sp<CameraProviderManager> providerManager = new CameraProviderManager();
    sp<TestStatusListener> statusListener = new TestStatusListener();
    TestInteractionProxy serviceProxy;

    sp<TestICameraProvider> provider =  new TestICameraProvider(deviceNames,
            vendorSection);
    serviceProxy.setProvider(provider);

    auto res = providerManager->initialize(statusListener, &serviceProxy);
    ASSERT_EQ(res, OK) << "Unable to initialize provider manager";

    hardware::hidl_string testProviderInstanceName = "test/0";
    hardware::hidl_string testProviderFqInterfaceName =
            "android.hardware.camera.provider@2.4::ICameraProvider";
    serviceProxy.mManagerNotificationInterface->onRegistration(
            testProviderFqInterfaceName, testProviderInstanceName, false);
    ASSERT_EQ(serviceProxy.mLastRequestedServiceName, testProviderInstanceName) <<
            "Incorrect instance requested from service manager";

    hardware::hidl_string sectionNameSecond = "SecondVendorTestSection";
    hardware::hidl_string secondTagName = "SecondVendorTestTag";
    CameraMetadataType secondTagType = CameraMetadataType::DOUBLE;
    vendorSection[0].sectionName = sectionNameSecond;
    vendorSection[0].tags[0].tagId = tagId;
    vendorSection[0].tags[0].tagName = secondTagName;
    vendorSection[0].tags[0].tagType = secondTagType;
    deviceNames = {"device@3.2/test2/1"};

    sp<TestICameraProvider> secondProvider =  new TestICameraProvider(
            deviceNames, vendorSection);
    serviceProxy.setProvider(secondProvider);
    hardware::hidl_string testProviderSecondInstanceName = "test2/0";
    serviceProxy.mManagerNotificationInterface->onRegistration(
            testProviderFqInterfaceName, testProviderSecondInstanceName, false);
    ASSERT_EQ(serviceProxy.mLastRequestedServiceName,
              testProviderSecondInstanceName) <<
            "Incorrect instance requested from service manager";

    ASSERT_EQ(NO_ERROR , providerManager->setUpVendorTags());
    sp<VendorTagDescriptorCache> vendorCache =
            VendorTagDescriptorCache::getGlobalVendorTagCache();
    ASSERT_NE(nullptr, vendorCache.get());

    metadata_vendor_id_t vendorId = std::hash<std::string> {} (
            testProviderInstanceName.c_str());
    metadata_vendor_id_t vendorIdSecond = std::hash<std::string> {} (
            testProviderSecondInstanceName.c_str());

    hardware::hidl_string resultTag = vendorCache->getTagName(tagId, vendorId);
    ASSERT_EQ(resultTag, tagName);

    resultTag = vendorCache->getTagName(tagId, vendorIdSecond);
    ASSERT_EQ(resultTag, secondTagName);

    // Check whether we can create two separate CameraMetadata instances
    // using different tag vendor vendors.
    camera_metadata *metaBuffer = allocate_camera_metadata(10, 20);
    ASSERT_NE(nullptr, metaBuffer);
    set_camera_metadata_vendor_id(metaBuffer, vendorId);
    CameraMetadata metadata(metaBuffer);

    uint8_t byteVal = 10;
    ASSERT_TRUE(metadata.isEmpty());
    ASSERT_EQ(OK, metadata.update(tagId, &byteVal, 1));
    ASSERT_FALSE(metadata.isEmpty());
    ASSERT_TRUE(metadata.exists(tagId));

    metaBuffer = allocate_camera_metadata(10, 20);
    ASSERT_NE(nullptr, metaBuffer);
    set_camera_metadata_vendor_id(metaBuffer, vendorIdSecond);
    CameraMetadata secondMetadata(metaBuffer);

    ASSERT_TRUE(secondMetadata.isEmpty());
    double doubleVal = 1.0f;
    ASSERT_EQ(OK, secondMetadata.update(tagId, &doubleVal, 1));
    ASSERT_FALSE(secondMetadata.isEmpty());
    ASSERT_TRUE(secondMetadata.exists(tagId));

    // Check whether CameraMetadata copying works as expected
    CameraMetadata metadataCopy(metadata);
    ASSERT_FALSE(metadataCopy.isEmpty());
    ASSERT_TRUE(metadataCopy.exists(tagId));
    ASSERT_EQ(OK, metadataCopy.update(tagId, &byteVal, 1));
    ASSERT_TRUE(metadataCopy.exists(tagId));

    // Check whether values are as expected
    camera_metadata_entry_t entry = metadata.find(tagId);
    ASSERT_EQ(1u, entry.count);
    ASSERT_EQ(byteVal, entry.data.u8[0]);
    entry = secondMetadata.find(tagId);
    ASSERT_EQ(1u, entry.count);
    ASSERT_EQ(doubleVal, entry.data.d[0]);

    // Swap and erase
    secondMetadata.swap(metadataCopy);
    ASSERT_TRUE(metadataCopy.exists(tagId));
    ASSERT_TRUE(secondMetadata.exists(tagId));
    ASSERT_EQ(OK, secondMetadata.erase(tagId));
    ASSERT_TRUE(secondMetadata.isEmpty());
    doubleVal = 0.0f;
    ASSERT_EQ(OK, metadataCopy.update(tagId, &doubleVal, 1));
    entry = metadataCopy.find(tagId);
    ASSERT_EQ(1u, entry.count);
    ASSERT_EQ(doubleVal, entry.data.d[0]);

    // Append
    uint8_t sceneMode = ANDROID_CONTROL_SCENE_MODE_ACTION;
    secondMetadata.update(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);
    // Append from two different vendor tag providers is not supported!
    ASSERT_NE(OK, metadataCopy.append(secondMetadata));
    ASSERT_EQ(OK, metadataCopy.erase(tagId));
    metadataCopy.update(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);
    // However appending from same vendor tag provider should be fine
    ASSERT_EQ(OK, metadata.append(secondMetadata));
    // Append from a metadata without vendor tag provider should be supported
    CameraMetadata regularMetadata(10, 20);
    uint8_t controlMode = ANDROID_CONTROL_MODE_AUTO;
    regularMetadata.update(ANDROID_CONTROL_MODE, &controlMode, 1);
    ASSERT_EQ(OK, secondMetadata.append(regularMetadata));
    ASSERT_EQ(2u, secondMetadata.entryCount());
    ASSERT_EQ(2u, metadata.entryCount());

    // Dump
    metadata.dump(1, 2);
    metadataCopy.dump(1, 2);
    secondMetadata.dump(1, 2);
}
