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

#define LOG_NDEBUG 0
#define LOG_TAG "WifiDisplaySink"
#include <utils/Log.h>

#include "WifiDisplaySink.h"
#include "media/stagefright/foundation/ParsedMessage.h"
#include "RTPSink.h"

#include <android/hidl/base/1.0/IBase.h>
#include <binder/IServiceManager.h>
#include <media/IMediaPlayerService.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaErrors.h>
#include <cutils/properties.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

#if ANDROID_PLATFORM_SDK_VERSION <= 27
#include <media/IMediaPlayerService.h>
#else
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCPService.h>
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCP.h>
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCPObserver.h>
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/types.h>
#include <media/hardware/HDCPAPI.h>
#include <android/hidl/manager/1.0/IServiceNotification.h>
#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#endif

#define CEA_HIGH_RESOLUTION         "0001DEFF"
#define CEA_NORMAL_RESOLUTION       "00008C7F"
#define VESA_HIGH_RESOLUTION        "157C7FFF"
#define VESA_NORMAL_RESOLUTION      "00007FFF"
#define HH_HIGH_RESOLUTION          "00000FFF"
#define HH_NORMAL_RESOLUTION        "00000FFF"

namespace android
{
    WifiDisplaySink::WifiDisplaySink(
        const sp<AmANetworkSession> &netSession,
        const sp<IGraphicBufferProducer> &bufferProducer)
        : mState(UNDEFINED),
          mNetSession(netSession),
          mBufferProducer(bufferProducer),
          mSessionID(0),
          mNextCSeq(1),
          mConnectionRetry(0),
          mUsingHDCP(false),
          mNeedHDCP(false),
          mResolution(Normal),
          mHDCPRunning(false),
          mSourceStandby(false),
          mIDRFrameRequestPending(false)
    {
        srand(time(0));
        mHDCPPort = 9000 + rand() % 100;
        setResolution(High);
        property_set("sys.wfd.state","0"); //0-init play,1-normal->pause,2-pause->play
    }

    WifiDisplaySink::~WifiDisplaySink()
    {
    }

    void WifiDisplaySink::setSinkHandler(const sp<AHandler> &handler)
    {
        mSinkHandler = handler;
    }

    void WifiDisplaySink::start(const char *sourceHost, int32_t sourcePort)
    {
        prepareHDCP();
        mConnectionRetry = 0;
        sp<AMessage> msg = new AMessage(kWhatStart, this);
        msg->setString("sourceHost", sourceHost);
        msg->setInt32("sourcePort", sourcePort);
        ALOGI("post msg kWhatStart.");
        msg->post();
    }

    void WifiDisplaySink::start(const char *uri)
    {
        prepareHDCP();
        mConnectionRetry = 0;
        sp<AMessage> msg = new AMessage(kWhatStart, this);
        msg->setString("setupURI", uri);
        msg->post();
    }

    void WifiDisplaySink::retryStart(int32_t uDelay)
    {
        sp<AMessage> msg = new AMessage(kWhatStart, this);
        msg->setString("sourceHost", mRTSPHost.c_str());
        msg->setInt32("sourcePort", mRTSPPort);
        ALOGI("post msg kWhatStart.");
        msg->post(uDelay);

    }

    void WifiDisplaySink::stop(void)
    {
        sp<AMessage> msg = new AMessage(kWhatStop, this);
        msg->post();
    }

    void WifiDisplaySink::setPlay(void)
    {
        AString url = AStringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str());
        sendPlay(mSessionID, !mSetupURI.empty()? mSetupURI.c_str() : url.c_str());
    }

    void WifiDisplaySink::setPause(void)
    {
        AString url = AStringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str());
        sendPause(mSessionID, !mSetupURI.empty()? mSetupURI.c_str() : url.c_str());
    }

    void WifiDisplaySink::setTeardown(void)
    {
        if (mState < CONNECTED) {
            ALOGI("No need call setTeardown in state is %d", mState);
            return;
        }
        AString url = AStringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str());
        sendTeardown(mSessionID, !mSetupURI.empty()? mSetupURI.c_str() : url.c_str());
    }
    // static
    bool WifiDisplaySink::ParseURL(
        const char *url, AString *host, int32_t *port, AString *path,
        AString *user, AString *pass)
    {
        host->clear();
        *port = 0;
        path->clear();
        user->clear();
        pass->clear();

        if (strncasecmp("rtsp://", url, 7))
        {
            return false;
        }

        const char *slashPos = strchr(&url[7], '/');

        if (slashPos == NULL)
        {
            host->setTo(&url[7]);
            path->setTo("/");
        }
        else
        {
            host->setTo(&url[7], slashPos - &url[7]);
            path->setTo(slashPos);
        }

        ssize_t atPos = host->find("@");

        if (atPos >= 0)
        {
            // Split of user:pass@ from hostname.

            AString userPass(*host, 0, atPos);
            host->erase(0, atPos + 1);

            ssize_t colonPos = userPass.find(":");

            if (colonPos < 0)
            {
                *user = userPass;
            }
            else
            {
                user->setTo(userPass, 0, colonPos);
                pass->setTo(userPass, colonPos + 1, userPass.size() - colonPos - 1);
            }
        }

        const char *colonPos = strchr(host->c_str(), ':');

        if (colonPos != NULL)
        {
            char *end;
            unsigned long x = strtoul(colonPos + 1, &end, 10);

            if (end == colonPos + 1 || *end != '\0' || x >= 65536)
            {
                return false;
            }

            *port = x;

            size_t colonOffset = colonPos - host->c_str();
            size_t trailing = host->size() - colonOffset;
            host->erase(colonOffset, trailing);
        }
        else
        {
            *port = 554;
        }

        return true;
    }

    void WifiDisplaySink::setResolution(int resolution)
    {
        if (resolution == High)
        {
            mResolution = High;
            mCEA = CEA_HIGH_RESOLUTION;
            mVESA = VESA_HIGH_RESOLUTION;
            mHH = HH_HIGH_RESOLUTION;
        }
        else if(resolution == Normal)
        {
            mResolution = Normal;
            mCEA = CEA_NORMAL_RESOLUTION;
            mVESA = VESA_NORMAL_RESOLUTION;
            mHH = HH_NORMAL_RESOLUTION;
        }
        else
        {
            ALOGE("unsupported resolution.");
        }
    }
    int WifiDisplaySink::getResolution()
    {
        return mResolution;
    }

    void WifiDisplaySink::onMessageReceived(const sp<AMessage> &msg)
    {
        switch (msg->what())
        {
        case kWhatStart:
        {
            ALOGI("Received msg kWhatStart.");
            mMsgNotify = new AMessage(kWhatNoPacket, this);
            if (msg->findString("setupURI", &mSetupURI))
            {
                AString path, user, pass;
                CHECK(ParseURL(
                          mSetupURI.c_str(),
                          &mRTSPHost, &mRTSPPort, &path, &user, &pass)
                      && user.empty() && pass.empty());
            }
            else
            {
                CHECK(msg->findString("sourceHost", &mRTSPHost));
                CHECK(msg->findInt32("sourcePort", &mRTSPPort));
            }

            sp<AMessage> notify = new AMessage(kWhatRTSPNotify, this);

            ALOGI("Create RTSPClient.");
            status_t err = mNetSession->createRTSPClient(
                               mRTSPHost.c_str(), mRTSPPort, notify, &mSessionID);
            if (err)    //network not ready
            {
                sp<AMessage> msg = new AMessage(kWhatSinkNotify, mSinkHandler);
                ALOGI("post msg kWhatSinkNotify - RTSP_ERROR x1");
                msg->setString("reason", "RTSP_ERROR x1");
                msg->post();
                break;
            }
            CHECK_EQ(err, (status_t)OK);

            mState = CONNECTING;
            break;
        }

        case kWhatRTSPNotify:
        {
            int32_t reason;
            CHECK(msg->findInt32("reason", &reason));

            ALOGI("Received msg kWhatRTSPNotify.");

            switch (reason)
            {
            case AmANetworkSession::kWhatError:
            {
                int32_t sessionID;
                CHECK(msg->findInt32("sessionID", &sessionID));
                ALOGI("reason: kWhatError.");

                int32_t err;
                CHECK(msg->findInt32("err", &err));

                AString detail;
                CHECK(msg->findString("detail", &detail));

                ALOGE("An error occurred in session %d (%d, '%s/%s').",
                      sessionID,
                      err,
                      detail.c_str(),
                      strerror(-err));

                if (sessionID == mSessionID)
                {
                    ALOGI("Lost control connection.");
                    // The control connection is dead now.
                    if (err == -111)    //"Connection refused"
                    {
                        if (mConnectionRetry++ < MAX_CONN_RETRY)
                        {
                            mNetSession->destroySession(mSessionID);
                            mSessionID = 0;
                            ALOGI("Retry rtsp connection %d", mConnectionRetry);
                            retryStart(200000ll);
                        }
                        else
                        {
                            sp<AMessage> msg = new AMessage(kWhatSinkNotify, mSinkHandler);
                            ALOGI("Post msg kWhatSinkNotify - RTSP_ERROR x2");
                            msg->setString("reason", "RTSP_ERROR x2");
                            msg->post();
                        }
                    }
                    else {
                        sp<AMessage> msg = new AMessage(kWhatSinkNotify, mSinkHandler);
                        ALOGI("post msg kWhatSinkNotify - connection reset by peer");
                        msg->setString("reason", "RTSP_RESET");
                        msg->post();
                    }
                    //looper()->stop();
                }
                break;
            }

            case AmANetworkSession::kWhatConnected:
            {
                ALOGI("reason: kWhatConnected.");
                ALOGI("We're now connected.");
                mState = CONNECTED;

                if (!mSetupURI.empty())
                {
                    ALOGI("sendDescribe.[uri]");

                    status_t err =
                        sendDescribe(mSessionID, mSetupURI.c_str());

                    CHECK_EQ(err, (status_t)OK);
                }
                //After connect we send a message for 2s to cehck have rtsp packet from source or not
                //If not this will report a error
                sp<AMessage> msg = new AMessage(kWhatNoPacket, this);
                msg->setInt32("msg", kWhatConnectedNoPacketCheck);
                msg->post(2000000);
                break;
            }

            case AmANetworkSession::kWhatData:
            {
                ALOGI("reason: kWhatData.");
                onReceiveClientData(msg);
                break;
            }

            case AmANetworkSession::kWhatBinaryData:
            {
                CHECK(sUseTCPInterleaving);

                ALOGI("reason: kWhatBinaryData.");

                int32_t channel;
                CHECK(msg->findInt32("channel", &channel));

                sp<ABuffer> data;
                CHECK(msg->findBuffer("data", &data));

                mRTPSink->injectPacket(channel == 0 /* isRTP */, data);
                break;
            }

            default:
                TRESPASS();
            }
            break;
        }

        case kWhatHDCPNotify:
        {
            int32_t msgCode, ext1, ext2;
            CHECK(msg->findInt32("msg", &msgCode));
            CHECK(msg->findInt32("ext1", &ext1));
            CHECK(msg->findInt32("ext2", &ext2));

            ALOGI("Saw HDCP notification code %d, ext1 %d, ext2 %d",
                  msgCode, ext1, ext2);

            switch (msgCode)
            {
            case HDCPModule::HDCP_INITIALIZATION_COMPLETE:
            {
                mHDCPRunning = true;
                mRTPSink->setIsHDCP(true);
                break;
            }
            case HDCPModule::HDCP_SHUTDOWN_COMPLETE:
            case HDCPModule::HDCP_SHUTDOWN_FAILED:
            {
                mHDCP->setObserver(NULL);
                mHDCPObserver.clear();
                mHDCP.clear();
                mHDCPRunning = false;
                mHDCPLock.unlock();
                sp<AMessage> sinkMsg = new AMessage(kWhatStopCompleted, mSinkHandler);
                ALOGI("post msg kWhatStopCompleted for hdcp mode");
                sinkMsg->post();
                break;
            }
            case HDCPModule::HDCP_SESSION_ESTABLISHED:
                break;
            default:
            {
                ALOGE("HDCP failure, shutting down.");
                // TODO: Is this error handling correct?
                // If any error occured, we should send M8 to terminate RTSP.
                sp<AMessage> msg = new AMessage(kWhatStop, this);
                msg->post();
                break;
            }
            }
            break;
        }

        case kWhatStop:
        {
            ALOGI("Received msg stop.");
            if (mSessionID != 0)
            {
                mNetSession->destroySession(mSessionID);
                mSessionID = 0;
            }
            if (mRTPSink != NULL)
            {
                looper()->unregisterHandler(mRTPSink->id());
                mRTPSink.clear();
            }
            if (mHDCP != NULL /*&& mHDCPRunning == true*/) {
                mHDCPLock.lock();
                mHDCPRunning = true;
                ALOGI("kWhatStop: Initiating HDCP shutdown.");
                mHDCP->shutdownAsync();
            } else {
                sp<AMessage> sinkMsg = new AMessage(kWhatStopCompleted, mSinkHandler);
                ALOGI("post msg kWhatStopCompleted");
                sinkMsg->post();
            }
            //looper()->stop();
            break;
        }
        case kWhatNoPacket:
        {
            int32_t msgCode;
            CHECK(msg->findInt32("msg", &msgCode));
            switch (msgCode)
            {
                case kWhatNoPacketMsg: {
                    sp<AMessage> msg = new AMessage(kWhatSinkNotify, mSinkHandler);
                    ALOGI("post msg kWhatSinkNotify - RTP no packets");
                    msg->setString("reason", "RTP_NO_PACKET");
                    msg->post();
                    break;
                }
                case kWahtLostPacketMsg: {
                    if (!mIDRFrameRequestPending && mState == PLAYING && mSourceStandby == false) {
                        ALOGI("requesting IDR frame");
                        sendIDRFrameRequest(mSessionID);
                    }
                    int32_t isLoop = 0;
                    CHECK(msg->findInt32("isLoop", &isLoop));
                    if (isLoop) {
                        sp<AMessage> msg = new AMessage(kWhatNoPacket, this);
                        msg->setInt32("msg", kWahtLostPacketMsg);
                        msg->setInt32("isLoop", 1);
                        msg->post(5000000);
                    }
                    break;
                }
                case kWhatConnectedNoPacketCheck: {
                //After connect we send a message for 2s to cehck have rtsp packet from source or not
                //If not this will report a error we use mNextCSeq to judge if have rtsp or not
                    ALOGI("kWhatConnectedNoPacketCheck  mState is %d and mNextCSeq is %d ", mState, mNextCSeq);
                    if (mState == CONNECTED && mNextCSeq <= 1) {
                        sp<AMessage> msg = new AMessage(kWhatNoPacket, this);
                        msg->setInt32("msg", kWhatNoPacketMsg);
                        msg->post();
                    }
                }
                default:
                    break;
            }
            break;
        }
        default:
            TRESPASS();
        }
    }

    void WifiDisplaySink::registerResponseHandler(
        int32_t sessionID, int32_t cseq, HandleRTSPResponseFunc func)
    {
        ResponseID id;
        id.mSessionID = sessionID;
        id.mCSeq = cseq;

        mResponseHandlers.add(id, func);
    }

    status_t WifiDisplaySink::sendM2(int32_t sessionID)
    {
        AString request = "OPTIONS * RTSP/1.0\r\n";
        AppendCommonResponse(&request, mNextCSeq);

        request.append(
            "Require: org.wfa.wfd1.0\r\n"
            "\r\n");

        ALOGI("\nSend Request:\n%s", request.c_str());
        status_t err =
            mNetSession->sendRequest(sessionID, request.c_str(), request.size());

        if (err != OK)
        {
            return err;
        }

        ALOGI("registerResponseHandler:id = %d, onReceiveM2Response", mNextCSeq);

        registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceiveM2Response);

        ++mNextCSeq;

        return OK;
    }


    status_t WifiDisplaySink::onReceivePauseResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg) {
        int32_t statusCode;

        ALOGI("%s:%d", __FUNCTION__, sessionID);

        if (!msg->getStatusCode(&statusCode)) {
            return ERROR_MALFORMED;
        }

        if (statusCode != 200) {
            return ERROR_UNSUPPORTED;
        }

       // sp<AMessage> msg1 = new AMessage(kWhatSinkNotify, mHandlerId);
        //ALOGI("post msg kWhatSinkNotify - received msg pause Response");
      //  msg1->setString("reason", "RTSP_PAUSE");
      //  msg1->post();

        // mState = UNDEFINED;
        mState = PAUSED;

        property_set("sys.wfd.state","1");

        return OK;
    }

    status_t WifiDisplaySink::onReceiveM2Response(
        int32_t sessionID, const sp<ParsedMessage> &msg)
    {
        int32_t statusCode;

        ALOGI("%s:%d", __FUNCTION__, sessionID);

        if (!msg->getStatusCode(&statusCode))
        {
            return ERROR_MALFORMED;
        }

        if (statusCode != 200)
        {
            return ERROR_UNSUPPORTED;
        }

        return OK;
    }

    status_t WifiDisplaySink::onReceiveDescribeResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg)
    {
        int32_t statusCode;
        ALOGI("%s:%d", __FUNCTION__, sessionID);

        if (!msg->getStatusCode(&statusCode))
        {
            return ERROR_MALFORMED;
        }

        if (statusCode != 200)
        {
            return ERROR_UNSUPPORTED;
        }

        return sendSetup(sessionID, mSetupURI.c_str());
    }

    status_t WifiDisplaySink::onReceiveSetupResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg)
    {
        int32_t statusCode;

        ALOGI("%s:%d", __FUNCTION__, sessionID);

        if (!msg->getStatusCode(&statusCode))
        {
            return ERROR_MALFORMED;
        }

        if (statusCode != 200)
        {
            return ERROR_UNSUPPORTED;
        }

        if (!msg->findString("session", &mPlaybackSessionID))
        {
            return ERROR_MALFORMED;
        }

        if (!ParsedMessage::GetInt32Attribute(
                    mPlaybackSessionID.c_str(),
                    "timeout",
                    &mPlaybackSessionTimeoutSecs))
        {
            mPlaybackSessionTimeoutSecs = -1;
        }

        ssize_t colonPos = mPlaybackSessionID.find(";");
        if (colonPos >= 0)
        {
            // Strip any options from the returned session id.
            mPlaybackSessionID.erase(
                colonPos, mPlaybackSessionID.size() - colonPos);
        }

        status_t err = configureTransport(msg);

        if (err != OK)
        {
            return err;
        }


        AString url = AStringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str());

        ALOGI("%s: send PLAY Request", __FUNCTION__);

        return sendPlay(
                   sessionID,
                   !mSetupURI.empty()
                   ? mSetupURI.c_str() : url.c_str());
    }

    status_t WifiDisplaySink::configureTransport(const sp<ParsedMessage> &msg)
    {
        if (sUseTCPInterleaving)
        {
            return OK;
        }

        AString transport;
        if (!msg->findString("transport", &transport))
        {
            ALOGE("Missing 'transport' field in SETUP response.");
            return ERROR_MALFORMED;
        }

        AString sourceHost;
        if (!ParsedMessage::GetAttribute(
                    transport.c_str(), "source", &sourceHost))
        {
            sourceHost = mRTSPHost;
        }

        AString serverPortStr;
        if (!ParsedMessage::GetAttribute(
                    transport.c_str(), "server_port", &serverPortStr))
        {
            ALOGE("Missing 'server_port' in Transport field.");
            //serverPortStr.clear();
            //serverPortStr.append("16660-16661");
            //return ERROR_MALFORMED;
        }

        int rtpPort = 0, rtcpPort = 0;
        int ret;
        ret = sscanf(serverPortStr.c_str(), "%d-%d", &rtpPort, &rtcpPort);
        if (ret != 2
                || rtpPort <= 0 || rtpPort > 65535
                || rtcpPort <= 0 || rtcpPort > 65535
                || rtcpPort != rtpPort + 1)
        {
            ALOGE("!!! FIXME: Invalid server_port description '%s'.",
                  serverPortStr.c_str());

            ALOGE("ret=%d, rtpPort=%d, rtcpPort=%d",
                  ret, rtpPort, rtcpPort);

            //return ERROR_MALFORMED;
        }

        if (rtpPort & 1)
        {
            ALOGW("Server picked an odd numbered RTP port.");
        }

        //return mRTPSink->connect(sourceHost.c_str(), rtpPort, rtcpPort);
        return OK;
    }

    status_t WifiDisplaySink::onReceivePlayResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg)
    {
        int32_t statusCode;

        ALOGI("%s:%d", __FUNCTION__, sessionID);

        if (!msg->getStatusCode(&statusCode))
        {
            return ERROR_MALFORMED;
        }

        if (statusCode != 200)
        {
            return ERROR_UNSUPPORTED;
        }

        if (mState == PAUSED)
            property_set("sys.wfd.state","2");
        else
            property_set("sys.wfd.state","0");

        mState = PLAYING;
        sp<AMessage> requestIdrMsg = new AMessage(kWhatNoPacket, this);
        requestIdrMsg->setInt32("msg", kWahtLostPacketMsg);
        requestIdrMsg->setInt32("isLoop", 1);
        requestIdrMsg->post();
        return OK;
    }

    status_t WifiDisplaySink::onReceiveTeardownResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg)
    {
        int32_t statusCode;

        ALOGI("%s:%d", __FUNCTION__, sessionID);

        if (!msg->getStatusCode(&statusCode))
        {
            return ERROR_MALFORMED;
        }

        if (statusCode != 200)
        {
            return ERROR_UNSUPPORTED;
        }

        sp<AMessage> msg1 = new AMessage(kWhatSinkNotify, mSinkHandler);
        ALOGI("post msg kWhatSinkNotify - received msg teardown Response");
        msg1->setString("reason", "RTSP_TEARDOWN");
        msg1->post();

        mState = UNDEFINED;

        return OK;
    }

    void WifiDisplaySink::onReceiveClientData(const sp<AMessage> &msg)
    {
        int32_t sessionID;
        CHECK(msg->findInt32("sessionID", &sessionID));

        sp<RefBase> obj;
        CHECK(msg->findObject("data", &obj));

        sp<ParsedMessage> data =
            static_cast<ParsedMessage *>(obj.get());

        AString method;
        AString uri;
        data->getRequestField(0, &method);

        int32_t cseq;
        if (!data->findInt32("cseq", &cseq))
        {
            ALOGI("\nReceived invalid packet:\n%s",
                  data->debugString().c_str());
            sendErrorResponse(sessionID, "400 Bad Request", -1 /* cseq */);
            return;
        }

        if (method.startsWith("RTSP/"))
        {
            ALOGI("\nReceived Response:\n%s",
                  data->debugString().c_str());
            // This is a response.

            ResponseID id;
            id.mSessionID = sessionID;
            id.mCSeq = cseq;

            ssize_t index = mResponseHandlers.indexOfKey(id);

            if (index < 0)
            {
                ALOGW("Received unsolicited server response, cseq %d", cseq);
                return;
            }

            HandleRTSPResponseFunc func = mResponseHandlers.valueAt(index);
            mResponseHandlers.removeItemsAt(index);

            status_t err = (this->*func)(sessionID, data);
            CHECK_EQ(err, (status_t)OK);
        }
        else
        {
            AString version;
            AString content(data->debugString().c_str());
            ssize_t pos = -1;

            ALOGI("\nReceived Request:\n%s",
                  data->debugString().c_str());

            data->getRequestField(2, &version);
            if (!(version == AString("RTSP/1.0")))
            {
                sendErrorResponse(sessionID, "505 RTSP Version not supported", cseq);
                return;
            }

            if (method == "OPTIONS")
            {
                onOptionsRequest(sessionID, cseq, data);
            }
            else if (method == "GET_PARAMETER")
            {
                pos = content.find("wfd_content_protection", 0);
                mNeedHDCP = ((pos != -1) ? true : false);
                ALOGI("\nmNeedHDCP:%d\n",mNeedHDCP);
                /*add by yalong.liu*/
                pos = content.find("wfd_audio_codecs", 0);
                mNeedAudioCodecs = ((pos != -1) ? true : false);

                pos = content.find("wfd_video_formats", 0);
                mNeedVideoFormats = ((pos != -1) ? true : false);

                pos = content.find("wfd_3d_video_formats", 0);
                mNeed3dVideoFormats = ((pos != -1) ? true : false);

                pos = content.find("wfd_display_edid", 0);
                mNeedDisplayEdid = ((pos != -1) ? true : false);

                pos = content.find("wfd_coupled_sink", 0);
                mNeedCoupledSink = ((pos != -1) ? true : false);

                pos = content.find("wfd_client_rtp_ports", 0);
                mNeedclientRtpPorts = ((pos != -1) ? true : false);

                pos = content.find("wfd_standby_resume_capability", 0);
                mNeedStandbyResumeCapability = ((pos != -1) ? true : false);

                pos = content.find("wfd_uibc_capability", 0);
                mNeedUibcCapability = ((pos != -1) ? true : false);

                pos = content.find("wfd_connector_type", 0);
                mNeedConnectorType = ((pos != -1) ? true :false);

                pos = content.find("wfd_I2C", 0);
                mNeedI2C = ((pos != -1) ? true :false);

                onGetParameterRequest(sessionID, cseq, data);
            }
            else if (method == "SET_PARAMETER")
            {
                onSetParameterRequest(sessionID, cseq, data);
            }
            else
            {
                sendErrorResponse(sessionID, "405 Method Not Allowed", cseq);
            }
        }
    }

    void WifiDisplaySink::onOptionsRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data)
    {
        ALOGV("session %d received '%s'",
            sessionID, data->debugString().c_str());
        //save_sessionid_to_file("/data/data/com.amlogic.miracast/sessionId", sessionID);
        AString response = "RTSP/1.0 200 OK\r\n";
        AppendCommonResponse(&response, cseq);
        response.append("Public: org.wfa.wfd1.0, GET_PARAMETER, SET_PARAMETER\r\n");
        response.append("\r\n");

        ALOGI("\nSend Response:\n%s", response.c_str());
        status_t err = mNetSession->sendRequest(sessionID, response.c_str());
        CHECK_EQ(err, (status_t)OK);

        err = sendM2(sessionID);
        CHECK_EQ(err, (status_t)OK);
    }

    int WifiDisplaySink::save_sessionid_to_file(char* filepath, int32_t sessionID)
    {
        FILE *fpo = NULL;

        fpo = fopen(filepath, "w+");
        if (fpo == NULL ) {
            ALOGI("failed to open output file %s", filepath);
            return -1;
        }

        //fwrite(&sessionID, sizeof(int32_t), 1, fpo);
        fprintf(fpo, "%d", sessionID);

        fclose(fpo);

        return 0;
    }
    void WifiDisplaySink::prepareHDCP()
    {
#if 1
        char prop[PROPERTY_VALUE_MAX];
        int ret = property_get("persist.miracast.hdcp2", prop, "false");

#if ANDROID_PLATFORM_SDK_VERSION <= 27
        status_t err;
#else
        Status err;
#endif
        ALOGI("prepareHDCP==>");
        while (1)
        {
            if (!mHDCPRunning)
            {
                ALOGI("prepareHDCP1==>");
                mHDCPLock.lock();
                break;
            }
            else
            {
                ALOGI("prepareHDCP2==>");
                sleep(2);
            }
        }
        ALOGI("prepareHDCP lock");
        if (!strcmp(prop, "true"))
        {
            if (mHDCP == 0)
            {
                {
                    mHDCPPort++;
                    err = makeHDCP();
                    mUsingHDCP = true;
                    mHDCPRunning = false;
#if ANDROID_PLATFORM_SDK_VERSION <= 27
                    if (err != OK)
                    {
                        ALOGE("Unable to instantiate HDCP component. "
                              "Not using HDCP after all.");
                        mUsingHDCP = false;
                    }
#else
                    if (err != Status::OK)
                    {
                        ALOGE("Unable to instantiate HDCP component. "
                              "Not using HDCP after all.");
                        mUsingHDCP = false;
                    }
#endif
                }
            }
            else
            {
                mUsingHDCP = false;
            }
        }
        mHDCPLock.unlock();
#endif
    }

    status_t WifiDisplaySink::sendPause(int32_t sessionID, const char *uri)
    {
        AString request = AStringPrintf("PAUSE %s RTSP/1.0\r\n", uri);
        AppendCommonResponse(&request, mNextCSeq);
        request.append(AStringPrintf("Session: %s\r\n", mPlaybackSessionID.c_str()));
        request.append("\r\n");

        ALOGI("\nSend Request:\n%s", request.c_str());

        status_t err = mNetSession->sendRequest(
                sessionID, request.c_str(), request.size());

        if (err != OK) {
            return err;
        }

        ALOGI("registerResponseHandler:id = %d, onReceivePauseResponse", mNextCSeq);

        registerResponseHandler(
                sessionID, mNextCSeq, &WifiDisplaySink::onReceivePauseResponse);

        ++mNextCSeq;

        return OK;
    }

    void WifiDisplaySink::onGetParameterRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data)
    {
        ALOGV("session %d received '%s'",
            sessionID, data->debugString().c_str());
        status_t err;

        if (mRTPSink == NULL)
        {
            mRTPSink = new RTPSink(mNetSession, mBufferProducer, mMsgNotify);
            looper()->registerHandler(mRTPSink);

            err = mRTPSink->init(sUseTCPInterleaving);

            if (err != OK)
            {
                looper()->unregisterHandler(mRTPSink->id());
                mRTPSink.clear();
                return;
            }
        }
        const char *content = data->getContent();
        if (strstr(content,"wfd_") == NULL) {
            AString responseAlive = "RTSP/1.0 200 OK\r\n";
            AppendCommonResponse(&responseAlive, cseq);
            responseAlive.append("\r\n");
            ALOGI("===== Response M16 Request =====");
            status_t errAlive = mNetSession->sendRequest(sessionID, responseAlive.c_str());
            CHECK_EQ(errAlive, (status_t)OK);
            return;
        }
        //ALOGI("!!! FIXME: HARD CODE with video_formats, audio_codecs and client_rtp_ports");

        char body[1024];

        /*modified by yalong.liu*/
        memset(body, 0, 1024);
        if (mNeedVideoFormats)
            snprintf(body, sizeof(body),
                     "wfd_video_formats: 28 00 02 02 %s %s %s 00 0000 0000 11 none none, 01 02 %s %s %s 00 0000 0000 11 none none\r\n",
                     mCEA.c_str(),
                     mVESA.c_str(),
                     mHH.c_str(),
                     mCEA.c_str(),
                     mVESA.c_str(),
                     mHH.c_str());

        if (mNeedAudioCodecs)
            snprintf(body + strlen(body), sizeof(body) - strlen(body), "wfd_audio_codecs: LPCM 00000003 00, AAC 00000001 00\r\n");

        if (mNeed3dVideoFormats)
            snprintf(body + strlen(body), sizeof(body) - strlen(body), "wfd_3d_video_formats: none\r\n");

        if (mUsingHDCP && mNeedHDCP)
        {
            snprintf(body + strlen(body), sizeof(body) - strlen(body),
                     "wfd_content_protection: HDCP2.1 port=%d\r\n",
                     mHDCPPort);
        }
        else if (mNeedHDCP)
            snprintf(body + strlen(body), sizeof(body) - strlen(body), "wfd_content_protection: none\r\n");

        if (mNeedDisplayEdid)
            snprintf(body + strlen(body), sizeof(body) - strlen(body), "wfd_display_edid: none\r\n");

        if (mNeedCoupledSink)
            snprintf(body + strlen(body), sizeof(body) - strlen(body), "wfd_coupled_sink: none\r\n");

        if (mNeedclientRtpPorts)
            snprintf(body + strlen(body), sizeof(body) - strlen(body),
                     "wfd_client_rtp_ports: RTP/AVP/UDP;unicast %d 0 mode=play\r\n",
                     mRTPSink->getRTPPort());

        if (mNeedUibcCapability)
            snprintf(body+ strlen(body), sizeof(body) - strlen(body),
                "wfd_uibc_capability: none\r\n");

        if (mNeedStandbyResumeCapability)
            snprintf(body + strlen(body), sizeof(body) - strlen(body), "wfd_standby_resume_capability: supported\r\n");

        if (mNeedConnectorType)
            snprintf(body+ strlen(body), sizeof(body) - strlen(body),
                "wfd_connector_type: 00 00\r\n");

        if (mNeedI2C)
            snprintf(body+ strlen(body), sizeof(body) - strlen(body),
                "wfd_I2C: none\r\n");

        AString response = "RTSP/1.0 200 OK\r\n";
        AppendCommonResponse(&response, cseq);
        response.append("Content-Type: text/parameters\r\n");
        response.append(AStringPrintf("Content-Length: %d\r\n", strlen(body)));
        response.append("\r\n");
        response.append(body);

        ALOGI("\nSend Response:\n%s", response.c_str());
        err = mNetSession->sendRequest(sessionID, response.c_str());
        CHECK_EQ(err, (status_t)OK);
    }

    status_t WifiDisplaySink::sendDescribe(int32_t sessionID, const char *uri)
    {
        AString request = AStringPrintf("DESCRIBE %s RTSP/1.0\r\n", uri);
        AppendCommonResponse(&request, mNextCSeq);

        request.append("Accept: application/sdp\r\n");
        request.append("\r\n");

        ALOGI("\nSend Request:\n%s", request.c_str());

        status_t err = mNetSession->sendRequest(
                           sessionID, request.c_str(), request.size());

        if (err != OK)
        {
            return err;
        }

        ALOGI("registerResponseHandler:id = %d, onReceiveDescribeResponse", mNextCSeq);

        registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceiveDescribeResponse);

        ++mNextCSeq;

        return OK;
    }

    status_t WifiDisplaySink::sendSetup(int32_t sessionID, const char *uri)
    {
        status_t err;

        if (mRTPSink == NULL)
        {
            mRTPSink = new RTPSink(mNetSession, mBufferProducer, mMsgNotify);
            looper()->registerHandler(mRTPSink);

            err = mRTPSink->init(sUseTCPInterleaving);

            if (err != OK)
            {
                looper()->unregisterHandler(mRTPSink->id());
                mRTPSink.clear();
                return err;
            }
        }

        AString request = AStringPrintf("SETUP %s RTSP/1.0\r\n", uri);

        AppendCommonResponse(&request, mNextCSeq);

        if (sUseTCPInterleaving)
        {
            request.append("Transport: RTP/AVP/TCP;interleaved=0-1\r\n");
        }
        else
        {
            int32_t rtpPort = mRTPSink->getRTPPort();

            request.append(
                AStringPrintf(
                    "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n",
                    rtpPort, rtpPort + 1));
        }

        request.append("\r\n");

        ALOGI("\nSend Request:\n%s", request.c_str());

        err = mNetSession->sendRequest(sessionID, request.c_str(), request.size());

        if (err != OK)
        {
            return err;
        }

        ALOGI("registerResponseHandler:id = %d, onReceiveSetupResponse", mNextCSeq);
        registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceiveSetupResponse);

        ++mNextCSeq;

        return OK;
    }

    status_t WifiDisplaySink::sendPlay(int32_t sessionID, const char *uri)
    {
        AString request = AStringPrintf("PLAY %s RTSP/1.0\r\n", uri);

        AppendCommonResponse(&request, mNextCSeq);

        request.append(AStringPrintf("Session: %s\r\n", mPlaybackSessionID.c_str()));
        request.append("\r\n");

        ALOGI("\nSend Request:\n%s", request.c_str());

        status_t err =
            mNetSession->sendRequest(sessionID, request.c_str(), request.size());

        if (err != OK)
        {
            return err;
        }

        ALOGI("registerResponseHandler:id = %d, onReceivePlayResponse", mNextCSeq);
        registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceivePlayResponse);

        ++mNextCSeq;

        return OK;
    }

    status_t WifiDisplaySink::sendTeardown(int32_t sessionID, const char *uri)
    {
        AString request = AStringPrintf("TEARDOWN %s RTSP/1.0\r\n", uri);
        AppendCommonResponse(&request, mNextCSeq);
        request.append(AStringPrintf("Session: %s\r\n", mPlaybackSessionID.c_str()));
        request.append("\r\n");

        ALOGI("\nSend Request:\n%s", request.c_str());

        status_t err = mNetSession->sendRequest(
                sessionID, request.c_str(), request.size());

        if (err != OK)
        {
            return err;
        }

        ALOGI("registerResponseHandler:id = %d, onReceiveTeardownResponse", mNextCSeq);

        registerResponseHandler(
                sessionID, mNextCSeq, &WifiDisplaySink::onReceiveTeardownResponse);

        ++mNextCSeq;

        return OK;
    }

    void WifiDisplaySink::onSetParameterRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data)
    {
        const char *content = data->getContent();

        AString response = "RTSP/1.0 200 OK\r\n";
        AppendCommonResponse(&response, cseq);
        response.append("\r\n");

        ALOGI("\nSend Response:\n%s", response.c_str());
        status_t err = mNetSession->sendRequest(sessionID, response.c_str());
        CHECK_EQ(err, (status_t)OK);

        if (strstr(content, "wfd_trigger_method: SETUP\r\n") != NULL) {
            AString url = AStringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str());
            status_t err = (status_t)OK;
            if (mSourceStandby == true) {
                err = sendPlay(sessionID, url.c_str());
                mSourceStandby = false;
            } else {
                err = sendSetup( sessionID, url.c_str());
            }
            CHECK_EQ(err, (status_t)OK);
        }
        else if (strstr(content, "wfd_trigger_method: TEARDOWN\r\n") != NULL)
        {
            AString url = AStringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str());
            status_t err = sendTeardown(sessionID, url.c_str());
            CHECK_EQ(err, (status_t)OK);
        } else if (strstr(content, "wfd_trigger_method: PAUSE\r\n") != NULL) {
            AString url = AStringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str());
            status_t err = sendPause(sessionID, url.c_str());
            CHECK_EQ(err, (status_t)OK);
        } else if (strstr(content, "wfd_trigger_method: PLAY\r\n") != NULL) {
            AString url = AStringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str());
            status_t err = sendPlay(sessionID, url.c_str());
            CHECK_EQ(err, (status_t)OK);
        } else if (strstr(content, "wfd_standby") != NULL) {
            ALOGE("the source should be in standby now");
            //We don't do anything just wait the source exit from standby
            mSourceStandby = true;
        } else {
            ALOGE("on setparameter receive other information");
        }
    }

    void WifiDisplaySink::sendErrorResponse(
        int32_t sessionID,
        const char *errorDetail,
        int32_t cseq)
    {
        AString response;
        response.append("RTSP/1.0 ");
        response.append(errorDetail);
        response.append("\r\n");

        AppendCommonResponse(&response, cseq);

        response.append("\r\n");

        ALOGI("\nSend Response:\n%s", response.c_str());
        status_t err = mNetSession->sendRequest(sessionID, response.c_str());
        CHECK_EQ(err, (status_t)OK);
    }

    // static
    void WifiDisplaySink::AppendCommonResponse(AString *response, int32_t cseq)
    {
        time_t now = time(NULL);
        struct tm *now2 = gmtime(&now);
        char buf[128];
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %z", now2);

        response->append("Date: ");
        response->append(buf);
        response->append("\r\n");

        response->append("User-Agent: stagefright/1.1 (Linux;Android 9.0)\r\n");

        if (cseq >= 0)
        {
            response->append(AStringPrintf("CSeq: %d\r\n", cseq));
        }
    }

    status_t WifiDisplaySink::onReceiveIDRFrameRequestResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg) {
        CHECK(mIDRFrameRequestPending);
        mIDRFrameRequestPending = false;
        ALOGI("sessionID=%d, has msg = %d", sessionID, msg != NULL);
        return OK;
    }

    status_t WifiDisplaySink::sendIDRFrameRequest(int32_t sessionID) {
        CHECK(!mIDRFrameRequestPending);
        AString request = "SET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0\r\n";
        AppendCommonResponse(&request, mNextCSeq);
        AString content = "wfd_idr_request\r\n";
        request.append(AStringPrintf("Session: %s\r\n", mPlaybackSessionID.c_str()));
        request.append(AStringPrintf("Content-Length: %d\r\n", content.size()));
        request.append("\r\n");
        request.append(content);
        ALOGD("===== Send IDR Request =====");
        status_t err = mNetSession->sendRequest(sessionID, request.c_str(), request.size());
        if (err != OK) {
            return err;
        }
        registerResponseHandler(
            sessionID,
            mNextCSeq,
            &WifiDisplaySink::onReceiveIDRFrameRequestResponse);
        ++mNextCSeq;
        mIDRFrameRequestPending = true;
        return OK;
    }

#if 1
    WifiDisplaySink::HDCPObserver::HDCPObserver(
        const sp<AMessage> &notify)
        : mNotify(notify)
    {
    }

#if ANDROID_PLATFORM_SDK_VERSION <= 27
    void WifiDisplaySink::HDCPObserver::notify(
        int msg, int ext1, int ext2, const Parcel *obj)
    {
        sp<AMessage> notify = mNotify->dup();
        notify->setInt32("msg", msg);
        notify->setInt32("ext1", ext1);
        notify->setInt32("ext2", ext2);
        notify->post();
        return;
    }

    status_t WifiDisplaySink::makeHDCP()
    {
        ALOGE("[%s %d]\n", __FUNCTION__, __LINE__);
        if (mHDCP != NULL) {
            return OK;
        }
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder = sm->getService(String16("media.player"));

        sp<IMediaPlayerService> service =
            interface_cast<IMediaPlayerService>(binder);

        CHECK(service != NULL);

        mHDCP = service->makeHDCP(false /* createEncryptionModule */);
        if (mHDCP == NULL) {
            ALOGE("makeHDCP failed");
            return ERROR_UNSUPPORTED;
        }
        ALOGE("[HDCP2.X Rx] makeHDCP, mHDCP is not NULL");
        sp<AMessage> notify = new AMessage(kWhatHDCPNotify, this);
        mHDCPObserver = new HDCPObserver(notify);

        status_t err = mHDCP->setObserver(mHDCPObserver);

        if (err != (status_t)OK) {
            ALOGE("Failed to set HDCP observer.");

            mHDCPObserver.clear();
            mHDCP.clear();

            return err;
        }

        err = mHDCP->initAsync(mRTSPHost.c_str(), mHDCPPort);

        if (err != (status_t)OK)
        {
            return err;
        }

        return OK;
    }
#else
    Return<void> WifiDisplaySink::HDCPObserver::notify(
        int msg, int ext1, int ext2)
    {
        sp<AMessage> notify = mNotify->dup();
        notify->setInt32("msg", msg);
        notify->setInt32("ext1", ext1);
        notify->setInt32("ext2", ext2);
        notify->post();
        return Void();
    }

    Status WifiDisplaySink::makeHDCP()
    {
        ALOGE("[%s %d]\n", __FUNCTION__, __LINE__);
        sp<IHDCPService> hdcp_service = IHDCPService::getService();
        if (!hdcp_service) {
            ALOGE("failed to get hdcp2 service\n");
            return Status::NO_INIT;
        }

        mHDCP = IHDCP::castFrom(hdcp_service->makeHDCP(false));

        if (mHDCP == NULL)
        {
            ALOGE("failed to cast from hdcp2 service\n");
            return Status::NO_INIT;
        }

        sp<AMessage> notify = new AMessage(kWhatHDCPNotify, this);
        mHDCPObserver = new HDCPObserver(notify);

        Status err = mHDCP->setObserver(mHDCPObserver);

        if (err != Status::OK)
        {
            ALOGE("Failed to set HDCP observer.");

            mHDCPObserver.clear();
            mHDCP.clear();

            return err;
        }

        err = mHDCP->initAsync(mRTSPHost.c_str(), mHDCPPort);

        if (err != Status::OK)
        {
            return err;
        }

        return Status::OK;
    }
#endif
#endif
}  // namespace android
