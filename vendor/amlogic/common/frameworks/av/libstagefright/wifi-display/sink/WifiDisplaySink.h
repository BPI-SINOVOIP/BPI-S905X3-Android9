/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WIFI_DISPLAY_SINK_H_

#define WIFI_DISPLAY_SINK_H_

#include "AmANetworkSession.h"

#include <gui/Surface.h>
#include <gui/IGraphicBufferProducer.h>
#include <media/stagefright/foundation/AHandler.h>

#if ANDROID_PLATFORM_SDK_VERSION <= 27
#include <media/IHDCP.h>
#else
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCPService.h>
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCP.h>
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCPObserver.h>
using namespace vendor::amlogic::hardware::miracast_hdcp2::V1_0;
#endif
namespace android
{
#if ANDROID_PLATFORM_SDK_VERSION <= 27
    struct HDCPObserver;
#else
    using vendor::amlogic::hardware::miracast_hdcp2::V1_0::IHDCP;
    using vendor::amlogic::hardware::miracast_hdcp2::V1_0::Status;
    using ::android::hardware::Void;
    using ::android::hardware::Return;
    struct HDCPObserver;
#endif
    //struct IHDCP;
    struct ParsedMessage;
    struct RTPSink;

    // Represents the RTSP client acting as a wifi display sink.
    // Connects to a wifi display source and renders the incoming
    // transport stream using a MediaPlayer instance.
    struct WifiDisplaySink : public AHandler
    {
        WifiDisplaySink(
            const sp<AmANetworkSession> &netSession,
            const sp<IGraphicBufferProducer> &bufferProducer = NULL);
        enum
        {
            High,
            Normal
        };
#if ANDROID_PLATFORM_SDK_VERSION <= 27
        struct HDCPObserver : public BnHDCPObserver
        {
            HDCPObserver(const sp<AMessage> &notify);
            virtual void notify(
                int msg, int ext1, int ext2, const Parcel *obj);

            private:
                sp<AMessage> mNotify;
            DISALLOW_EVIL_CONSTRUCTORS(HDCPObserver);
        };
#else
        struct HDCPObserver : public IHDCPObserver
        {
            HDCPObserver(const sp<AMessage> &notify);
            virtual Return<void> notify(
                int msg, int ext1, int ext2) override;

        private:
            sp<AMessage> mNotify;
            DISALLOW_EVIL_CONSTRUCTORS(HDCPObserver);
        };
#endif
        void start(const char *sourceHost, int32_t sourcePort);
        void start(const char *uri);
        void retryStart(int32_t uDelay);
        void stop(void);
        void setPlay(void);
        void setPause(void);
        void setTeardown(void);
        void setSinkHandler(const sp<AHandler> &handler);
        void setResolution(int resolution);
        int getResolution();
    protected:
        virtual ~WifiDisplaySink();
        virtual void onMessageReceived(const sp<AMessage> &msg);

    private:
        //struct HDCPObserver;

        enum State
        {
            UNDEFINED,
            CONNECTING,
            CONNECTED,
            PAUSED,
            PLAYING,
        };

        enum {
            kWhatNoPacketMsg,
            kWahtLostPacketMsg,
            kWhatConnectedNoPacketCheck,
        };

        enum
        {
            kWhatStart,
            kWhatRTSPNotify,
            kWhatStop,
            kWhatHDCPNotify,
            kWhatNoPacket,
        };

        enum
        {
            kWhatSinkNotify,
            kWhatStopCompleted,
        };

        struct ResponseID
        {
            int32_t mSessionID;
            int32_t mCSeq;

            bool operator<(const ResponseID &other) const
            {
                return mSessionID < other.mSessionID
                       || (mSessionID == other.mSessionID
                           && mCSeq < other.mCSeq);
            }
        };

        typedef status_t (WifiDisplaySink::*HandleRTSPResponseFunc)(
            int32_t sessionID, const sp<ParsedMessage> &msg);

        static const bool sUseTCPInterleaving = false;

        State mState;
        sp<AmANetworkSession> mNetSession;
        sp<IGraphicBufferProducer> mBufferProducer;
        AString mSetupURI;
        AString mRTSPHost;
        int32_t mSessionID;

        int32_t mNextCSeq;

        KeyedVector<ResponseID, HandleRTSPResponseFunc> mResponseHandlers;

        sp<RTPSink> mRTPSink;
        AString mPlaybackSessionID;
        int32_t mPlaybackSessionTimeoutSecs;
        sp<AHandler> mSinkHandler;
        sp<AMessage> mMsgNotify;
        int32_t mRTSPPort;
        int32_t mConnectionRetry;
        #define MAX_CONN_RETRY 5

        /*add by yalong.liu*/
        bool mNeedAudioCodecs;
        bool mNeedVideoFormats;
        bool mNeed3dVideoFormats;
        bool mNeedDisplayEdid;
        bool mNeedCoupledSink;
        bool mNeedclientRtpPorts;
        bool mNeedStandbyResumeCapability;
        bool mNeedUibcCapability;
        bool mNeedConnectorType;
        bool mNeedI2C;

        // HDCP specific section >>>>
        bool mUsingHDCP;
        bool mNeedHDCP;
        int32_t mHDCPPort;
        int mResolution;
        AString mCEA;
        AString mVESA;
        AString mHH;
        sp<IHDCP> mHDCP;
        sp<HDCPObserver> mHDCPObserver;
#if ANDROID_PLATFORM_SDK_VERSION <= 27
        status_t makeHDCP();
#else
        Status makeHDCP();
#endif
        Mutex mHDCPLock;
        bool mHDCPRunning;
        void prepareHDCP();
        // <<<< HDCP specific section

        status_t sendM2(int32_t sessionID);
        status_t sendDescribe(int32_t sessionID, const char *uri);
        status_t sendSetup(int32_t sessionID, const char *uri);
        status_t sendPlay(int32_t sessionID, const char *uri);
        status_t sendPause(int32_t sessionID, const char *uri);
        status_t sendTeardown(int32_t sessionID, const char *uri);

        status_t onReceiveM2Response(
            int32_t sessionID, const sp<ParsedMessage> &msg);

        status_t onReceiveDescribeResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg);

        status_t onReceiveSetupResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg);

        status_t configureTransport(const sp<ParsedMessage> &msg);

        status_t onReceivePlayResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg);

        status_t onReceivePauseResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg);

        status_t onReceiveTeardownResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg);

        void registerResponseHandler(
            int32_t sessionID, int32_t cseq, HandleRTSPResponseFunc func);

        void onReceiveClientData(const sp<AMessage> &msg);

        void onOptionsRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

        void onGetParameterRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

        void onSetParameterRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

        void sendErrorResponse(
            int32_t sessionID,
            const char *errorDetail,
            int32_t cseq);

        static void AppendCommonResponse(AString *response, int32_t cseq);

        bool ParseURL(
            const char *url, AString *host, int32_t *port, AString *path,
            AString *user, AString *pass);

        int save_sessionid_to_file(char* filepath, int32_t sessionID);
        bool mIDRFrameRequestPending;
        status_t sendIDRFrameRequest(int32_t sessionID);
        status_t onReceiveIDRFrameRequestResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg);
        bool mSourceStandby;
        DISALLOW_EVIL_CONSTRUCTORS(WifiDisplaySink);
    };

}  // namespace android

#endif  // WIFI_DISPLAY_SINK_H_
