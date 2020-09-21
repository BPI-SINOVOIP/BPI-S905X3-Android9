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

#ifndef MOCK_CAS_PLUGIN_H_
#define MOCK_CAS_PLUGIN_H_

#include <media/cas/CasAPI.h>
#include <media/cas/DescramblerAPI.h>
#include <utils/Mutex.h>

extern "C" {
      android::CasFactory *createCasFactory();
      android::DescramblerFactory *createDescramblerFactory();
}

namespace android {

class MockCasFactory : public CasFactory {
public:
    MockCasFactory() {}
    virtual ~MockCasFactory() {}

    virtual bool isSystemIdSupported(
            int32_t CA_system_id) const override;
    virtual status_t queryPlugins(
            std::vector<CasPluginDescriptor> *descriptors) const override;
    virtual status_t createPlugin(
            int32_t CA_system_id,
            void *appData,
            CasPluginCallback callback,
            CasPlugin **plugin) override;
};

class MockDescramblerFactory : public DescramblerFactory {
public:
    MockDescramblerFactory() {}
    virtual ~MockDescramblerFactory() {}

    virtual bool isSystemIdSupported(
            int32_t CA_system_id) const override;
    virtual status_t createPlugin(
            int32_t CA_system_id, DescramblerPlugin **plugin) override;
};

class MockCasPlugin : public CasPlugin {
public:
    MockCasPlugin();
    virtual ~MockCasPlugin();

    virtual status_t setPrivateData(
            const CasData &data) override;

    virtual status_t openSession(CasSessionId *sessionId) override;

    virtual status_t closeSession(
            const CasSessionId &sessionId) override;

    virtual status_t setSessionPrivateData(
            const CasSessionId &sessionId,
            const CasData &data) override;

    virtual status_t processEcm(
            const CasSessionId &sessionId, const CasEcm &ecm) override;

    virtual status_t processEmm(const CasEmm &emm) override;

    virtual status_t sendEvent(
            int32_t event, int32_t arg, const CasData &eventData) override;

    virtual status_t provision(const String8 &str) override;

    virtual status_t refreshEntitlements(
            int32_t refreshType, const CasData &refreshData) override;

private:

    Mutex mLock;
};

class MockDescramblerPlugin : public DescramblerPlugin {
public:
    MockDescramblerPlugin() {}
    virtual ~MockDescramblerPlugin() {};

    virtual bool requiresSecureDecoderComponent(
            const char *mime) const override;

    virtual status_t setMediaCasSession(
            const CasSessionId &sessionId) override;

    virtual ssize_t descramble(
            bool secure,
            ScramblingControl scramblingControl,
            size_t numSubSamples,
            const SubSample *subSamples,
            const void *srcPtr,
            int32_t srcOffset,
            void *dstPtr,
            int32_t dstOffset,
            AString *errorDetailMsg) override;

private:
    String8 subSamplesToString(
            SubSample const *subSamples,
            size_t numSubSamples) const;
    String8 arrayToString(uint8_t const *array, size_t len) const;
};
} // namespace android

#endif // MOCK_CAS_PLUGIN_H_
