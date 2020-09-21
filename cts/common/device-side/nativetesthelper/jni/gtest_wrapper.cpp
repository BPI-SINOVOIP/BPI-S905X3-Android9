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

#include <jni.h>
#include <nativehelper/ScopedLocalRef.h>
#include <gtest/gtest.h>

static struct {
    jclass clazz;

    /** static methods **/
    jmethodID createTestDescription;

    /** methods **/
    jmethodID addChild;
} gDescription;

static struct {
    jclass clazz;

    jmethodID fireTestStarted;
    jmethodID fireTestIgnored;
    jmethodID fireTestFailure;
    jmethodID fireTestFinished;

} gRunNotifier;

static struct {
    jclass clazz;
    jmethodID ctor;
} gAssertionFailure;

static struct {
    jclass clazz;
    jmethodID ctor;
} gFailure;

jobject gEmptyAnnotationsArray;

static jobject createTestDescription(JNIEnv* env, const char* className, const char* testName) {
    ScopedLocalRef<jstring> jClassName(env, env->NewStringUTF(className));
    ScopedLocalRef<jstring> jTestName(env, env->NewStringUTF(testName));
    return env->CallStaticObjectMethod(gDescription.clazz, gDescription.createTestDescription,
            jClassName.get(), jTestName.get(), gEmptyAnnotationsArray);
}

static void addChild(JNIEnv* env, jobject description, jobject childDescription) {
    env->CallVoidMethod(description, gDescription.addChild, childDescription);
}


class JUnitNotifyingListener : public ::testing::EmptyTestEventListener {
public:

    JUnitNotifyingListener(JNIEnv* env, jobject runNotifier)
            : mEnv(env)
            , mRunNotifier(runNotifier)
            , mCurrentTestDescription{env, nullptr}
    {}
    virtual ~JUnitNotifyingListener() {}

    virtual void OnTestStart(const testing::TestInfo &testInfo) override {
        mCurrentTestDescription.reset(
                createTestDescription(mEnv, testInfo.test_case_name(), testInfo.name()));
        notify(gRunNotifier.fireTestStarted);
    }

    virtual void OnTestPartResult(const testing::TestPartResult &testPartResult) override {
        if (!testPartResult.passed()) {
            char message[1024];
            snprintf(message, 1024, "%s:%d\n%s", testPartResult.file_name(), testPartResult.line_number(),
                    testPartResult.message());
            ScopedLocalRef<jstring> jmessage(mEnv, mEnv->NewStringUTF(message));
            ScopedLocalRef<jobject> jthrowable(mEnv, mEnv->NewObject(gAssertionFailure.clazz,
                    gAssertionFailure.ctor, jmessage.get()));
            ScopedLocalRef<jobject> jfailure(mEnv, mEnv->NewObject(gFailure.clazz,
                    gFailure.ctor, mCurrentTestDescription.get(), jthrowable.get()));
            mEnv->CallVoidMethod(mRunNotifier, gRunNotifier.fireTestFailure, jfailure.get());
        }
    }

    virtual void OnTestEnd(const testing::TestInfo&) override {
        notify(gRunNotifier.fireTestFinished);
        mCurrentTestDescription.reset();
    }

    virtual void OnTestProgramEnd(const testing::UnitTest& unitTest) override {
        // Invoke the notifiers for all the disabled tests
        for (int testCaseIndex = 0; testCaseIndex < unitTest.total_test_case_count(); testCaseIndex++) {
            auto testCase = unitTest.GetTestCase(testCaseIndex);
            for (int testIndex = 0; testIndex < testCase->total_test_count(); testIndex++) {
                auto testInfo = testCase->GetTestInfo(testIndex);
                if (!testInfo->should_run()) {
                    mCurrentTestDescription.reset(
                            createTestDescription(mEnv, testCase->name(), testInfo->name()));
                    notify(gRunNotifier.fireTestIgnored);
                    mCurrentTestDescription.reset();
                }
            }
        }
    }

private:
    void notify(jmethodID method) {
        mEnv->CallVoidMethod(mRunNotifier, method, mCurrentTestDescription.get());
    }

    JNIEnv* mEnv;
    jobject mRunNotifier;
    ScopedLocalRef<jobject> mCurrentTestDescription;
};

extern "C"
JNIEXPORT void JNICALL
Java_com_android_gtestrunner_GtestRunner_nInitialize(JNIEnv *env, jclass, jobject suite) {
    // Initialize gtest, removing the default result printer
    int argc = 1;
    const char* argv[] = { "gtest_wrapper" };
    ::testing::InitGoogleTest(&argc, (char**) argv);

    auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());

    gDescription.clazz = (jclass) env->NewGlobalRef(env->FindClass("org/junit/runner/Description"));
    gDescription.createTestDescription = env->GetStaticMethodID(gDescription.clazz, "createTestDescription",
            "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/annotation/Annotation;)Lorg/junit/runner/Description;");
    gDescription.addChild = env->GetMethodID(gDescription.clazz, "addChild",
            "(Lorg/junit/runner/Description;)V");

    jclass annotations = env->FindClass("java/lang/annotation/Annotation");
    gEmptyAnnotationsArray = env->NewGlobalRef(env->NewObjectArray(0, annotations, nullptr));

    gAssertionFailure.clazz = (jclass) env->NewGlobalRef(env->FindClass("java/lang/AssertionError"));
    gAssertionFailure.ctor = env->GetMethodID(gAssertionFailure.clazz, "<init>", "(Ljava/lang/Object;)V");

    gFailure.clazz = (jclass) env->NewGlobalRef(env->FindClass("org/junit/runner/notification/Failure"));
    gFailure.ctor = env->GetMethodID(gFailure.clazz, "<init>",
            "(Lorg/junit/runner/Description;Ljava/lang/Throwable;)V");

    gRunNotifier.clazz = (jclass) env->NewGlobalRef(
            env->FindClass("org/junit/runner/notification/RunNotifier"));
    gRunNotifier.fireTestStarted = env->GetMethodID(gRunNotifier.clazz, "fireTestStarted",
            "(Lorg/junit/runner/Description;)V");
    gRunNotifier.fireTestIgnored = env->GetMethodID(gRunNotifier.clazz, "fireTestIgnored",
            "(Lorg/junit/runner/Description;)V");
    gRunNotifier.fireTestFinished = env->GetMethodID(gRunNotifier.clazz, "fireTestFinished",
            "(Lorg/junit/runner/Description;)V");
    gRunNotifier.fireTestFailure = env->GetMethodID(gRunNotifier.clazz, "fireTestFailure",
            "(Lorg/junit/runner/notification/Failure;)V");

    auto unitTest = ::testing::UnitTest::GetInstance();
    for (int testCaseIndex = 0; testCaseIndex < unitTest->total_test_case_count(); testCaseIndex++) {
        auto testCase = unitTest->GetTestCase(testCaseIndex);
        for (int testIndex = 0; testIndex < testCase->total_test_count(); testIndex++) {
            auto testInfo = testCase->GetTestInfo(testIndex);
            ScopedLocalRef<jobject> testDescription(env,
                    createTestDescription(env, testCase->name(), testInfo->name()));
            addChild(env, suite, testDescription.get());
        }
    }
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_android_gtestrunner_GtestRunner_nRun(JNIEnv *env, jclass, jobject notifier) {
    auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
    JUnitNotifyingListener junitListener{env, notifier};
    listeners.Append(&junitListener);
    int success = RUN_ALL_TESTS();
    listeners.Release(&junitListener);
    return success == 0;
}
