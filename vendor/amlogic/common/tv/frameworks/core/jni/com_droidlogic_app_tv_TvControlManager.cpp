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
#define LOG_TAG "tv-jni"
#include "com_droidlogic_app_tv_TvControlManager.h"
#include <vendor/amlogic/hardware/hdmicec/1.0/IDroidHdmiCEC.h>

static sp<TvServerHidlClient> spTv = NULL;
sp<EventCallback> spEventCB;
static jobject TvObject;
static jmethodID notifyCallback;
static JavaVM   *gJavaVM = NULL;
static jclass    notifyClazz;


static JNIEnv* getJniEnv(bool *needDetach) {
    int ret = -1;
    JNIEnv *env = NULL;
    ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
    if (ret < 0) {
        ret = gJavaVM->AttachCurrentThread(&env, NULL);
        if (ret < 0) {
            ALOGE("Can't attach thread ret = %d", ret);
            return NULL;
        }
        *needDetach = true;
    }
    return env;
}

static void DetachJniEnv() {
    int result = gJavaVM->DetachCurrentThread();
    if (result != JNI_OK) {
        ALOGE("thread detach failed: %#x", result);
    }
}

#if 0
static void ScanServicesEvent(const tv_parcel_t &parcel) {
    bool needDetach = false;
    JNIEnv *env = getJniEnv(&needDetach);
    if (env != NULL && TvObject != NULL) {
        ALOGD("[%s] ScanServicesEvent", __FUNCTION__);
        //jclass jniClass = env->FindClass("com/droidlogic/app/tv/TvControlManager$ScannerEvent");
        jclass jniClass = scanClazz;
        if (jniClass == NULL) {
            ALOGD("[%s] jniClass is null", __FUNCTION__);
            return;
        }
        jmethodID consID = env->GetMethodID(jniClass, "<init>", "()V");
        jobject scannerEvent = env->NewObject(jniClass, consID);
        jfieldID jtype = env->GetFieldID(jniClass, "type", "I");
        env->SetIntField(scannerEvent, jtype, parcel.bodyInt[0]);
        jfieldID jprecent = env->GetFieldID(jniClass, "precent", "I");
        env->SetIntField(scannerEvent, jprecent, parcel.bodyInt[1]);
        jfieldID jtotalcount = env->GetFieldID(jniClass, "totalcount", "I");
        env->SetIntField(scannerEvent, jtotalcount, parcel.bodyInt[2]);
        jfieldID jlock = env->GetFieldID(jniClass, "lock", "I");
        env->SetIntField(scannerEvent, jlock, parcel.bodyInt[3]);
        jfieldID jcnum = env->GetFieldID(jniClass, "cnum", "I");
        env->SetIntField(scannerEvent, jcnum, parcel.bodyInt[4]);
        jfieldID jfreq = env->GetFieldID(jniClass, "freq", "I");
        env->SetIntField(scannerEvent, jfreq, parcel.bodyInt[5]);
        jfieldID jprogramName = env->GetFieldID(jniClass, "programName", "Ljava/lang/String;");
        jstring jstr0 = env->NewStringUTF(parcel.bodyString[0].c_str());
        env->SetObjectField(scannerEvent, jprogramName, jstr0);
        env->DeleteLocalRef(jstr0);
        jfieldID jsrvType = env->GetFieldID(jniClass, "srvType", "I");
        env->SetIntField(scannerEvent, jsrvType, parcel.bodyInt[6]);
        jfieldID jparas = env->GetFieldID(jniClass, "paras", "Ljava/lang/String;");
        jstring jstr1 = env->NewStringUTF(parcel.bodyString[1].c_str());
        env->SetObjectField(scannerEvent, jparas, jstr1);
        env->DeleteLocalRef(jstr1);
        jfieldID jstrength = env->GetFieldID(jniClass, "strength", "I");
        env->SetIntField(scannerEvent, jstrength, parcel.bodyInt[7]);
        jfieldID jquality = env->GetFieldID(jniClass, "quality", "I");
        env->SetIntField(scannerEvent, jquality, parcel.bodyInt[8]);
        jfieldID jvideoStd = env->GetFieldID(jniClass, "videoStd", "I");
        env->SetIntField(scannerEvent, jvideoStd, parcel.bodyInt[9]);
        jfieldID jaudioStd = env->GetFieldID(jniClass, "audioStd", "I");
        env->SetIntField(scannerEvent, jaudioStd, parcel.bodyInt[10]);
        jfieldID jisAutoStd = env->GetFieldID(jniClass, "isAutoStd", "I");
        env->SetIntField(scannerEvent, jisAutoStd, parcel.bodyInt[11]);
        jfieldID jmode = env->GetFieldID(jniClass, "mode", "I");
        env->SetIntField(scannerEvent, jmode, parcel.bodyInt[12]);
        jfieldID jsr = env->GetFieldID(jniClass, "sr", "I");
        env->SetIntField(scannerEvent, jsr, parcel.bodyInt[13]);
        jfieldID jmod = env->GetFieldID(jniClass, "mod", "I");
        env->SetIntField(scannerEvent, jmod, parcel.bodyInt[14]);
        jfieldID jbandwidth = env->GetFieldID(jniClass, "bandwidth", "I");
        env->SetIntField(scannerEvent, jbandwidth, parcel.bodyInt[15]);
        jfieldID jreserved = env->GetFieldID(jniClass, "reserved", "I");
        env->SetIntField(scannerEvent, jreserved, parcel.bodyInt[16]);
        jfieldID jts_id = env->GetFieldID(jniClass, "ts_id", "I");
        env->SetIntField(scannerEvent, jts_id, parcel.bodyInt[17]);
        jfieldID jorig_net_id = env->GetFieldID(jniClass, "orig_net_id", "I");
        env->SetIntField(scannerEvent, jorig_net_id, parcel.bodyInt[18]);
        jfieldID jserviceID = env->GetFieldID(jniClass, "serviceID", "I");
        env->SetIntField(scannerEvent, jserviceID, parcel.bodyInt[19]);
        jfieldID jvid = env->GetFieldID(jniClass, "vid", "I");
        env->SetIntField(scannerEvent, jvid, parcel.bodyInt[20]);
        jfieldID jvfmt = env->GetFieldID(jniClass, "vfmt", "I");
        env->SetIntField(scannerEvent, jvfmt, parcel.bodyInt[21]);
        jint acnt = parcel.bodyInt[22];
        if (acnt != 0) {
            //aids
            jfieldID jaids = env->GetFieldID(jniClass, "aids", "[I");
            jintArray jaidsArray = env->NewIntArray(acnt);
            jint *jaidsdata = env->GetIntArrayElements(jaidsArray, 0);
            for (jint i = 0; i < acnt; i++)
 {
                jaidsdata[i] = parcel.bodyInt[i+23];
            }
            env->SetIntArrayRegion(jaidsArray, 0, acnt, (jint*)jaidsdata);
            env->SetObjectField(scannerEvent, jaids, jaidsArray);
            //afmts
            jfieldID jafmts = env->GetFieldID(jniClass, "afmts", "[I");
            jintArray jafmtsArray = env->NewIntArray(acnt);
            jint *jafmtsdata = env->GetIntArrayElements(jafmtsArray, 0);
            for (jint i = 0; i < acnt; i++)
 {
                jafmtsdata[i] = parcel.bodyInt[i+acnt+23];
            }
            env->SetIntArrayRegion(jafmtsArray, 0, acnt, (jint*)jafmtsdata);
            env->SetObjectField(scannerEvent, jafmts, jafmtsArray);
            //alangs
            jfieldID jalangs = env->GetFieldID(jniClass, "alangs", "[Ljava/lang/String;");
            jobjectArray jalangsArray = env->NewObjectArray(acnt, env->FindClass("java/lang/String"), nullptr);
            for (jint i = 0; i < acnt; i++) {
                jstring jstr = env->NewStringUTF(parcel.bodyString[i+2].c_str());
                env->SetObjectArrayElement(jalangsArray, i, jstr);
                env->DeleteLocalRef(jstr);
            }
            env->SetObjectField(scannerEvent, jalangs, jalangsArray);
            //atypes
            jfieldID jatypes = env->GetFieldID(jniClass, "atypes", "[I");
            jintArray jatypesArray = env->NewIntArray(acnt);
            jint *jatypesdata = env->GetIntArrayElements(jatypesArray, 0);
            for (jint i = 0; i < acnt; i++)
 {
                jatypesdata[i] = parcel.bodyInt[i+2*acnt+23];
            }
            env->SetIntArrayRegion(jatypesArray, 0, acnt, (jint*)jatypesdata);
            env->SetObjectField(scannerEvent, jatypes, jatypesArray);
            //aexts
            jfieldID jaexts = env->GetFieldID(jniClass, "aexts", "[I");
            jintArray jaextsArray = env->NewIntArray(acnt);
            jint *jaextsdata = env->GetIntArrayElements(jaextsArray, 0);
            for (jint i = 0; i < acnt; i++)
 {
                jaextsdata[i] = parcel.bodyInt[i+3*acnt+23];
            }
            env->SetIntArrayRegion(jaextsArray, 0, acnt, (jint*)jaextsdata);
            env->SetObjectField(scannerEvent, jaexts, jaextsArray);
        }
        jfieldID jpcr = env->GetFieldID(jniClass, "pcr", "I");
        env->SetIntField(scannerEvent, jpcr, parcel.bodyInt[4*acnt+23]);
        jint scnt = parcel.bodyInt[4*acnt+24];
        if (scnt != 0) {
            //stypes
            jfieldID jstypes = env->GetFieldID(jniClass, "stypes", "[I");
            jintArray jstypesArray = env->NewIntArray(scnt);
            jint *jstypesdata = env->GetIntArrayElements(jstypesArray, 0);
            for (jint i = 0; i < acnt; i++)
 {
                jstypesdata[i] = parcel.bodyInt[i+4*acnt+25];
            }
            env->SetIntArrayRegion(jstypesArray, 0, scnt, (jint*)jstypesdata);
            env->SetObjectField(scannerEvent, jstypes, jstypesArray);
            //sids
            jfieldID jsids = env->GetFieldID(jniClass, "sids", "[I");
            jintArray jsidsArray = env->NewIntArray(scnt);
            jint *jsidsdata = env->GetIntArrayElements(jsidsArray, 0);
            for (jint i = 0; i < acnt; i++)
 {
                jsidsdata[i] = parcel.bodyInt[i+scnt+4*acnt+25];
            }
            env->SetIntArrayRegion(jsidsArray, 0, scnt, (jint*)jsidsdata);
            env->SetObjectField(scannerEvent, jsids, jsidsArray);
            //sstypes
            jfieldID jsstypes = env->GetFieldID(jniClass, "sstypes", "[I");
            jintArray jsstypesArray = env->NewIntArray(scnt);
            jint *jsstypesdata = env->GetIntArrayElements(jsstypesArray, 0);
            for (jint i = 0; i < acnt; i++)
 {
                jsstypesdata[i] = parcel.bodyInt[i+2*scnt+4*acnt+25];
            }
            env->SetIntArrayRegion(jsstypesArray, 0, scnt, (jint*)jsstypesdata);
            env->SetObjectField(scannerEvent, jsstypes, jsstypesArray);
            //sid1s
            jfieldID jsid1s = env->GetFieldID(jniClass, "sid1s", "[I");
            jintArray jsid1sArray = env->NewIntArray(scnt);
            jint *jsid1sdata = env->GetIntArrayElements(jsid1sArray, 0);
            for (jint i = 0; i < acnt; i++)
 {
                jsid1sdata[i] = parcel.bodyInt[i+3*scnt+4*acnt+25];
            }
            env->SetIntArrayRegion(jsid1sArray, 0, scnt, (jint*)jsid1sdata);
            env->SetObjectField(scannerEvent, jsid1s, jsid1sArray);
            //sid2s
            jfieldID jsid2s = env->GetFieldID(jniClass, "sid2s", "[I");
            jintArray jsid2sArray = env->NewIntArray(scnt);
            jint *jsid2sdata = env->GetIntArrayElements(jsid2sArray, 0);
            for (jint i = 0; i < acnt; i++)
 {
                jsid2sdata[i] = parcel.bodyInt[i+4*scnt+4*acnt+25];
            }
            env->SetIntArrayRegion(jsid2sArray, 0, scnt, (jint*)jsid2sdata);
            env->SetObjectField(scannerEvent, jsid2s, jsid2sArray);
            //slangs
            jfieldID jslangs = env->GetFieldID(jniClass, "slangs", "[Ljava/lang/String;");
            jobjectArray jslangsArray = env->NewObjectArray(scnt, env->FindClass("java/lang/String"), nullptr);
            for (jint i = 0; i < scnt; i++) {
                jstring jstr = env->NewStringUTF(parcel.bodyString[i+acnt+2].c_str());
                env->SetObjectArrayElement(jslangsArray, i, jstr);
                env->DeleteLocalRef(jstr);
            }
            env->SetObjectField(scannerEvent, jslangs, jslangsArray);
        }
        jfieldID jfree_ca = env->GetFieldID(jniClass, "free_ca", "I");
        env->SetIntField(scannerEvent, jfree_ca, parcel.bodyInt[5*scnt+4*acnt+25]);
        jfieldID jscrambled = env->GetFieldID(jniClass, "scrambled", "I");
        env->SetIntField(scannerEvent, jscrambled, parcel.bodyInt[5*scnt+4*acnt+26]);
        jfieldID jscan_mode = env->GetFieldID(jniClass, "scan_mode", "I");
        env->SetIntField(scannerEvent, jscan_mode, parcel.bodyInt[5*scnt+4*acnt+27]);
        jfieldID jsdtVersion = env->GetFieldID(jniClass, "sdtVersion", "I");
        env->SetIntField(scannerEvent, jsdtVersion, parcel.bodyInt[5*scnt+4*acnt+28]);
        jfieldID jsort_mode = env->GetFieldID(jniClass, "sort_mode", "I");
        env->SetIntField(scannerEvent, jsort_mode, parcel.bodyInt[5*scnt+4*acnt+29]);
        #if 0
        //start ScannerLcnInfo
        //jfieldID jlcnInfo = env->GetFieldID(jniClass, "lcnInfo", "com/droidlogic/app/tv/TvControlManager$ScannerLcnInfo");
        //jclass lcnInfoClass = env->FindClass("com/droidlogic/app/tv/TvControlManager$ScannerLcnInfo");
        jclass lcnInfoClass = scanLcnClazz;
        if (lcnInfoClass == NULL) {
            ALOGD("[%s] lcnInfoClass is null", __FUNCTION__);
            return;
        }
        jmethodID lcnInfoID = env->GetMethodID(lcnInfoClass, "<init>", "()V");
        jobject scannerLcnInfo = env->NewObject(lcnInfoClass, lcnInfoID);
        jfieldID jnetId = env->GetFieldID(lcnInfoClass, "netId", "I");
        env->SetIntField(scannerLcnInfo, jnetId, parcel.bodyInt[5*scnt+4*acnt+30]);
        jfieldID jtsId = env->GetFieldID(lcnInfoClass, "tsId", "I");
        env->SetIntField(scannerLcnInfo, jtsId, parcel.bodyInt[5*scnt+4*acnt+31]);
        jfieldID jserviceId = env->GetFieldID(lcnInfoClass, "serviceId", "I");
        env->SetIntField(scannerLcnInfo, jserviceId, parcel.bodyInt[5*scnt+4*acnt+32]);
        jfieldID jvisible = env->GetFieldID(lcnInfoClass, "visible", "[I");
        jfieldID jlcn = env->GetFieldID(lcnInfoClass, "lcn", "[I");
        jfieldID jvalid = env->GetFieldID(lcnInfoClass, "valid", "[I");
        jintArray jvisibleArray = env->NewIntArray(4);
        jintArray jlcnArray = env->NewIntArray(4);
        jintArray jvalidArray = env->NewIntArray(4);
        jint *jvisibledata = env->GetIntArrayElements(jvisibleArray, 0);
        jint *jlcndata = env->GetIntArrayElements(jlcnArray, 0);
        jint *jvaliddata = env->GetIntArrayElements(jvalidArray, 0);
        for (jint i = 0; i < 4; i++) {
            jvisibledata[i] = parcel.bodyInt[i*3+5*scnt+4*acnt+33];
            jlcndata[i] = parcel.bodyInt[i*3+5*scnt+4*acnt+34];
            jvaliddata[i] = parcel.bodyInt[i*3+5*scnt+4*acnt+35];
        }
        env->SetIntArrayRegion(jvisibleArray, 0, 4, (jint*)jvisibledata);
        env->SetIntArrayRegion(jlcnArray, 0, 4, (jint*)jlcndata);
        env->SetIntArrayRegion(jvalidArray, 0, 4, (jint*)jvaliddata);
        env->SetObjectField(scannerLcnInfo, jvisible, jvisibleArray);
        env->SetObjectField(scannerLcnInfo, jlcn, jlcnArray);
        env->SetObjectField(scannerLcnInfo, jvalid, jvalidArray);
        if (gjlcnInfo != NULL)
            env->SetObjectField(scannerEvent, gjlcnInfo, scannerLcnInfo);
        //end ScannerLcnInfo
        #endif
        jint jnetId = parcel.bodyInt[5*scnt+4*acnt+30];
        jint jtsId  = parcel.bodyInt[5*scnt+4*acnt+31];
        jint jserviceId = parcel.bodyInt[5*scnt+4*acnt+32];
        jintArray jvisibleArray = env->NewIntArray(4);
        jintArray jlcnArray = env->NewIntArray(4);
        jintArray jvalidArray = env->NewIntArray(4);
        jint *jvisibledata = env->GetIntArrayElements(jvisibleArray, 0);
        jint *jlcndata = env->GetIntArrayElements(jlcnArray, 0);
        jint *jvaliddata = env->GetIntArrayElements(jvalidArray, 0);

        for (jint i = 0; i < 4; i++) {
            jvisibledata[i] = parcel.bodyInt[i*3+5*scnt+4*acnt+33];
            jlcndata[i] = parcel.bodyInt[i*3+5*scnt+4*acnt+34];
            jvaliddata[i] = parcel.bodyInt[i*3+5*scnt+4*acnt+35];
        }
        env->SetIntArrayRegion(jvisibleArray, 0, 4, (jint*)jvisibledata);
        env->SetIntArrayRegion(jlcnArray, 0, 4, (jint*)jlcndata);
        env->SetIntArrayRegion(jvalidArray, 0, 4, (jint*)jvaliddata);

        jfieldID jmajorChannelNumber = env->GetFieldID(jniClass, "majorChannelNumber", "I");
        env->SetIntField(scannerEvent, jmajorChannelNumber, parcel.bodyInt[4*3+5*scnt+4*acnt+33]);
        jfieldID jminorChannelNumber = env->GetFieldID(jniClass, "minorChannelNumber", "I");
        env->SetIntField(scannerEvent, jminorChannelNumber, parcel.bodyInt[4*3+5*scnt+4*acnt+34]);
        jfieldID jsourceId = env->GetFieldID(jniClass, "sourceId", "I");
        env->SetIntField(scannerEvent, jsourceId, parcel.bodyInt[4*3+5*scnt+4*acnt+35]);
        jfieldID jaccessControlled = env->GetFieldID(jniClass, "accessControlled", "I");
        env->SetIntField(scannerEvent, jaccessControlled, parcel.bodyInt[4*3+5*scnt+4*acnt+36]);
        jfieldID jhidden = env->GetFieldID(jniClass, "hidden", "I");
        env->SetIntField(scannerEvent, jhidden, parcel.bodyInt[4*3+5*scnt+4*acnt+37]);
        jfieldID jhideGuide = env->GetFieldID(jniClass, "hideGuide", "I");
        env->SetIntField(scannerEvent, jhideGuide, parcel.bodyInt[4*3+5*scnt+4*acnt+38]);
        jfieldID jvct = env->GetFieldID(jniClass, "vct", "Ljava/lang/String;");
        jstring jstr2 = env->NewStringUTF(parcel.bodyString[scnt+acnt+2].c_str());
        env->SetObjectField(scannerEvent, jvct, jstr2);
        env->DeleteLocalRef(jstr2);
        jfieldID jprograms_in_pat = env->GetFieldID(jniClass, "programs_in_pat", "I");
        env->SetIntField(scannerEvent, jprograms_in_pat, parcel.bodyInt[4*3+5*scnt+4*acnt+39]);
        jfieldID jpat_ts_id = env->GetFieldID(jniClass, "pat_ts_id", "I");
        env->SetIntField(scannerEvent, jpat_ts_id, parcel.bodyInt[4*3+5*scnt+4*acnt+40]);
        env->CallVoidMethod(TvObject, notifyScanEventCallback, scannerEvent, jnetId, jtsId, jserviceId, jvisibleArray,
        jlcnArray, jvalidArray);
    } else {
        ALOGE("[%s] env or TvObject is NULL", __FUNCTION__);
    }
    if (needDetach) {
        DetachJniEnv();
    }

}
#endif
void EventCallback::notify (const tv_parcel_t &parcel) {
    ALOGD("eventcallback notify parcel.msgType = %d", parcel.msgType);
    //AutoMutex _l(mLock);
    bool needDetach = false;
    JNIEnv *env = getJniEnv(&needDetach);
    if (env != NULL && TvObject != NULL && notifyClazz != NULL) {
        jclass jniClass = notifyClazz;
        jmethodID consID = env->GetMethodID(jniClass, "<init>", "()V");
        jobject hidlParcel = env->NewObject(jniClass, consID);
        jfieldID jmsgType = env->GetFieldID(jniClass, "msgType", "I");
        env->SetIntField(hidlParcel, jmsgType, parcel.msgType);
        jfieldID jbodyInt = env->GetFieldID(jniClass, "bodyInt", "[I");
        jint sizeInt = parcel.bodyInt.size();
        jintArray jbodyIntArray = env->NewIntArray(sizeInt);
        jint *jbodyIntdata = env->GetIntArrayElements(jbodyIntArray, 0);
        for (jint i = 0; i < sizeInt; i++)
 {
            jbodyIntdata[i] = parcel.bodyInt[i];
        }
        env->SetIntArrayRegion(jbodyIntArray, 0, sizeInt, (jint*)jbodyIntdata);
        env->SetObjectField(hidlParcel, jbodyInt, jbodyIntArray);

        jfieldID jbodyString = env->GetFieldID(jniClass, "bodyString", "[Ljava/lang/String;");
        jint sizeStr = parcel.bodyString.size();
        jobjectArray jbodyStringArray = env->NewObjectArray(sizeStr, env->FindClass("java/lang/String"), nullptr);
        for (jint i = 0; i < sizeStr; i++) {
            jstring jstr = env->NewStringUTF(parcel.bodyString[i].c_str());
            env->SetObjectArrayElement(jbodyStringArray, i, jstr);
            env->DeleteLocalRef(jstr);
        }
        env->SetObjectField(hidlParcel, jbodyString, jbodyStringArray);
        env->CallVoidMethod(TvObject, notifyCallback, hidlParcel);
    } else {
        ALOGE("[%s] env, TvObject or notifyClazz is NULL", __FUNCTION__);
    }

    if (needDetach) {
        DetachJniEnv();
    }
}

const sp<TvServerHidlClient>& getTvClient()
{
    if (spTv == NULL) {
        spTv = TvServerHidlClient::connect(CONNECT_TYPE_EXTEND);
        //spEventCB = new EventCallback();
        //spTv->setListener(spEventCB);
    }
    return spTv;
}

static void ConnectTvServer(JNIEnv *env, jclass clazz __unused, jobject obj)
{
    ALOGI("Connect Tv server");
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL) {
        spEventCB = new EventCallback();
        Tv->setListener(spEventCB);
        TvObject = env->NewGlobalRef(obj);
    }
}

static void DisConnectTvServer(JNIEnv *env, jclass clazz __unused, jobject obj)
{
    ALOGI("disConnect Tv server");
    spEventCB = NULL;
    env->DeleteGlobalRef(TvObject);
}


static jstring GetSupportInputDevices(JNIEnv *env, jclass clazz __unused) {
    std::string tvDevices;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL)
        tvDevices = Tv->getSupportInputDevices();
    return env->NewStringUTF(tvDevices.c_str());
}

static jstring GetTvSupportCountries(JNIEnv *env, jclass clazz __unused) {
    std::string tvCountries;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL)
        tvCountries = Tv->getTvSupportCountries();
    return env->NewStringUTF(tvCountries.c_str());
}

static jstring GetTvDefaultCountry(JNIEnv *env, jclass clazz __unused) {
    std::string Country;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL)
        Country = Tv->getTvDefaultCountry();
    return env->NewStringUTF(Country.c_str());
}

static jstring GetTvCountryName(JNIEnv *env, jclass clazz __unused, jstring jcountry_code) {
    std::string CountryName;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL) {
        const char *country_code = env->GetStringUTFChars(jcountry_code, nullptr);
        CountryName = Tv->getTvCountryName(country_code);
        env->ReleaseStringUTFChars(jcountry_code, country_code);
    }
    return env->NewStringUTF(CountryName.c_str());
}

static jstring GetTvSearchMode(JNIEnv *env, jclass clazz __unused, jstring jcountry_code) {
    std::string mode;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL) {
        const char *country_code = env->GetStringUTFChars(jcountry_code, nullptr);
        mode = Tv->getTvSearchMode(country_code);
        env->ReleaseStringUTFChars(jcountry_code, country_code);
    }
    return env->NewStringUTF(mode.c_str());
}

static jboolean GetTvDtvSupport(JNIEnv *env, jclass clazz __unused, jstring jcountry_code) {
    jboolean result = false;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL) {
        const char *country_code = env->GetStringUTFChars(jcountry_code, nullptr);
        result = Tv->getTvDtvSupport(country_code);
        env->ReleaseStringUTFChars(jcountry_code, country_code);
    }
    return result;
}

static jstring GetTvDtvSystem(JNIEnv *env, jclass clazz __unused, jstring jcountry_code) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    std::string dtvSystem;
    if (Tv != NULL) {
        const char *country_code = env->GetStringUTFChars(jcountry_code, nullptr);
        dtvSystem = Tv->getTvDtvSystem(country_code);
        env->ReleaseStringUTFChars(jcountry_code, country_code);
    }
    return env->NewStringUTF(dtvSystem.c_str());
}

static jboolean GetTvAtvSupport(JNIEnv *env, jclass clazz __unused, jstring jcountry_code) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jboolean result = false;
    if (Tv != NULL) {
        const char *country_code = env->GetStringUTFChars(jcountry_code, nullptr);
        result = Tv->getTvAtvSupport(country_code);
        env->ReleaseStringUTFChars(jcountry_code, country_code);
    }
    return result;
}

static jstring GetTvAtvColorSystem(JNIEnv *env, jclass clazz __unused, jstring jcountry_code) {
    std::string ColorSystem;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL) {
        const char *country_code = env->GetStringUTFChars(jcountry_code, nullptr);
        ColorSystem = Tv->getTvAtvColorSystem(country_code);
        env->ReleaseStringUTFChars(jcountry_code, country_code);
    }
    return env->NewStringUTF(ColorSystem.c_str());
}

static jstring GetTvAtvSoundSystem(JNIEnv *env, jclass clazz __unused, jstring jcountry_code) {
    std::string SoundSystem;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL) {
        const char *country_code = env->GetStringUTFChars(jcountry_code, nullptr);
        SoundSystem = Tv->getTvAtvSoundSystem(country_code);
        env->ReleaseStringUTFChars(jcountry_code, country_code);
    }
    return env->NewStringUTF(SoundSystem.c_str());
}

static jstring GetTvAtvMinMaxFreq(JNIEnv *env, jclass clazz __unused, jstring jcountry_code) {
    std::string MinMaxFreq;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL) {
        const char *country_code = env->GetStringUTFChars(jcountry_code, nullptr);
        MinMaxFreq = Tv->getTvAtvMinMaxFreq(country_code);
        env->ReleaseStringUTFChars(jcountry_code, country_code);
    }
    return env->NewStringUTF(MinMaxFreq.c_str());
}

static jboolean GetTvAtvStepScan(JNIEnv *env, jclass clazz __unused, jstring jcountry_code) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jboolean result = false;
    if (Tv != NULL) {
        const char *country_code = env->GetStringUTFChars(jcountry_code, nullptr);
        result = Tv->getTvAtvStepScan(country_code);
        env->ReleaseStringUTFChars(jcountry_code, country_code);
    }
    return result;
}

static void SetTvCountry(JNIEnv *env, jclass clazz __unused, jstring jcountry) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL) {
        const char *country = env->GetStringUTFChars(jcountry, nullptr);
        Tv->setTvCountry(country);
        env->ReleaseStringUTFChars(jcountry, country);
    }
}

static void SetCurrentLanguage(JNIEnv *env, jclass clazz __unused, jstring jlang) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL) {
        const char *lang = env->GetStringUTFChars(jlang, nullptr);
        Tv->setCurrentLanguage(lang);
        env->ReleaseStringUTFChars(jlang, lang);
    }
}

static jobject GetCurSignalInfo(JNIEnv *env, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jobject signalInfo = NULL;
    if (Tv != NULL) {
        SignalInfo info = Tv->getCurSignalInfo();
        jclass objectClass = (env)->FindClass("com/droidlogic/app/tv/TvControlManager$SignalInfo");
        jmethodID consID = (env)->GetMethodID(objectClass, "<init>", "()V");
        jfieldID jfmt = (env)->GetFieldID(objectClass, "fmt", "I");
        jfieldID jtransFmt = (env)->GetFieldID(objectClass, "transFmt", "I");
        jfieldID jstatus = (env)->GetFieldID(objectClass, "status", "I");
        jfieldID jframeRate = (env)->GetFieldID(objectClass, "frameRate", "I");

        signalInfo = (env)->NewObject(objectClass, consID);
        (env)->SetIntField(signalInfo, jfmt, info.fmt);
        (env)->SetIntField(signalInfo, jtransFmt, info.transFmt);
        (env)->SetIntField(signalInfo, jstatus, info.status);
        (env)->SetIntField(signalInfo, jframeRate, info.frameRate);
    }
    return signalInfo;
}

static jint SetMiscCfg(JNIEnv *env, jclass clazz __unused, jstring jkey, jstring jval) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        const char *val = env->GetStringUTFChars(jval, nullptr);
        result = Tv->setMiscCfg(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseStringUTFChars(jval, val);
    }
    return result;
}

static jstring GetMiscCfg(JNIEnv *env, jclass clazz __unused, jstring jkey, jstring jdef) {
    std::string miscCfg;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    if (Tv != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        const char *def = env->GetStringUTFChars(jdef, nullptr);
        miscCfg = Tv->getMiscCfg(key, def);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseStringUTFChars(jdef, def);
    }
    return env->NewStringUTF(miscCfg.c_str());;
}

static jint StopTv(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        return Tv->stopTv();
    return result;
}

static jint StartTv(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        return Tv->startTv();
    return result;
}

static jint GetTvRunStatus(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getTvRunStatus();
    return result;
}

static jint GetTvAction(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getTvAction();
    return result;
}

static jint GetCurrentSourceInput(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getCurrentSourceInput();
    return result;
}

static jint GetCurrentVirtualSourceInput(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getCurrentVirtualSourceInput();
    return result;
}

static jint SetSourceInput(JNIEnv *env __unused, jclass clazz __unused, jint inputSrc) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setSourceInput(inputSrc);
    return result;
}

static jint SetSourceInputExt(JNIEnv *env __unused, jclass clazz __unused, jint inputSrc, jint vInputSrc) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setSourceInputExt(inputSrc, vInputSrc);
     return result;
}

static jint IsDviSIgnal(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->isDviSIgnal();
    return result;
}

static jint IsVgaTimingInHdmi(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->isVgaTimingInHdmi();
    return result;
}

static jint GetInputSrcConnectStatus(JNIEnv *env __unused, jclass clazz __unused, jint inputSrc) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getInputSrcConnectStatus(inputSrc);
    return result;
}

static jint SetHdmiEdidVersion(JNIEnv *env __unused, jclass clazz __unused, jint port_id, jint ver) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setHdmiEdidVersion(port_id, ver);
    return result;
}

static jint GetHdmiEdidVersion(JNIEnv *env __unused, jclass clazz __unused, jint port_id) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getHdmiEdidVersion(port_id);
    return result;
}

static jint SaveHdmiEdidVersion(JNIEnv *env __unused, jclass clazz __unused, jint port_id, jint ver) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->saveHdmiEdidVersion(port_id, ver);
    return result;
}

static jint SetHdmiColorRangeMode(JNIEnv *env __unused, jclass clazz __unused, jint range_mode) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setHdmiColorRangeMode(range_mode);
    return result;
}

static jint GetHdmiColorRangeMode(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getHdmiColorRangeMode();
    return result;
}

static jint SetAudioOutmode(JNIEnv *env __unused, jclass clazz __unused, jint mode) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setAudioOutmode(mode);
    return result;
}

static jint GetAudioOutmode(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getAudioOutmode();
    return result;
}

static jint GetAudioStreamOutmode(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getAudioStreamOutmode();
     return result;
}

static jint GetAtvAutoScanMode(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getAtvAutoScanMode();
    return result;
}

static jint SetAmAudioPreMute(JNIEnv *env __unused, jclass clazz __unused, jint mute) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setAmAudioPreMute(mute);
    return result;
}

static jint SSMInitDevice(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->SSMInitDevice();
    return result;
}

static jint SaveMacAddress(JNIEnv *env, jclass clazz __unused, jintArray data_buf) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL) {
        jint* jints = env->GetIntArrayElements(data_buf, NULL);
        int len = env->GetArrayLength(data_buf);
        char buf[len+1];
        memset(buf,0,len + 1);
        for (int i = 0; i < len; i++) {
            buf[i] = jints[i];
        }
        buf[len] = '\0';
        result = Tv->saveMacAddress(buf);
        env->ReleaseIntArrayElements(data_buf, jints, 0);
    }
    return result;
}

static jint ReadMacAddress(JNIEnv *env, jclass clazz __unused, jintArray jvalue) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL) {
        jint* jints = env->GetIntArrayElements(jvalue, NULL);
        int len = env->GetArrayLength(jvalue);
        char buf[len+1];
        memset(buf,0,len + 1);
        for (int i = 0; i < len; i++) {
            buf[i] = jints[i];
        }
        buf[len] = '\0';
        result = Tv->readMacAddress(buf);
        env->ReleaseIntArrayElements(jvalue, jints, 0);
    }
    return result;
}

static jint DtvScan(JNIEnv *env __unused, jclass clazz __unused, jint mode, jint scan_mode, jint beginFreq, jint endFreq, jint para1,
jint para2) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->dtvScan(mode, scan_mode, beginFreq, endFreq, para1, para2);
    return result;
}

static jint AtvAutoScan(JNIEnv *env __unused, jclass clazz __unused, jint videoStd, jint audioStd, jint searchType, jint procMode) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->atvAutoScan(videoStd, audioStd, searchType, procMode);
    return result;
}

static jint AtvManualScan(JNIEnv *env __unused, jclass clazz __unused, jint startFreq, jint endFreq, jint videoStd, jint audioStd) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->atvManualScan(startFreq, endFreq, videoStd, audioStd);
    return result;
}

static jint PauseScan(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->pauseScan();
    return result;
}

static jint ResumeScan(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->resumeScan();
    return result;
}

static jint OperateDeviceForScan(JNIEnv *env __unused, jclass clazz __unused, jint type) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->operateDeviceForScan(type);
    return result;
}

static jint AtvdtvGetScanStatus(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->atvdtvGetScanStatus();
    return result;
}

static jint SetDvbTextCoding(JNIEnv *env, jclass clazz __unused, jstring jcoding) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL) {
        const char *coding = env->GetStringUTFChars(jcoding, nullptr);
        result = Tv->setDvbTextCoding(coding);
        env->ReleaseStringUTFChars(jcoding, coding);
    }
    return result;
}

static jint SetBlackoutEnable(JNIEnv *env __unused, jclass clazz __unused, jint status, jint is_save) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setBlackoutEnable(status, is_save);
    return result;
}

static jint GetBlackoutEnable(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getBlackoutEnable();
    return result;
}

static jint GetATVMinMaxFreq(JNIEnv *env __unused, jclass clazz __unused, jint scanMinFreq, jint scanMaxFreq) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getATVMinMaxFreq(scanMinFreq, scanMaxFreq);
    return result;

}

static jobjectArray DtvGetScanFreqListMode(JNIEnv *env, jclass clazz __unused, jint mode) {
    std::vector<FreqList> freqlist;
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jobjectArray freqlistJni = NULL;
    jclass jniClass = (env)->FindClass("com/droidlogic/app/tv/TvControlManager$FreqList");
    if (Tv != NULL) {
        freqlist = Tv->dtvGetScanFreqListMode(mode);
    }
    jsize size = freqlist.size();
    if (size != 0) {
        freqlistJni = (env)->NewObjectArray(size, jniClass, NULL);
        jmethodID objectClassInitID = (env)->GetMethodID(jniClass, "<init>", "()V");
        jobject  objectFreqlist;
        jfieldID jID                     = (env)->GetFieldID(jniClass,"ID", "I");
        jfieldID jfreq                   = (env)->GetFieldID(jniClass, "freq", "I");
        jfieldID jchannelNum             = (env)->GetFieldID(jniClass, "channelNum", "I");
        jfieldID jphysicalNumDisplayName = (env)->GetFieldID(jniClass,"physicalNumDisplayName", "Ljava/lang/String;");

        for (int i = 0; i< size; i++) {
            objectFreqlist = (env)->NewObject(jniClass, objectClassInitID);
            (env)->SetIntField(objectFreqlist, jID, freqlist[i].ID);
            (env)->SetIntField(objectFreqlist, jfreq, freqlist[i].freq);
            (env)->SetIntField(objectFreqlist, jchannelNum, freqlist[i].channelNum);
            (env)->SetObjectField(objectFreqlist, jphysicalNumDisplayName, (env)->NewStringUTF(freqlist[i].physicalNumDisplayName.c_str()));
            (env)->SetObjectArrayElement(freqlistJni, i, objectFreqlist);
        }
    }
    return freqlistJni;
}

static jint UpdateRRT(JNIEnv *env __unused, jclass clazz __unused, jint freq, jint moudle, jint mode) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->updateRRT(freq, moudle, mode);
    return result;
}

static jobject SearchRrtInfo(JNIEnv *env, jclass clazz __unused, jint rating_region_id, jint dimension_id,
jint value_id, jint program_id) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jobject rrtInfo = NULL;
    if (Tv != NULL) {
        RRTSearchInfo info = Tv->searchRrtInfo(rating_region_id, dimension_id, value_id, program_id);
        jclass objectClass = (env)->FindClass("com/droidlogic/app/tv/TvControlManager$RrtSearchInfo");
        jmethodID consID = (env)->GetMethodID(objectClass, "<init>", "()V");
        jfieldID jrating_region_name = (env)->GetFieldID(objectClass, "rating_region_name", "Ljava/lang/String;");
        jfieldID jdimensions_name = (env)->GetFieldID(objectClass, "dimensions_name", "Ljava/lang/String;");
        jfieldID jrating_value_text = (env)->GetFieldID(objectClass, "rating_value_text", "Ljava/lang/String;");
        jfieldID jstatus = (env)->GetFieldID(objectClass, "status", "I");

        rrtInfo = (env)->NewObject(objectClass, consID);
        (env)->SetObjectField(rrtInfo, jrating_region_name, (env)->NewStringUTF(info.RatingRegionName.c_str()));
        (env)->SetObjectField(rrtInfo, jdimensions_name, (env)->NewStringUTF(info.DimensionsName.c_str()));
        (env)->SetObjectField(rrtInfo, jrating_value_text, (env)->NewStringUTF(info.RatingValueText.c_str()));
        (env)->SetIntField(rrtInfo, jstatus, info.status);
    }
    return rrtInfo;
}

static jint DtvStopScan(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->dtvStopScan();
    return result;
}

static jint DtvGetSignalStrength(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->dtvGetSignalStrength();
    return result;
}

static jint DtvSetAudioChannleMod(JNIEnv *env __unused, jclass clazz __unused, jint audioChannelIdx) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->dtvSetAudioChannleMod(audioChannelIdx);
    return result;
}

static jint DtvSwitchAudioTrack3(JNIEnv *env __unused, jclass clazz __unused, jint audio_pid, jint audio_format, jint audio_param) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->DtvSwitchAudioTrack3(audio_pid, audio_format,audio_param);
    return result;
}

static jint DtvSwitchAudioTrack(JNIEnv *env __unused, jclass clazz __unused, int prog_id, int audio_track_id) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->DtvSwitchAudioTrack(prog_id, audio_track_id);
    return result;
}

static jint DtvSetAudioAD(JNIEnv *env __unused, jclass clazz __unused, jint enable, jint audio_pid, jint audio_format) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->DtvSetAudioAD(enable, audio_pid, audio_format);
     return result;
}

static jobject DtvGetVideoFormatInfo(JNIEnv *env, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jobject formatInfo = NULL;
    if (Tv != NULL) {
        FormatInfo info = Tv->dtvGetVideoFormatInfo();
        jclass objectClass = (env)->FindClass("com/droidlogic/app/tv/TvControlManager$VideoFormatInfo");
        jmethodID consID = (env)->GetMethodID(objectClass, "<init>", "()V");

        jfieldID jwidth = (env)->GetFieldID(objectClass, "width", "I");
        jfieldID jheight = (env)->GetFieldID(objectClass, "height", "I");
        jfieldID jfps = (env)->GetFieldID(objectClass, "fps", "I");
        jfieldID jinterlace = (env)->GetFieldID(objectClass, "interlace", "I");
        formatInfo = (env)->NewObject(objectClass, consID);
        (env)->SetIntField(formatInfo, jwidth, info.width);
        (env)->SetIntField(formatInfo, jheight, info.height);
        (env)->SetIntField(formatInfo, jfps, info.fps);
        (env)->SetIntField(formatInfo, jinterlace, info.interlace);
    }
    return formatInfo;
}

static jint Scan(JNIEnv *env, jclass clazz __unused, jstring jfeparas, jstring jscanparas) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL) {
        const char *feparas = env->GetStringUTFChars(jfeparas, nullptr);
        const char *scanparas = env->GetStringUTFChars(jscanparas, nullptr);
        result = Tv->Scan(feparas, scanparas);
        env->ReleaseStringUTFChars(jfeparas, feparas);
        env->ReleaseStringUTFChars(jscanparas, scanparas);
    }
    return result;
}

static jint TvSetFrontEnd(JNIEnv *env, jclass clazz __unused, jstring jfeparas, jint force) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL) {
        const char *feparas = env->GetStringUTFChars(jfeparas, nullptr);
        result = Tv->tvSetFrontEnd(feparas, force);
        env->ReleaseStringUTFChars(jfeparas, feparas);
    }
    return result;
}

static jint TvSetFrontendParms(JNIEnv *env __unused, jclass clazz __unused, jint feType, jint freq, jint vStd, jint aStd, jint vfmt,
jint soundsys, jint p1, jint p2) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->tvSetFrontendParms(feType, freq, vStd, aStd, vfmt, soundsys, p1, p2);
     return result;
}

static jint HandleGPIO(JNIEnv *env, jclass clazz __unused, jstring jkey, jint is_out, jint edge) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        result = Tv->handleGPIO(key, is_out, edge);
        env->ReleaseStringUTFChars(jkey, key);
    }
    return result;
}

static jint SetLcdEnable(JNIEnv *env __unused, jclass clazz __unused, jint enable) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setLcdEnable(enable);
    return result;
}

static jint SendRecordingCmd(JNIEnv *env, jclass clazz __unused, jint cmd, jstring jid, jstring jparam) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL) {
        const char *id = env->GetStringUTFChars(jid, nullptr);
        const char *param = env->GetStringUTFChars(jparam, nullptr);
        result = Tv->sendRecordingCmd(cmd, id, param);
        env->ReleaseStringUTFChars(jid, id);
        env->ReleaseStringUTFChars(jparam, param);
    }
    return result;
}

static jint SendPlayCmd(JNIEnv *env, jclass clazz __unused, jint cmd, jstring jid, jstring jparam) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL) {
        const char *id = env->GetStringUTFChars(jid, nullptr);
        const char *param = env->GetStringUTFChars(jparam, nullptr);
        result = Tv->sendPlayCmd(cmd, id, param);
        env->ReleaseStringUTFChars(jid, id);
        env->ReleaseStringUTFChars(jparam, param);
    }
    return result;
}

static jint SetDeviceIdForCec(JNIEnv *env __unused, jclass clazz __unused, jint DeviceId) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setDeviceIdForCec(DeviceId);
    return result;
}

static jint GetIwattRegs(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->getIwattRegs();
    return result;
}

static jint SetSameSourceEnable(JNIEnv *env __unused, jclass clazz __unused, jint isEnable) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setSameSourceEnable(isEnable);
    return result;
}

static jint FactoryCleanAllTableForProgram(JNIEnv *env __unused, jclass clazz __unused) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->FactoryCleanAllTableForProgram();
    return result;
}

static jint SetPreviewWindow(JNIEnv *env __unused, jclass clazz __unused, jint x1, jint y1, jint x2, jint y2) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setPreviewWindow(x1, y1, x2, y2);
    return result;
}

static jint SetPreviewWindowMode(JNIEnv *env __unused, jclass clazz __unused, jint enable) {
    const sp<TvServerHidlClient>& Tv = getTvClient();
    jint result = -1;
    if (Tv != NULL)
        result = Tv->setPreviewWindowMode(enable);
    return result;
}

static jint GetCecWakePort(JNIEnv *env __unused, jclass clazz __unused) {
    using ::vendor::amlogic::hardware::hdmicec::V1_0::IDroidHdmiCEC;
    ALOGD("GetCecWakePort");
    jint result = -1;
    sp<IDroidHdmiCEC> hdmicec = IDroidHdmiCEC::tryGetService();
    while (hdmicec == nullptr) {
         usleep(200*1000);//sleep 200ms
         hdmicec = IDroidHdmiCEC::tryGetService();
         ALOGE("tryGet hdmicecd daemon Service");
    };
    if (hdmicec != nullptr) {
        result = hdmicec->getCecWakePort();
        ALOGD("GetCecWakePort %d", result);
    }
    return result;
}

static JNINativeMethod Tv_Methods[] = {
{"native_ConnectTvServer", "(Lcom/droidlogic/app/tv/TvControlManager;)V", (void *) ConnectTvServer },
{"native_DisConnectTvServer", "()V", (void *) DisConnectTvServer },
{"native_GetSupportInputDevices", "()Ljava/lang/String;", (void *) GetSupportInputDevices },
{"native_GetTvSupportCountries", "()Ljava/lang/String;", (void *) GetTvSupportCountries },
{"native_GetTvDefaultCountry", "()Ljava/lang/String;", (void *) GetTvDefaultCountry },
{"native_GetTvCountryName", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetTvCountryName },
{"native_GetTvSearchMode", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetTvSearchMode },
{"native_GetTvDtvSupport", "(Ljava/lang/String;)Z", (void *) GetTvDtvSupport },
{"native_GetTvDtvSystem", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetTvDtvSystem },
{"native_GetTvAtvSupport", "(Ljava/lang/String;)Z", (void *) GetTvAtvSupport },
{"native_GetTvAtvColorSystem", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetTvAtvColorSystem },
{"native_GetTvAtvSoundSystem", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetTvAtvSoundSystem },
{"native_GetTvAtvMinMaxFreq", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetTvAtvMinMaxFreq },
{"native_GetTvAtvStepScan", "(Ljava/lang/String;)Z", (void *) GetTvAtvStepScan },
{"native_SetTvCountry", "(Ljava/lang/String;)V", (void *) SetTvCountry },
{"native_SetCurrentLanguage", "(Ljava/lang/String;)V", (void *) SetCurrentLanguage },
{"native_GetCurSignalInfo", "()Lcom/droidlogic/app/tv/TvControlManager$SignalInfo;", (void *) GetCurSignalInfo },
{"native_SetMiscCfg", "(Ljava/lang/String;Ljava/lang/String;)I", (void *) SetMiscCfg },
{"native_GetMiscCfg", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void *) GetMiscCfg },
{"native_StopTv", "()I", (void *) StopTv },
{"native_StartTv", "()I", (void *) StartTv },
{"native_GetTvRunStatus", "()I", (void *) GetTvRunStatus },
{"native_GetTvAction", "()I", (void *) GetTvAction },
{"native_GetCurrentSourceInput", "()I", (void *) GetCurrentSourceInput },
{"native_GetCurrentVirtualSourceInput", "()I", (void *) GetCurrentVirtualSourceInput },
{"native_SetSourceInput", "(I)I", (void *) SetSourceInput },
{"native_SetSourceInputExt", "(II)I", (void *) SetSourceInputExt },
{"native_IsDviSIgnal", "()I", (void *) IsDviSIgnal },
{"native_IsVgaTimingInHdmi", "()I", (void *) IsVgaTimingInHdmi },
{"native_GetInputSrcConnectStatus", "(I)I", (void *) GetInputSrcConnectStatus },
{"native_SetHdmiEdidVersion", "(II)I", (void *) SetHdmiEdidVersion },
{"native_GetHdmiEdidVersion", "(I)I", (void *) GetHdmiEdidVersion },
{"native_SaveHdmiEdidVersion", "(II)I", (void *) SaveHdmiEdidVersion },
{"native_SetHdmiColorRangeMode", "(I)I", (void *) SetHdmiColorRangeMode },
{"native_GetHdmiColorRangeMode", "()I", (void *) GetHdmiColorRangeMode },
{"native_SetAudioOutmode", "(I)I", (void *) SetAudioOutmode },
{"native_GetAudioOutmode", "()I", (void *) GetAudioOutmode },
{"native_GetAudioStreamOutmode", "()I", (void *) GetAudioStreamOutmode },
{"native_GetAtvAutoScanMode", "()I", (void *) GetAtvAutoScanMode },
{"native_SetAmAudioPreMute", "(I)I", (void *) SetAmAudioPreMute },
{"native_SSMInitDevice", "()I", (void *) SSMInitDevice },
{"native_SaveMacAddress", "([I)I", (void *) SaveMacAddress },
{"native_ReadMacAddress", "([I)I", (void *) ReadMacAddress },
{"native_DtvScan", "(IIIIII)I", (void *) DtvScan },
{"native_AtvAutoScan", "(IIII)I", (void *) AtvAutoScan },
{"native_AtvManualScan", "(IIII)I", (void *) AtvManualScan },
{"native_PauseScan", "()I", (void *) PauseScan },
{"native_ResumeScan", "()I", (void *) ResumeScan },
{"native_OperateDeviceForScan", "(I)I", (void *) OperateDeviceForScan },
{"native_AtvdtvGetScanStatus", "()I", (void *) AtvdtvGetScanStatus },
{"native_SetDvbTextCoding", "(Ljava/lang/String;)I", (void *) SetDvbTextCoding },
{"native_SetBlackoutEnable", "(II)I", (void *) SetBlackoutEnable },
{"native_GetBlackoutEnable", "()I", (void *) GetBlackoutEnable },
{"native_GetATVMinMaxFreq", "(II)I", (void *) GetATVMinMaxFreq },
{"native_UpdateRRT", "(III)I", (void *) UpdateRRT },
{"native_DtvStopScan", "()I", (void *) DtvStopScan },
{"native_DtvGetSignalStrength", "()I", (void *) DtvGetSignalStrength },
{"native_DtvSetAudioChannleMod", "(I)I", (void *) DtvSetAudioChannleMod },
{"native_DtvSwitchAudioTrack3", "(III)I", (void *) DtvSwitchAudioTrack3 },
{"native_DtvSwitchAudioTrack", "(II)I", (void *) DtvSwitchAudioTrack },
{"native_DtvSetAudioAD", "(III)I", (void *) DtvSetAudioAD },
{"native_Scan", "(Ljava/lang/String;Ljava/lang/String;)I", (void *) Scan },
{"native_TvSetFrontEnd", "(Ljava/lang/String;I)I", (void *) TvSetFrontEnd },
{"native_TvSetFrontendParms", "(IIIIIIII)I", (void *) TvSetFrontendParms },
{"native_HandleGPIO", "(Ljava/lang/String;II)I", (void *) HandleGPIO },
{"native_SetLcdEnable", "(I)I", (void *) SetLcdEnable },
{"native_SendRecordingCmd", "(ILjava/lang/String;Ljava/lang/String;)I", (void *) SendRecordingCmd },
{"native_SendPlayCmd", "(ILjava/lang/String;Ljava/lang/String;)I", (void *) SendPlayCmd },
{"native_SetDeviceIdForCec", "(I)I", (void *) SetDeviceIdForCec },
{"native_GetIwattRegs", "()I", (void *) GetIwattRegs },
{"native_SetSameSourceEnable", "(I)I", (void *) SetSameSourceEnable },
{"native_FactoryCleanAllTableForProgram", "()I", (void *) FactoryCleanAllTableForProgram },
{"native_DtvGetVideoFormatInfo", "()Lcom/droidlogic/app/tv/TvControlManager$VideoFormatInfo;", (void *) DtvGetVideoFormatInfo },
{"native_SearchRrtInfo", "(IIII)Lcom/droidlogic/app/tv/TvControlManager$RrtSearchInfo;", (void *) SearchRrtInfo },
{"native_DtvGetScanFreqListMode", "(I)[Lcom/droidlogic/app/tv/TvControlManager$FreqList;", (void *) DtvGetScanFreqListMode },
{"native_SetPreviewWindow", "(IIII)I", (void *) SetPreviewWindow },
{"native_SetPreviewWindowMode", "(I)I", (void *) SetPreviewWindowMode },
{"native_GetCecWakePort", "()I", (void *) GetCecWakePort },

};

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

int register_com_droidlogic_app_tv_TvControlManager(JNIEnv *env)
{
    static const char *const kClassPathName = "com/droidlogic/app/tv/TvControlManager";
    jclass clazz;
    int rc;
    FIND_CLASS(clazz, kClassPathName);

    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'\n", kClassPathName);
        return -1;
    }

    rc = (env->RegisterNatives(clazz, Tv_Methods, NELEM(Tv_Methods)));
    if (rc < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s' %d\n", kClassPathName, rc);
        return -1;
    }

    GET_METHOD_ID(notifyCallback, clazz, "notifyCallback", "(Lcom/droidlogic/app/tv/TvControlManager$TvHidlParcel;)V");
    return rc;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved __unused)
{
    ALOGD("bbb");
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        ALOGI("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);
    gJavaVM = vm;

    if (register_com_droidlogic_app_tv_TvControlManager(env) < 0)
    {
        ALOGE("Can't register TvControlManager");
        goto bail;
    }

    static const char *const kClassPathName   = "com/droidlogic/app/tv/TvControlManager$TvHidlParcel";

    FIND_CLASS(notifyClazz, kClassPathName);
    notifyClazz    = (jclass)env->NewGlobalRef(notifyClazz);

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}


