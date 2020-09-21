// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#define LOG_NDEBUG 0
#define LOG_TAG "C2VDACompIntf_test"

#include <C2VDAComponent.h>

#include <C2PlatformSupport.h>

#include <gtest/gtest.h>
#include <utils/Log.h>

#include <inttypes.h>
#include <stdio.h>
#include <limits>

#define UNUSED(expr)  \
    do {              \
        (void)(expr); \
    } while (0)

namespace android {

const C2String testCompName = "c2.vda.avc.decoder";
const c2_node_id_t testCompNodeId = 12345;

const char* MEDIA_MIMETYPE_VIDEO_RAW = "video/raw";
const char* MEDIA_MIMETYPE_VIDEO_AVC = "video/avc";

const C2Allocator::id_t kInputAllocators[] = {C2PlatformAllocatorStore::ION};
const C2Allocator::id_t kOutputAllocators[] = {C2PlatformAllocatorStore::GRALLOC};
const C2BlockPool::local_id_t kDefaultOutputBlockPool = C2BlockPool::BASIC_GRAPHIC;

class C2VDACompIntfTest : public ::testing::Test {
protected:
    C2VDACompIntfTest() {
        mReflector = std::make_shared<C2ReflectorHelper>();
        mIntf = std::shared_ptr<C2ComponentInterface>(new SimpleInterface<C2VDAComponent::IntfImpl>(
                testCompName.c_str(), testCompNodeId,
                std::make_shared<C2VDAComponent::IntfImpl>(testCompName, mReflector)));
    }
    ~C2VDACompIntfTest() override {}

    template <typename T>
    void testReadOnlyParam(const T* expected, T* invalid);

    template <typename T>
    void checkReadOnlyFailureOnConfig(T* param);

    template <typename T>
    void testReadOnlyParamOnStack(const T* expected, T* invalid);

    template <typename T>
    void testReadOnlyParamOnHeap(const T* expected, T* invalid);

    template <typename T>
    void testWritableParam(T* newParam);

    template <typename T>
    void testInvalidWritableParam(T* invalidParam);

    template <typename T>
    void testWritableVideoSizeParam(int32_t widthMin, int32_t widthMax, int32_t widthStep,
                                    int32_t heightMin, int32_t heightMax, int32_t heightStep);

    std::shared_ptr<C2ComponentInterface> mIntf;
    std::shared_ptr<C2ReflectorHelper> mReflector;
};

template <typename T>
void C2VDACompIntfTest::testReadOnlyParam(const T* expected, T* invalid) {
    testReadOnlyParamOnStack(expected, invalid);
    testReadOnlyParamOnHeap(expected, invalid);
}

template <typename T>
void C2VDACompIntfTest::checkReadOnlyFailureOnConfig(T* param) {
    std::vector<C2Param*> params{param};
    std::vector<std::unique_ptr<C2SettingResult>> failures;

    // TODO: do not assert on checking return value since it is not consistent for
    //       C2InterfaceHelper now. (b/79720928)
    //   1) if config same value, it returns C2_OK
    //   2) if config different value, it returns C2_CORRUPTED. But when you config again, it
    //      returns C2_OK
    //ASSERT_EQ(C2_BAD_VALUE, mIntf->config_vb(params, C2_DONT_BLOCK, &failures));
    mIntf->config_vb(params, C2_DONT_BLOCK, &failures);

    // TODO: failure is not yet supported for C2InterfaceHelper
    //ASSERT_EQ(1u, failures.size());
    //EXPECT_EQ(C2SettingResult::READ_ONLY, failures[0]->failure);
}

template <typename T>
void C2VDACompIntfTest::testReadOnlyParamOnStack(const T* expected, T* invalid) {
    T param;
    std::vector<C2Param*> stackParams{&param};
    ASSERT_EQ(C2_OK, mIntf->query_vb(stackParams, {}, C2_DONT_BLOCK, nullptr));
    EXPECT_EQ(*expected, param);

    checkReadOnlyFailureOnConfig(&param);
    checkReadOnlyFailureOnConfig(invalid);

    // The param must not change after failed config.
    ASSERT_EQ(C2_OK, mIntf->query_vb(stackParams, {}, C2_DONT_BLOCK, nullptr));
    EXPECT_EQ(*expected, param);
}

template <typename T>
void C2VDACompIntfTest::testReadOnlyParamOnHeap(const T* expected, T* invalid) {
    std::vector<std::unique_ptr<C2Param>> heapParams;

    uint32_t index = expected->index();

    ASSERT_EQ(C2_OK, mIntf->query_vb({}, {index}, C2_DONT_BLOCK, &heapParams));
    ASSERT_EQ(1u, heapParams.size());
    EXPECT_EQ(*expected, *heapParams[0]);

    checkReadOnlyFailureOnConfig(heapParams[0].get());
    checkReadOnlyFailureOnConfig(invalid);

    // The param must not change after failed config.
    heapParams.clear();
    ASSERT_EQ(C2_OK, mIntf->query_vb({}, {index}, C2_DONT_BLOCK, &heapParams));
    ASSERT_EQ(1u, heapParams.size());
    EXPECT_EQ(*expected, *heapParams[0]);
}

template <typename T>
void C2VDACompIntfTest::testWritableParam(T* newParam) {
    std::vector<C2Param*> params{newParam};
    std::vector<std::unique_ptr<C2SettingResult>> failures;
    ASSERT_EQ(C2_OK, mIntf->config_vb(params, C2_DONT_BLOCK, &failures));
    EXPECT_EQ(0u, failures.size());

    // The param must change to newParam
    // Check like param on stack
    T param;
    std::vector<C2Param*> stackParams{&param};
    ASSERT_EQ(C2_OK, mIntf->query_vb(stackParams, {}, C2_DONT_BLOCK, nullptr));
    EXPECT_EQ(*newParam, param);

    // Check also like param on heap
    std::vector<std::unique_ptr<C2Param>> heapParams;
    ASSERT_EQ(C2_OK, mIntf->query_vb({}, {newParam->index()}, C2_DONT_BLOCK, &heapParams));
    ASSERT_EQ(1u, heapParams.size());
    EXPECT_EQ(*newParam, *heapParams[0]);
}

template <typename T>
void C2VDACompIntfTest::testInvalidWritableParam(T* invalidParam) {
    // Get the current parameter info
    T preParam;
    std::vector<C2Param*> stackParams{&preParam};
    ASSERT_EQ(C2_OK, mIntf->query_vb(stackParams, {}, C2_DONT_BLOCK, nullptr));

    // Config invalid value. The failure is expected
    std::vector<C2Param*> params{invalidParam};
    std::vector<std::unique_ptr<C2SettingResult>> failures;
    ASSERT_EQ(C2_BAD_VALUE, mIntf->config_vb(params, C2_DONT_BLOCK, &failures));
    EXPECT_EQ(1u, failures.size());

    //The param must not change after config failed
    T param;
    std::vector<C2Param*> stackParams2{&param};
    ASSERT_EQ(C2_OK, mIntf->query_vb(stackParams2, {}, C2_DONT_BLOCK, nullptr));
    EXPECT_EQ(preParam, param);

    // Check also like param on heap
    std::vector<std::unique_ptr<C2Param>> heapParams;
    ASSERT_EQ(C2_OK, mIntf->query_vb({}, {invalidParam->index()}, C2_DONT_BLOCK, &heapParams));
    ASSERT_EQ(1u, heapParams.size());
    EXPECT_EQ(preParam, *heapParams[0]);
}

bool isUnderflowSubstract(int32_t a, int32_t b) {
    return a < 0 && b > a - std::numeric_limits<int32_t>::min();
}

bool isOverflowAdd(int32_t a, int32_t b) {
    return a > 0 && b > std::numeric_limits<int32_t>::max() - a;
}

template <typename T>
void C2VDACompIntfTest::testWritableVideoSizeParam(int32_t widthMin, int32_t widthMax,
                                                   int32_t widthStep, int32_t heightMin,
                                                   int32_t heightMax, int32_t heightStep) {
    // Test supported values of video size
    T valid;
    for (int32_t h = heightMin; h <= heightMax; h += heightStep) {
        for (int32_t w = widthMin; w <= widthMax; w += widthStep) {
            valid.width = w;
            valid.height = h;
            {
                SCOPED_TRACE("testWritableParam");
                testWritableParam(&valid);
                if (HasFailure()) {
                    printf("Failed while config width = %d, height = %d\n", valid.width,
                           valid.height);
                }
                if (HasFatalFailure()) return;
            }
        }
    }

    // TODO: validate possible values in C2InterfaceHelper is not implemented yet.
    //// Test invalid values video size
    //T invalid;
    //// Width or height is smaller than min values
    //if (!isUnderflowSubstract(widthMin, widthStep)) {
    //    invalid.width = widthMin - widthStep;
    //    invalid.height = heightMin;
    //    testInvalidWritableParam(&invalid);
    //}
    //if (!isUnderflowSubstract(heightMin, heightStep)) {
    //    invalid.width = widthMin;
    //    invalid.height = heightMin - heightStep;
    //    testInvalidWritableParam(&invalid);
    //}

    //// Width or height is bigger than max values
    //if (!isOverflowAdd(widthMax, widthStep)) {
    //    invalid.width = widthMax + widthStep;
    //    invalid.height = heightMax;
    //    testInvalidWritableParam(&invalid);
    //}
    //if (!isOverflowAdd(heightMax, heightStep)) {
    //    invalid.width = widthMax;
    //    invalid.height = heightMax + heightStep;
    //    testInvalidWritableParam(&invalid);
    //}

    //// Invalid width/height within the range
    //if (widthStep != 1) {
    //    invalid.width = widthMin + 1;
    //    invalid.height = heightMin;
    //    testInvalidWritableParam(&invalid);
    //}
    //if (heightStep != 1) {
    //    invalid.width = widthMin;
    //    invalid.height = heightMin + 1;
    //    testInvalidWritableParam(&invalid);
    //}
}

#define TRACED_FAILURE(func)                            \
    do {                                                \
        SCOPED_TRACE(#func);                            \
        func;                                           \
        if (::testing::Test::HasFatalFailure()) return; \
    } while (false)

TEST_F(C2VDACompIntfTest, CreateInstance) {
    auto name = mIntf->getName();
    auto id = mIntf->getId();
    printf("name = %s\n", name.c_str());
    printf("node_id = %u\n", id);
    EXPECT_STREQ(name.c_str(), testCompName.c_str());
    EXPECT_EQ(id, testCompNodeId);
}

TEST_F(C2VDACompIntfTest, TestInputFormat) {
    C2StreamBufferTypeSetting::input expected(0u, C2FormatCompressed);
    expected.setStream(0);  // only support single stream
    C2StreamBufferTypeSetting::input invalid(0u, C2FormatVideo);
    invalid.setStream(0);  // only support single stream
    TRACED_FAILURE(testReadOnlyParam(&expected, &invalid));
}

TEST_F(C2VDACompIntfTest, TestOutputFormat) {
    C2StreamBufferTypeSetting::output expected(0u, C2FormatVideo);
    expected.setStream(0);  // only support single stream
    C2StreamBufferTypeSetting::output invalid(0u, C2FormatCompressed);
    invalid.setStream(0);  // only support single stream
    TRACED_FAILURE(testReadOnlyParam(&expected, &invalid));
}

TEST_F(C2VDACompIntfTest, TestInputPortMime) {
    std::shared_ptr<C2PortMediaTypeSetting::input> expected(
            AllocSharedString<C2PortMediaTypeSetting::input>(MEDIA_MIMETYPE_VIDEO_AVC));
    std::shared_ptr<C2PortMediaTypeSetting::input> invalid(
            AllocSharedString<C2PortMediaTypeSetting::input>(MEDIA_MIMETYPE_VIDEO_RAW));
    TRACED_FAILURE(testReadOnlyParamOnHeap(expected.get(), invalid.get()));
}

TEST_F(C2VDACompIntfTest, TestOutputPortMime) {
    std::shared_ptr<C2PortMediaTypeSetting::output> expected(
            AllocSharedString<C2PortMediaTypeSetting::output>(MEDIA_MIMETYPE_VIDEO_RAW));
    std::shared_ptr<C2PortMediaTypeSetting::output> invalid(
            AllocSharedString<C2PortMediaTypeSetting::output>(MEDIA_MIMETYPE_VIDEO_AVC));
    TRACED_FAILURE(testReadOnlyParamOnHeap(expected.get(), invalid.get()));
}

TEST_F(C2VDACompIntfTest, TestVideoSize) {
    C2StreamPictureSizeInfo::output videoSize;
    videoSize.setStream(0);  // only support single stream
    std::vector<C2FieldSupportedValuesQuery> widthC2FSV = {
            {C2ParamField(&videoSize, &C2StreamPictureSizeInfo::width),
             C2FieldSupportedValuesQuery::CURRENT},
    };
    ASSERT_EQ(C2_OK, mIntf->querySupportedValues_vb(widthC2FSV, C2_DONT_BLOCK));
    std::vector<C2FieldSupportedValuesQuery> heightC2FSV = {
            {C2ParamField(&videoSize, &C2StreamPictureSizeInfo::height),
             C2FieldSupportedValuesQuery::CURRENT},
    };
    ASSERT_EQ(C2_OK, mIntf->querySupportedValues_vb(heightC2FSV, C2_DONT_BLOCK));
    ASSERT_EQ(1u, widthC2FSV.size());
    ASSERT_EQ(C2_OK, widthC2FSV[0].status);
    ASSERT_EQ(C2FieldSupportedValues::RANGE, widthC2FSV[0].values.type);
    auto& widthFSVRange = widthC2FSV[0].values.range;
    int32_t widthMin = widthFSVRange.min.i32;
    int32_t widthMax = widthFSVRange.max.i32;
    int32_t widthStep = widthFSVRange.step.i32;

    ASSERT_EQ(1u, heightC2FSV.size());
    ASSERT_EQ(C2_OK, heightC2FSV[0].status);
    ASSERT_EQ(C2FieldSupportedValues::RANGE, heightC2FSV[0].values.type);
    auto& heightFSVRange = heightC2FSV[0].values.range;
    int32_t heightMin = heightFSVRange.min.i32;
    int32_t heightMax = heightFSVRange.max.i32;
    int32_t heightStep = heightFSVRange.step.i32;

    // test updating valid and invalid values
    TRACED_FAILURE(testWritableVideoSizeParam<C2StreamPictureSizeInfo::output>(
            widthMin, widthMax, widthStep, heightMin, heightMax, heightStep));
}

TEST_F(C2VDACompIntfTest, TestInputAllocatorIds) {
    std::shared_ptr<C2PortAllocatorsTuning::input> expected(
            C2PortAllocatorsTuning::input::AllocShared(kInputAllocators));
    std::shared_ptr<C2PortAllocatorsTuning::input> invalid(
            C2PortAllocatorsTuning::input::AllocShared(kOutputAllocators));
    TRACED_FAILURE(testReadOnlyParamOnHeap(expected.get(), invalid.get()));
}

TEST_F(C2VDACompIntfTest, TestOutputAllocatorIds) {
    std::shared_ptr<C2PortAllocatorsTuning::output> expected(
            C2PortAllocatorsTuning::output::AllocShared(kOutputAllocators));
    std::shared_ptr<C2PortAllocatorsTuning::output> invalid(
            C2PortAllocatorsTuning::output::AllocShared(kInputAllocators));
    TRACED_FAILURE(testReadOnlyParamOnHeap(expected.get(), invalid.get()));
}

TEST_F(C2VDACompIntfTest, TestOutputBlockPoolIds) {
    std::vector<std::unique_ptr<C2Param>> heapParams;
    C2Param::Index index = C2PortBlockPoolsTuning::output::PARAM_TYPE;

    // Query the param and check the default value.
    ASSERT_EQ(C2_OK, mIntf->query_vb({}, {index}, C2_DONT_BLOCK, &heapParams));
    ASSERT_EQ(1u, heapParams.size());
    C2BlockPool::local_id_t value = ((C2PortBlockPoolsTuning*)heapParams[0].get())->m.values[0];
    ASSERT_EQ(kDefaultOutputBlockPool, value);

    // Configure the param.
    C2BlockPool::local_id_t configBlockPools[] = {C2BlockPool::PLATFORM_START + 1};
    std::shared_ptr<C2PortBlockPoolsTuning::output> newParam(
            C2PortBlockPoolsTuning::output::AllocShared(configBlockPools));

    std::vector<C2Param*> params{newParam.get()};
    std::vector<std::unique_ptr<C2SettingResult>> failures;
    ASSERT_EQ(C2_OK, mIntf->config_vb(params, C2_DONT_BLOCK, &failures));
    EXPECT_EQ(0u, failures.size());

    // Query the param again and check the value is as configured
    heapParams.clear();
    ASSERT_EQ(C2_OK, mIntf->query_vb({}, {index}, C2_DONT_BLOCK, &heapParams));
    ASSERT_EQ(1u, heapParams.size());
    value = ((C2PortBlockPoolsTuning*)heapParams[0].get())->m.values[0];
    ASSERT_EQ(configBlockPools[0], value);
}

TEST_F(C2VDACompIntfTest, TestUnsupportedParam) {
    C2ComponentTemporalInfo unsupportedParam;
    std::vector<C2Param*> stackParams{&unsupportedParam};
    ASSERT_EQ(C2_BAD_INDEX, mIntf->query_vb(stackParams, {}, C2_DONT_BLOCK, nullptr));
    EXPECT_EQ(0u, unsupportedParam.size());  // invalidated
}

void dumpType(const C2FieldDescriptor::type_t type) {
    switch (type) {
    case C2FieldDescriptor::INT32:
        printf("int32_t");
        break;
    case C2FieldDescriptor::UINT32:
        printf("uint32_t");
        break;
    case C2FieldDescriptor::INT64:
        printf("int64_t");
        break;
    case C2FieldDescriptor::UINT64:
        printf("uint64_t");
        break;
    case C2FieldDescriptor::FLOAT:
        printf("float");
        break;
    default:
        printf("<flex>");
        break;
    }
}

void dumpStruct(const C2StructDescriptor& sd) {
    printf("  struct: { ");
    for (const C2FieldDescriptor& f : sd) {
        printf("%s:", f.name().c_str());
        dumpType(f.type());
        printf(", ");
    }
    printf("}\n");
}

TEST_F(C2VDACompIntfTest, ParamReflector) {
    std::vector<std::shared_ptr<C2ParamDescriptor>> params;

    ASSERT_EQ(mIntf->querySupportedParams_nb(&params), C2_OK);
    for (const auto& paramDesc : params) {
        printf("name: %s\n", paramDesc->name().c_str());
        printf("  required: %s\n", paramDesc->isRequired() ? "yes" : "no");
        printf("  type: %x\n", paramDesc->index().type());
        std::unique_ptr<C2StructDescriptor> desc{mReflector->describe(paramDesc->index().type())};
        if (desc.get()) dumpStruct(*desc);
    }
}
}  // namespace android
