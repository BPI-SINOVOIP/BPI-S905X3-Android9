/*
 * Copyright (C) 2017 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "hidl_test_java_native"

#include <android-base/logging.h>

#include <android/hardware/tests/baz/1.0/IBaz.h>

#include <hidl/LegacySupport.h>
#include <hidl/ServiceManagement.h>
#include <gtest/gtest.h>

#include <hidl/HidlTransportSupport.h>
#include <hidl/Status.h>
using ::android::sp;
using ::android::hardware::tests::baz::V1_0::IBase;
using ::android::hardware::tests::baz::V1_0::IBaz;
using ::android::hardware::tests::baz::V1_0::IBazCallback;

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::defaultPassthroughServiceImplementation;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct BazCallback : public IBazCallback {
    Return<void> heyItsMe(const sp<IBazCallback> &cb) override;
    Return<void> hey() override;
};

Return<void> BazCallback::heyItsMe(
        const sp<IBazCallback> &cb) {
    LOG(INFO) << "SERVER: heyItsMe cb = " << cb.get();

    return Void();
}

Return<void> BazCallback::hey() {
    LOG(INFO) << "SERVER: hey";

    return Void();
}

using std::to_string;

static void usage(const char *me) {
    fprintf(stderr, "%s [-c]lient | [-s]erver\n", me);
}

struct HidlEnvironment : public ::testing::Environment {
    void SetUp() override {
    }

    void TearDown() override {
    }
};

struct HidlTest : public ::testing::Test {
    sp<IBaz> baz;

    void SetUp() override {
        using namespace ::android::hardware;

        ::android::hardware::details::waitForHwService(
                IBaz::descriptor, "baz");

        baz = IBaz::getService("baz");

        CHECK(baz != NULL);
        CHECK(baz->isRemote());
    }

    void TearDown() override {
    }
};

template <typename T>
static void EXPECT_OK(const ::android::hardware::Return<T> &ret) {
    EXPECT_TRUE(ret.isOk());
}

TEST_F(HidlTest, GetDescriptorTest) {
    EXPECT_OK(baz->interfaceDescriptor([&] (const auto &desc) {
        EXPECT_EQ(desc, IBaz::descriptor);
    }));
}

TEST_F(HidlTest, BazSomeBaseMethodTest) {
    EXPECT_OK(baz->someBaseMethod());
}

TEST_F(HidlTest, BazSomeOtherBaseMethodTest) {
    IBase::Foo foo;
    foo.x = 1;
    foo.y.z = 2.5;
    // A valid UTF-8 string
    foo.y.s = "Hello, world, \x46\x6F\x6F\x20\xC2\xA9\x20\x62\x61\x72\x20\xF0\x9D\x8C\x86\x20\x54\x72\x65\x62\x6C\x65\x20\xE2\x98\x83\x20\x72\x6F\x63\x6B\x73";

    foo.aaa.resize(5);
    for (size_t i = 0; i < foo.aaa.size(); ++i) {
        foo.aaa[i].z = 1.0f + (float)i * 0.01f;
        foo.aaa[i].s = ("Hello, world " + std::to_string(i)).c_str();
    }

    EXPECT_OK(
            baz->someOtherBaseMethod(
                foo,
                [&](const auto &result) {
                    // Strings should have the same size as they did before
                    // marshaling. b/35038064
                    EXPECT_EQ(result.y.s.size(), foo.y.s.size());
                    EXPECT_EQ(foo, result);
               }));
}

TEST_F(HidlTest, BazSomeMethodWithFooArraysTest) {
    hidl_array<IBase::Foo, 2> foo;

    foo[0].x = 1;
    foo[0].y.z = 2.5;
    foo[0].y.s = "Hello, world";

    foo[0].aaa.resize(5);
    for (size_t i = 0; i < foo[0].aaa.size(); ++i) {
        foo[0].aaa[i].z = 1.0f + (float)i * 0.01f;
        foo[0].aaa[i].s = ("Hello, world " + std::to_string(i)).c_str();
    }

    foo[1].x = 2;
    foo[1].y.z = -2.5;
    foo[1].y.s = "Morituri te salutant";

    foo[1].aaa.resize(3);
    for (size_t i = 0; i < foo[1].aaa.size(); ++i) {
        foo[1].aaa[i].z = 2.0f - (float)i * 0.01f;
        foo[1].aaa[i].s = ("Alea iacta est: " + std::to_string(i)).c_str();
    }

    hidl_array<IBaz::Foo, 2> fooExpectedOutput;
    fooExpectedOutput[0] = foo[1];
    fooExpectedOutput[1] = foo[0];

    EXPECT_OK(
            baz->someMethodWithFooArrays(
                foo,
                [&](const auto &result) {
                    EXPECT_EQ(result, fooExpectedOutput);
               }));
}

TEST_F(HidlTest, BazSomeMethodWithFooVectorsTest) {
    hidl_vec<IBase::Foo> foo;
    foo.resize(2);

    foo[0].x = 1;
    foo[0].y.z = 2.5;
    foo[0].y.s = "Hello, world";

    foo[0].aaa.resize(5);
    for (size_t i = 0; i < foo[0].aaa.size(); ++i) {
        foo[0].aaa[i].z = 1.0f + (float)i * 0.01f;
        foo[0].aaa[i].s = ("Hello, world " + std::to_string(i)).c_str();
    }

    foo[1].x = 2;
    foo[1].y.z = -2.5;
    foo[1].y.s = "Morituri te salutant";

    foo[1].aaa.resize(3);
    for (size_t i = 0; i < foo[1].aaa.size(); ++i) {
        foo[1].aaa[i].z = 2.0f - (float)i * 0.01f;
        foo[1].aaa[i].s = ("Alea iacta est: " + std::to_string(i)).c_str();
    }

    hidl_vec<IBaz::Foo> fooExpectedOutput;
    fooExpectedOutput.resize(2);
    fooExpectedOutput[0] = foo[1];
    fooExpectedOutput[1] = foo[0];

    EXPECT_OK(
            baz->someMethodWithFooVectors(
                foo,
                [&](const auto &result) {
                    EXPECT_EQ(result, fooExpectedOutput);
                }));
}

TEST_F(HidlTest, BazSomeMethodWithVectorOfArray) {
    IBase::VectorOfArray in, expectedOut;
    in.addresses.resize(3);
    expectedOut.addresses.resize(3);

    size_t k = 0;
    const size_t n = in.addresses.size();

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < 6; ++j, ++k) {
            in.addresses[i][j] = k;
            expectedOut.addresses[n - 1 - i][j] = k;
        }
    }

    EXPECT_OK(
            baz->someMethodWithVectorOfArray(
                in,
                [&](const auto &out) {
                    EXPECT_EQ(expectedOut, out);
                }));
}

TEST_F(HidlTest, BazSomeMethodTakingAVectorOfArray) {
    hidl_vec<hidl_array<uint8_t, 6> > in, expectedOut;
    in.resize(3);
    expectedOut.resize(3);

    size_t k = 0;
    const size_t n = in.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < 6; ++j, ++k) {
            in[i][j] = k;
            expectedOut[n - 1 - i][j] = k;
        }
    }

    EXPECT_OK(
            baz->someMethodTakingAVectorOfArray(
                in,
                [&](const auto &out) {
                    EXPECT_EQ(expectedOut, out);
                }));
}

static std::string numberToEnglish(int x) {
    static const char *const kDigits[] = {
        "zero",
        "one",
        "two",
        "three",
        "four",
        "five",
        "six",
        "seven",
        "eight",
        "nine",
    };

    if (x < 0) {
        return "negative " + numberToEnglish(-x);
    }

    if (x < 10) {
        return kDigits[x];
    }

    if (x <= 15) {
        static const char *const kSpecialTens[] = {
            "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen",
        };

        return kSpecialTens[x - 10];
    }

    if (x < 20) {
        return std::string(kDigits[x % 10]) + "teen";
    }

    if (x < 100) {
        static const char *const kDecades[] = {
            "twenty", "thirty", "forty", "fifty", "sixty", "seventy",
            "eighty", "ninety",
        };

        return std::string(kDecades[x / 10 - 2]) + kDigits[x % 10];
    }

    return "positively huge!";
}

TEST_F(HidlTest, BazTransposeTest) {
    IBase::StringMatrix5x3 in;
    IBase::StringMatrix3x5 expectedOut;

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 3; ++j) {
            in.s[i][j] = expectedOut.s[j][i] = numberToEnglish(3 * i + j + 1).c_str();
        }
    }

    EXPECT_OK(baz->transpose(
                in,
                [&](const auto &out) {
                    EXPECT_EQ(expectedOut, out);
                }));
}

TEST_F(HidlTest, BazTranspose2Test) {
    hidl_array<hidl_string, 5, 3> in;
    hidl_array<hidl_string, 3, 5> expectedOut;

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 3; ++j) {
            in[i][j] = expectedOut[j][i] = numberToEnglish(3 * i + j + 1).c_str();
        }
    }

    EXPECT_OK(baz->transpose2(
                in,
                [&](const auto &out) {
                    EXPECT_EQ(expectedOut, out);
                }));
}

TEST_F(HidlTest, BazSomeBoolMethodTest) {
    auto result = baz->someBoolMethod(true);
    EXPECT_OK(result);
    EXPECT_EQ(result, false);
}

TEST_F(HidlTest, BazSomeBoolArrayMethodTest) {
    hidl_array<bool, 3> someBoolArray;
    someBoolArray[0] = true;
    someBoolArray[1] = false;
    someBoolArray[2] = true;

    hidl_array<bool, 4> expectedOut;
    expectedOut[0] = false;
    expectedOut[1] = true;
    expectedOut[2] = false;
    expectedOut[3] = true;

    EXPECT_OK(
            baz->someBoolArrayMethod(
                someBoolArray,
                [&](const auto &result) {
                    EXPECT_EQ(expectedOut, result);
                }));
}

TEST_F(HidlTest, BazSomeBoolVectorMethodTest) {
    hidl_vec<bool> someBoolVector, expected;
    someBoolVector.resize(4);
    expected.resize(4);

    for (size_t i = 0; i < someBoolVector.size(); ++i) {
        someBoolVector[i] = ((i & 1) == 0);
        expected[i] = !someBoolVector[i];
    }

    EXPECT_OK(
            baz->someBoolVectorMethod(
                someBoolVector,
                [&](const auto &result) {
                    EXPECT_EQ(expected, result);
                }));
}

TEST_F(HidlTest, BazDoThisMethodTest) {
    EXPECT_OK(baz->doThis(1.0f));
}

TEST_F(HidlTest, BazDoThatAndReturnSomethingMethodTest) {
    auto result = baz->doThatAndReturnSomething(1);
    EXPECT_OK(result);
    EXPECT_EQ(result, 666);
}

TEST_F(HidlTest, BazDoQuiteABitMethodTest) {
    auto result = baz->doQuiteABit(1, 2ll, 3.0f, 4.0);

    EXPECT_OK(result);
    EXPECT_EQ(result, 666.5);
}

TEST_F(HidlTest, BazDoSomethingElseMethodTest) {
    hidl_array<int32_t, 15> param;
    hidl_array<int32_t, 32> expected;

    for (size_t i = 0; i < 15; ++i) {
        param[i] = expected[15 + i] = i;
        expected[i] = 2 * i;
    }

    expected[30] = 1;
    expected[31] = 2;

    EXPECT_OK(
            baz->doSomethingElse(
                param,
                [&](const auto &result) {
                    EXPECT_EQ(expected, result);
                }));
}

TEST_F(HidlTest, BazDoStuffAndReturnAStringMethodTest) {
    std::string expected = "Hello, world!";
    EXPECT_OK(
            baz->doStuffAndReturnAString(
                [&](const auto &result) {
                    EXPECT_EQ(expected, result);
                }));
}

TEST_F(HidlTest, BazMapThisVectorMethodTest) {
    hidl_vec<int32_t> vec_param, expected;
    vec_param.resize(15);
    expected.resize(15);

    for (size_t i = 0; i < 15; ++i) {
        vec_param[i] = i;
        expected[i] = 2 * i;
    }

    EXPECT_OK(
            baz->mapThisVector(
                vec_param,
                [&](const auto &result) {
                    EXPECT_EQ(expected, result);
                }));
}

TEST_F(HidlTest, BazCallMeMethodTest) {
    EXPECT_OK(baz->callMe(new BazCallback()));
}

TEST_F(HidlTest, BazCallMeLaterMethodTest) {
    EXPECT_OK(baz->callMeLater(new BazCallback()));
    EXPECT_OK(baz->iAmFreeNow());
}

TEST_F(HidlTest, BazUseAnEnumMethodTest) {
    auto result = baz->useAnEnum(IBaz::SomeEnum::bar);

    EXPECT_OK(result);
    EXPECT_TRUE(result == IBaz::SomeEnum::quux);
}

TEST_F(HidlTest, BazHaveSomeStringsMethodTest) {
    hidl_array<hidl_string, 3> string_params;
    string_params[0] = "one";
    string_params[1] = "two";
    string_params[2] = "three";

    hidl_array<hidl_string, 2> expected;
    expected[0] = "Hello";
    expected[1] = "World";

    EXPECT_OK(
            baz->haveSomeStrings(
                string_params,
                [&](const auto &result) {
                    EXPECT_EQ(expected, result);
                }));
}

TEST_F(HidlTest, BazHaveAStringVecMethodTest) {
    hidl_vec<hidl_string> string_vec{ "Uno", "Dos", "Tres", "Cuatro" };
    hidl_vec<hidl_string> expected{"Hello", "World"};

    EXPECT_OK(
            baz->haveAStringVec(
                string_vec,
                [&](const auto &result) {
                    EXPECT_EQ(expected, result);
                }));
}

TEST_F(HidlTest, BazReturnABunchOfStringsMethodTest) {
    std::string expectedA = "Eins";
    std::string expectedB = "Zwei";
    std::string expectedC = "Drei";
    EXPECT_OK(
            baz->returnABunchOfStrings(
                [&](const auto &a, const auto &b, const auto &c) {
                    EXPECT_EQ(a, expectedA);
                    EXPECT_EQ(b, expectedB);
                    EXPECT_EQ(c, expectedC);
                }));
}

TEST_F(HidlTest, BazTestArrays) {
    IBase::LotsOfPrimitiveArrays in;

    for (size_t i = 0; i < 128; ++i) {
        in.byte1[i] = i;
        in.boolean1[i] = (i & 4) != 0;
        in.double1[i] = i;
    }

    size_t k = 0;
    for (size_t i = 0; i < 8; ++i) {
        for (size_t j = 0; j < 128; ++j, ++k) {
            in.byte2[i][j] = k;
            in.boolean2[i][j] = (k & 4) != 0;
            in.double2[i][j] = k;
        }
    }

    size_t m = 0;
    for (size_t i = 0; i < 8; ++i) {
        for (size_t j = 0; j < 16; ++j) {
            for (size_t k = 0; k < 128; ++k, ++m) {
                in.byte3[i][j][k] = m;
                in.boolean3[i][j][k] = (m & 4) != 0;
                in.double3[i][j][k] = m;
            }
        }
    }

    EXPECT_OK(
            baz->testArrays(in,
                [&](const auto &out) {
                    EXPECT_EQ(in, out);
                }));
}

TEST_F(HidlTest, BazTestByteVecs) {
    hidl_vec<IBase::ByteOneDim> in;
    in.resize(8);

    size_t k = 0;
    for (size_t i = 0; i < in.size(); ++i) {
        for (size_t j = 0; j < 128; ++j, ++k) {
            in[i][j] = k;
        }
    }

    EXPECT_OK(baz->testByteVecs(
                in, [&](const auto &out) { EXPECT_EQ(in, out); }));
}

TEST_F(HidlTest, BazTestBooleanVecs) {
    hidl_vec<IBase::BooleanOneDim> in;
    in.resize(8);

    size_t k = 0;
    for (size_t i = 0; i < in.size(); ++i) {
        for (size_t j = 0; j < 128; ++j, ++k) {
            in[i][j] = (k & 4) != 0;
        }
    }

    EXPECT_OK(baz->testBooleanVecs(
                in, [&](const auto &out) { EXPECT_EQ(in, out); }));
}

TEST_F(HidlTest, BazTestDoubleVecs) {
    hidl_vec<IBase::DoubleOneDim> in;
    in.resize(8);

    size_t k = 0;
    for (size_t i = 0; i < in.size(); ++i) {
        for (size_t j = 0; j < 128; ++j, ++k) {
            in[i][j] = k;
        }
    }

    EXPECT_OK(baz->testDoubleVecs(
                in, [&](const auto &out) { EXPECT_EQ(in, out); }));
}

int main(int argc, char **argv) {
    setenv("TREBLE_TESTING_OVERRIDE", "true", true);

    using namespace android::hardware;

    const char *me = argv[0];

    bool wantClient = false;
    bool wantServer = false;

    int res;
    while ((res = getopt(argc, argv, "chs")) >= 0) {
        switch (res) {
            case 'c':
            {
                wantClient = true;
                break;
            }

            case 's':
            {
                wantServer = true;
                break;
            }

            case '?':
            case 'h':
            default:
            {
                usage(me);
                exit(1);
                break;
            }
        }
    }

    if ((!wantClient && !wantServer) || (wantClient && wantServer)) {
        usage(me);
        exit(1);
    }

    if (wantClient) {
        ::testing::AddGlobalTestEnvironment(new HidlEnvironment);
        ::testing::InitGoogleTest(&argc, argv);
        int status = RUN_ALL_TESTS();
        return status;
    }

    return defaultPassthroughServiceImplementation<IBaz>("baz");

}
