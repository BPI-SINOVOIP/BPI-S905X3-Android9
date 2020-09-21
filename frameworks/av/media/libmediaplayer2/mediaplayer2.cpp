/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaPlayer2Native"

#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include <media/AudioSystem.h>
#include <media/DataSourceDesc.h>
#include <media/MediaAnalyticsItem.h>
#include <media/MemoryLeakTrackUtil.h>
#include <media/Metadata.h>
#include <media/NdkWrapper.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooperRoster.h>
#include <mediaplayer2/MediaPlayer2AudioOutput.h>
#include <mediaplayer2/mediaplayer2.h>

#include <utils/Log.h>
#include <utils/SortedVector.h>
#include <utils/String8.h>

#include <system/audio.h>
#include <system/window.h>

#include <nuplayer2/NuPlayer2Driver.h>

#include <dirent.h>
#include <sys/stat.h>

namespace android {

extern ALooperRoster gLooperRoster;

namespace {

const int kDumpLockRetries = 50;
const int kDumpLockSleepUs = 20000;

// Max number of entries in the filter.
const int kMaxFilterSize = 64;  // I pulled that out of thin air.

// FIXME: Move all the metadata related function in the Metadata.cpp

// Unmarshall a filter from a Parcel.
// Filter format in a parcel:
//
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       number of entries (n)                   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       metadata type 1                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       metadata type 2                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  ....
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       metadata type n                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// @param p Parcel that should start with a filter.
// @param[out] filter On exit contains the list of metadata type to be
//                    filtered.
// @param[out] status On exit contains the status code to be returned.
// @return true if the parcel starts with a valid filter.
bool unmarshallFilter(const Parcel& p,
                      media::Metadata::Filter *filter,
                      status_t *status) {
    int32_t val;
    if (p.readInt32(&val) != OK) {
        ALOGE("Failed to read filter's length");
        *status = NOT_ENOUGH_DATA;
        return false;
    }

    if (val > kMaxFilterSize || val < 0) {
        ALOGE("Invalid filter len %d", val);
        *status = BAD_VALUE;
        return false;
    }

    const size_t num = val;

    filter->clear();
    filter->setCapacity(num);

    size_t size = num * sizeof(media::Metadata::Type);


    if (p.dataAvail() < size) {
        ALOGE("Filter too short expected %zu but got %zu", size, p.dataAvail());
        *status = NOT_ENOUGH_DATA;
        return false;
    }

    const media::Metadata::Type *data =
        static_cast<const media::Metadata::Type*>(p.readInplace(size));

    if (NULL == data) {
        ALOGE("Filter had no data");
        *status = BAD_VALUE;
        return false;
    }

    // TODO: The stl impl of vector would be more efficient here
    // because it degenerates into a memcpy on pod types. Try to
    // replace later or use stl::set.
    for (size_t i = 0; i < num; ++i) {
        filter->add(*data);
        ++data;
    }
    *status = OK;
    return true;
}

// @param filter Of metadata type.
// @param val To be searched.
// @return true if a match was found.
bool findMetadata(const media::Metadata::Filter& filter, const int32_t val) {
    // Deal with empty and ANY right away
    if (filter.isEmpty()) {
        return false;
    }
    if (filter[0] == media::Metadata::kAny) {
        return true;
    }

    return filter.indexOf(val) >= 0;
}

// marshalling tag indicating flattened utf16 tags
// keep in sync with frameworks/base/media/java/android/media/AudioAttributes.java
const int32_t kAudioAttributesMarshallTagFlattenTags = 1;

// Audio attributes format in a parcel:
//
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       usage                                   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       content_type                            |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       source                                  |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       flags                                   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       kAudioAttributesMarshallTagFlattenTags  | // ignore tags if not found
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       flattened tags in UTF16                 |
// |                         ...                                   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// @param p Parcel that contains audio attributes.
// @param[out] attributes On exit points to an initialized audio_attributes_t structure
// @param[out] status On exit contains the status code to be returned.
void unmarshallAudioAttributes(const Parcel& parcel, audio_attributes_t *attributes) {
    attributes->usage = (audio_usage_t) parcel.readInt32();
    attributes->content_type = (audio_content_type_t) parcel.readInt32();
    attributes->source = (audio_source_t) parcel.readInt32();
    attributes->flags = (audio_flags_mask_t) parcel.readInt32();
    const bool hasFlattenedTag = (parcel.readInt32() == kAudioAttributesMarshallTagFlattenTags);
    if (hasFlattenedTag) {
        // the tags are UTF16, convert to UTF8
        String16 tags = parcel.readString16();
        ssize_t realTagSize = utf16_to_utf8_length(tags.string(), tags.size());
        if (realTagSize <= 0) {
            strcpy(attributes->tags, "");
        } else {
            // copy the flattened string into the attributes as the destination for the conversion:
            // copying array size -1, array for tags was calloc'd, no need to NULL-terminate it
            size_t tagSize = realTagSize > AUDIO_ATTRIBUTES_TAGS_MAX_SIZE - 1 ?
                    AUDIO_ATTRIBUTES_TAGS_MAX_SIZE - 1 : realTagSize;
            utf16_to_utf8(tags.string(), tagSize, attributes->tags,
                    sizeof(attributes->tags) / sizeof(attributes->tags[0]));
        }
    } else {
        ALOGE("unmarshallAudioAttributes() received unflattened tags, ignoring tag values");
        strcpy(attributes->tags, "");
    }
}

class AudioDeviceUpdatedNotifier: public AudioSystem::AudioDeviceCallback {
public:
    AudioDeviceUpdatedNotifier(const sp<MediaPlayer2Interface>& listener)
        : mListener(listener) { }

    ~AudioDeviceUpdatedNotifier() { }

    virtual void onAudioDeviceUpdate(audio_io_handle_t audioIo,
                                     audio_port_handle_t deviceId) override {
        sp<MediaPlayer2Interface> listener = mListener.promote();
        if (listener != NULL) {
            listener->sendEvent(0, MEDIA2_AUDIO_ROUTING_CHANGED, audioIo, deviceId);
        } else {
            ALOGW("listener for process %d death is gone", MEDIA2_AUDIO_ROUTING_CHANGED);
        }
    }

private:
    wp<MediaPlayer2Interface> mListener;
};

class proxyListener : public MediaPlayer2InterfaceListener {
public:
    proxyListener(const wp<MediaPlayer2> &player)
        : mPlayer(player) { }

    ~proxyListener() { };

    virtual void notify(int64_t srcId, int msg, int ext1, int ext2, const Parcel *obj) override {
        sp<MediaPlayer2> player = mPlayer.promote();
        if (player != NULL) {
            player->notify(srcId, msg, ext1, ext2, obj);
        }
    }

private:
    wp<MediaPlayer2> mPlayer;
};

Mutex sRecordLock;
SortedVector<wp<MediaPlayer2> > *sPlayers;

void ensureInit_l() {
    if (sPlayers == NULL) {
        sPlayers = new SortedVector<wp<MediaPlayer2> >();
    }
}

void addPlayer(const wp<MediaPlayer2>& player) {
    Mutex::Autolock lock(sRecordLock);
    ensureInit_l();
    sPlayers->add(player);
}

void removePlayer(const wp<MediaPlayer2>& player) {
    Mutex::Autolock lock(sRecordLock);
    ensureInit_l();
    sPlayers->remove(player);
}

/**
 * The only arguments this understands right now are -c, -von and -voff,
 * which are parsed by ALooperRoster::dump()
 */
status_t dumpPlayers(int fd, const Vector<String16>& args) {
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    SortedVector< sp<MediaPlayer2> > players; //to serialise the mutex unlock & client destruction.

    if (checkCallingPermission(String16("android.permission.DUMP")) == false) {
        snprintf(buffer, SIZE, "Permission Denial: can't dump MediaPlayer2\n");
        result.append(buffer);
    } else {
        {
            Mutex::Autolock lock(sRecordLock);
            ensureInit_l();
            for (int i = 0, n = sPlayers->size(); i < n; ++i) {
                sp<MediaPlayer2> p = (*sPlayers)[i].promote();
                if (p != 0) {
                    p->dump(fd, args);
                }
                players.add(p);
            }
        }

        result.append(" Files opened and/or mapped:\n");
        snprintf(buffer, SIZE, "/proc/%d/maps", getpid());
        FILE *f = fopen(buffer, "r");
        if (f) {
            while (!feof(f)) {
                fgets(buffer, SIZE, f);
                if (strstr(buffer, " /storage/") ||
                    strstr(buffer, " /system/sounds/") ||
                    strstr(buffer, " /data/") ||
                    strstr(buffer, " /system/media/")) {
                    result.append("  ");
                    result.append(buffer);
                }
            }
            fclose(f);
        } else {
            result.append("couldn't open ");
            result.append(buffer);
            result.append("\n");
        }

        snprintf(buffer, SIZE, "/proc/%d/fd", getpid());
        DIR *d = opendir(buffer);
        if (d) {
            struct dirent *ent;
            while((ent = readdir(d)) != NULL) {
                if (strcmp(ent->d_name,".") && strcmp(ent->d_name,"..")) {
                    snprintf(buffer, SIZE, "/proc/%d/fd/%s", getpid(), ent->d_name);
                    struct stat s;
                    if (lstat(buffer, &s) == 0) {
                        if ((s.st_mode & S_IFMT) == S_IFLNK) {
                            char linkto[256];
                            int len = readlink(buffer, linkto, sizeof(linkto));
                            if(len > 0) {
                                if(len > 255) {
                                    linkto[252] = '.';
                                    linkto[253] = '.';
                                    linkto[254] = '.';
                                    linkto[255] = 0;
                                } else {
                                    linkto[len] = 0;
                                }
                                if (strstr(linkto, "/storage/") == linkto ||
                                    strstr(linkto, "/system/sounds/") == linkto ||
                                    strstr(linkto, "/data/") == linkto ||
                                    strstr(linkto, "/system/media/") == linkto) {
                                    result.append("  ");
                                    result.append(buffer);
                                    result.append(" -> ");
                                    result.append(linkto);
                                    result.append("\n");
                                }
                            }
                        } else {
                            result.append("  unexpected type for ");
                            result.append(buffer);
                            result.append("\n");
                        }
                    }
                }
            }
            closedir(d);
        } else {
            result.append("couldn't open ");
            result.append(buffer);
            result.append("\n");
        }

        gLooperRoster.dump(fd, args);

        bool dumpMem = false;
        bool unreachableMemory = false;
        for (size_t i = 0; i < args.size(); i++) {
            if (args[i] == String16("-m")) {
                dumpMem = true;
            } else if (args[i] == String16("--unreachable")) {
                unreachableMemory = true;
            }
        }
        if (dumpMem) {
            result.append("\nDumping memory:\n");
            std::string s = dumpMemoryAddresses(100 /* limit */);
            result.append(s.c_str(), s.size());
        }
        if (unreachableMemory) {
            result.append("\nDumping unreachable memory:\n");
            // TODO - should limit be an argument parameter?
            // TODO: enable GetUnreachableMemoryString if it's part of stable API
            //std::string s = GetUnreachableMemoryString(true /* contents */, 10000 /* limit */);
            //result.append(s.c_str(), s.size());
        }
    }
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

}  // anonymous namespace

//static
sp<MediaPlayer2> MediaPlayer2::Create() {
    sp<MediaPlayer2> player = new MediaPlayer2();

    if (!player->init()) {
        return NULL;
    }

    ALOGV("Create new player(%p)", player.get());

    addPlayer(player);
    return player;
}

// static
status_t MediaPlayer2::DumpAll(int fd, const Vector<String16>& args) {
    return dumpPlayers(fd, args);
}

MediaPlayer2::MediaPlayer2() {
    ALOGV("constructor");
    mSrcId = 0;
    mLockThreadId = 0;
    mListener = NULL;
    mStreamType = AUDIO_STREAM_MUSIC;
    mAudioAttributesParcel = NULL;
    mCurrentPosition = -1;
    mCurrentSeekMode = MediaPlayer2SeekMode::SEEK_PREVIOUS_SYNC;
    mSeekPosition = -1;
    mSeekMode = MediaPlayer2SeekMode::SEEK_PREVIOUS_SYNC;
    mCurrentState = MEDIA_PLAYER2_IDLE;
    mLoop = false;
    mLeftVolume = mRightVolume = 1.0;
    mVideoWidth = mVideoHeight = 0;
    mAudioSessionId = (audio_session_t) AudioSystem::newAudioUniqueId(AUDIO_UNIQUE_ID_USE_SESSION);
    AudioSystem::acquireAudioSessionId(mAudioSessionId, -1);
    mSendLevel = 0;

    // TODO: get pid and uid from JAVA
    mPid = IPCThreadState::self()->getCallingPid();
    mUid = IPCThreadState::self()->getCallingUid();

    mAudioAttributes = NULL;
}

MediaPlayer2::~MediaPlayer2() {
    ALOGV("destructor");
    if (mAudioAttributesParcel != NULL) {
        delete mAudioAttributesParcel;
        mAudioAttributesParcel = NULL;
    }
    AudioSystem::releaseAudioSessionId(mAudioSessionId, -1);
    disconnect();
    removePlayer(this);
    if (mAudioAttributes != NULL) {
        free(mAudioAttributes);
    }
}

bool MediaPlayer2::init() {
    // TODO: after merge with NuPlayer2Driver, MediaPlayer2 will have its own
    // looper for notification.
    return true;
}

void MediaPlayer2::disconnect() {
    ALOGV("disconnect");
    sp<MediaPlayer2Interface> p;
    {
        Mutex::Autolock _l(mLock);
        p = mPlayer;
        mPlayer.clear();
    }

    if (p != 0) {
        p->setListener(NULL);
        p->reset();
    }

    {
        Mutex::Autolock _l(mLock);
        disconnectNativeWindow_l();
    }
}

void MediaPlayer2::clear_l() {
    mCurrentPosition = -1;
    mCurrentSeekMode = MediaPlayer2SeekMode::SEEK_PREVIOUS_SYNC;
    mSeekPosition = -1;
    mSeekMode = MediaPlayer2SeekMode::SEEK_PREVIOUS_SYNC;
    mVideoWidth = mVideoHeight = 0;
}

status_t MediaPlayer2::setListener(const sp<MediaPlayer2Listener>& listener) {
    ALOGV("setListener");
    Mutex::Autolock _l(mLock);
    mListener = listener;
    return NO_ERROR;
}

status_t MediaPlayer2::getSrcId(int64_t *srcId) {
    if (srcId == NULL) {
        return BAD_VALUE;
    }

    Mutex::Autolock _l(mLock);
    *srcId = mSrcId;
    return OK;
}

status_t MediaPlayer2::setDataSource(const sp<DataSourceDesc> &dsd) {
    if (dsd == NULL) {
        return BAD_VALUE;
    }
    ALOGV("setDataSource type(%d), srcId(%lld)", dsd->mType, (long long)dsd->mId);

    sp<MediaPlayer2Interface> oldPlayer;

    Mutex::Autolock _l(mLock);
    {
        if (!((mCurrentState & MEDIA_PLAYER2_IDLE)
              || mCurrentState == MEDIA_PLAYER2_STATE_ERROR)) {
            ALOGE("setDataSource called in wrong state %d", mCurrentState);
            return INVALID_OPERATION;
        }

        sp<MediaPlayer2Interface> player = new NuPlayer2Driver(mPid, mUid);
        status_t err = player->initCheck();
        if (err != NO_ERROR) {
            ALOGE("Failed to create player object, initCheck failed(%d)", err);
            return err;
        }

        clear_l();

        player->setListener(new proxyListener(this));
        mAudioOutput = new MediaPlayer2AudioOutput(mAudioSessionId, mUid,
                mPid, mAudioAttributes, new AudioDeviceUpdatedNotifier(player));
        player->setAudioSink(mAudioOutput);

        err = player->setDataSource(dsd);
        if (err != OK) {
            ALOGE("setDataSource error: %d", err);
            return err;
        }

        sp<MediaPlayer2Interface> oldPlayer = mPlayer;
        mPlayer = player;
        mSrcId = dsd->mId;
        mCurrentState = MEDIA_PLAYER2_INITIALIZED;
    }

    if (oldPlayer != NULL) {
        oldPlayer->setListener(NULL);
        oldPlayer->reset();
    }

    return OK;
}

status_t MediaPlayer2::prepareNextDataSource(const sp<DataSourceDesc> &dsd) {
    if (dsd == NULL) {
        return BAD_VALUE;
    }
    ALOGV("prepareNextDataSource type(%d), srcId(%lld)", dsd->mType, (long long)dsd->mId);

    Mutex::Autolock _l(mLock);
    if (mPlayer == NULL) {
        ALOGE("prepareNextDataSource failed: state %X, mPlayer(%p)", mCurrentState, mPlayer.get());
        return INVALID_OPERATION;
    }
    return mPlayer->prepareNextDataSource(dsd);
}

status_t MediaPlayer2::playNextDataSource(int64_t srcId) {
    ALOGV("playNextDataSource srcId(%lld)", (long long)srcId);

    Mutex::Autolock _l(mLock);
    if (mPlayer == NULL) {
        ALOGE("playNextDataSource failed: state %X, mPlayer(%p)", mCurrentState, mPlayer.get());
        return INVALID_OPERATION;
    }
    mSrcId = srcId;
    return mPlayer->playNextDataSource(srcId);
}

status_t MediaPlayer2::invoke(const Parcel& request, Parcel *reply) {
    Mutex::Autolock _l(mLock);
    const bool hasBeenInitialized =
            (mCurrentState != MEDIA_PLAYER2_STATE_ERROR) &&
            ((mCurrentState & MEDIA_PLAYER2_IDLE) != MEDIA_PLAYER2_IDLE);
    if ((mPlayer == NULL) || !hasBeenInitialized) {
        ALOGE("invoke failed: wrong state %X, mPlayer(%p)", mCurrentState, mPlayer.get());
        return INVALID_OPERATION;
    }
    ALOGV("invoke %zu", request.dataSize());
    return mPlayer->invoke(request, reply);
}

// This call doesn't need to access the native player.
status_t MediaPlayer2::setMetadataFilter(const Parcel& filter) {
    ALOGD("setMetadataFilter");

    status_t status;
    media::Metadata::Filter allow, drop;

    if (unmarshallFilter(filter, &allow, &status) &&
        unmarshallFilter(filter, &drop, &status)) {
        Mutex::Autolock lock(mLock);

        mMetadataAllow = allow;
        mMetadataDrop = drop;
    }
    return status;
}

status_t MediaPlayer2::getMetadata(bool update_only, bool /* apply_filter */, Parcel *reply) {
    ALOGD("getMetadata");
    sp<MediaPlayer2Interface> player;
    media::Metadata::Filter ids;
    Mutex::Autolock lock(mLock);
    {
        if (mPlayer == NULL) {
            return NO_INIT;
        }

        player = mPlayer;
        // Placeholder for the return code, updated by the caller.
        reply->writeInt32(-1);

        // We don't block notifications while we fetch the data. We clear
        // mMetadataUpdated first so we don't lose notifications happening
        // during the rest of this call.
        if (update_only) {
            ids = mMetadataUpdated;
        }
        mMetadataUpdated.clear();
    }

    media::Metadata metadata(reply);

    metadata.appendHeader();
    status_t status = player->getMetadata(ids, reply);

    if (status != OK) {
        metadata.resetParcel();
        ALOGE("getMetadata failed %d", status);
        return status;
    }

    // FIXME: ement filtering on the result. Not critical since
    // filtering takes place on the update notifications already. This
    // would be when all the metadata are fetch and a filter is set.

    // Everything is fine, update the metadata length.
    metadata.updateLength();
    return OK;
}

void MediaPlayer2::disconnectNativeWindow_l() {
    if (mConnectedWindow != NULL && mConnectedWindow->getANativeWindow() != NULL) {
        status_t err = native_window_api_disconnect(
                mConnectedWindow->getANativeWindow(), NATIVE_WINDOW_API_MEDIA);

        if (err != OK) {
            ALOGW("nativeWindowDisconnect returned an error: %s (%d)",
                  strerror(-err), err);
        }
    }
    mConnectedWindow.clear();
}

status_t MediaPlayer2::setVideoSurfaceTexture(const sp<ANativeWindowWrapper>& nww) {
    ANativeWindow *anw = (nww == NULL ? NULL : nww->getANativeWindow());
    ALOGV("setVideoSurfaceTexture(%p)", anw);
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) {
        return NO_INIT;
    }

    if (anw != NULL) {
        if (mConnectedWindow != NULL
            && mConnectedWindow->getANativeWindow() == anw) {
            return OK;
        }
        status_t err = native_window_api_connect(anw, NATIVE_WINDOW_API_MEDIA);

        if (err != OK) {
            ALOGE("setVideoSurfaceTexture failed: %d", err);
            // Note that we must do the reset before disconnecting from the ANW.
            // Otherwise queue/dequeue calls could be made on the disconnected
            // ANW, which may result in errors.
            mPlayer->reset();
            disconnectNativeWindow_l();
            return err;
        }
    }

    // Note that we must set the player's new GraphicBufferProducer before
    // disconnecting the old one.  Otherwise queue/dequeue calls could be made
    // on the disconnected ANW, which may result in errors.
    status_t err = mPlayer->setVideoSurfaceTexture(nww);

    disconnectNativeWindow_l();

    if (err == OK) {
        mConnectedWindow = nww;
        mLock.unlock();
    } else if (anw != NULL) {
        mLock.unlock();
        status_t err = native_window_api_disconnect(anw, NATIVE_WINDOW_API_MEDIA);

        if (err != OK) {
            ALOGW("nativeWindowDisconnect returned an error: %s (%d)",
                  strerror(-err), err);
        }
    }

    return err;
}

status_t MediaPlayer2::getBufferingSettings(BufferingSettings* buffering /* nonnull */) {
    ALOGV("getBufferingSettings");

    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) {
        return NO_INIT;
    }

    status_t ret = mPlayer->getBufferingSettings(buffering);
    if (ret == NO_ERROR) {
        ALOGV("getBufferingSettings{%s}", buffering->toString().string());
    } else {
        ALOGE("getBufferingSettings returned %d", ret);
    }
    return ret;
}

status_t MediaPlayer2::setBufferingSettings(const BufferingSettings& buffering) {
    ALOGV("setBufferingSettings{%s}", buffering.toString().string());

    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) {
        return NO_INIT;
    }
    return mPlayer->setBufferingSettings(buffering);
}

status_t MediaPlayer2::setAudioAttributes_l(const Parcel &parcel) {
    if (mAudioAttributes != NULL) {
        free(mAudioAttributes);
    }
    mAudioAttributes = (audio_attributes_t *) calloc(1, sizeof(audio_attributes_t));
    if (mAudioAttributes == NULL) {
        return NO_MEMORY;
    }
    unmarshallAudioAttributes(parcel, mAudioAttributes);

    ALOGV("setAudioAttributes_l() usage=%d content=%d flags=0x%x tags=%s",
            mAudioAttributes->usage, mAudioAttributes->content_type, mAudioAttributes->flags,
            mAudioAttributes->tags);

    if (mAudioOutput != 0) {
        mAudioOutput->setAudioAttributes(mAudioAttributes);
    }
    return NO_ERROR;
}

status_t MediaPlayer2::prepareAsync() {
    ALOGV("prepareAsync");
    Mutex::Autolock _l(mLock);
    if ((mPlayer != 0) && (mCurrentState & (MEDIA_PLAYER2_INITIALIZED | MEDIA_PLAYER2_STOPPED))) {
        if (mAudioAttributesParcel != NULL) {
            status_t err = setAudioAttributes_l(*mAudioAttributesParcel);
            if (err != OK) {
                return err;
            }
        } else if (mAudioOutput != 0) {
            mAudioOutput->setAudioStreamType(mStreamType);
        }
        mCurrentState = MEDIA_PLAYER2_PREPARING;
        return mPlayer->prepareAsync();
    }
    ALOGE("prepareAsync called in state %d, mPlayer(%p)", mCurrentState, mPlayer.get());
    return INVALID_OPERATION;
}

status_t MediaPlayer2::start() {
    ALOGV("start");

    status_t ret = NO_ERROR;
    Mutex::Autolock _l(mLock);

    mLockThreadId = getThreadId();

    if (mCurrentState & MEDIA_PLAYER2_STARTED) {
        ret = NO_ERROR;
    } else if ( (mPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER2_PREPARED |
                    MEDIA_PLAYER2_PLAYBACK_COMPLETE | MEDIA_PLAYER2_PAUSED ) ) ) {
        mPlayer->setLooping(mLoop);

        if (mAudioOutput != 0) {
            mAudioOutput->setVolume(mLeftVolume, mRightVolume);
        }

        if (mAudioOutput != 0) {
            mAudioOutput->setAuxEffectSendLevel(mSendLevel);
        }
        mCurrentState = MEDIA_PLAYER2_STARTED;
        ret = mPlayer->start();
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER2_STATE_ERROR;
        } else {
            if (mCurrentState == MEDIA_PLAYER2_PLAYBACK_COMPLETE) {
                ALOGV("playback completed immediately following start()");
            }
        }
    } else {
        ALOGE("start called in state %d, mPlayer(%p)", mCurrentState, mPlayer.get());
        ret = INVALID_OPERATION;
    }

    mLockThreadId = 0;

    return ret;
}

status_t MediaPlayer2::stop() {
    ALOGV("stop");
    Mutex::Autolock _l(mLock);
    if (mCurrentState & MEDIA_PLAYER2_STOPPED) return NO_ERROR;
    if ( (mPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER2_STARTED | MEDIA_PLAYER2_PREPARED |
                    MEDIA_PLAYER2_PAUSED | MEDIA_PLAYER2_PLAYBACK_COMPLETE ) ) ) {
        status_t ret = mPlayer->stop();
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER2_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER2_STOPPED;
        }
        return ret;
    }
    ALOGE("stop called in state %d, mPlayer(%p)", mCurrentState, mPlayer.get());
    return INVALID_OPERATION;
}

status_t MediaPlayer2::pause() {
    ALOGV("pause");
    Mutex::Autolock _l(mLock);
    if (mCurrentState & (MEDIA_PLAYER2_PAUSED|MEDIA_PLAYER2_PLAYBACK_COMPLETE))
        return NO_ERROR;
    if ((mPlayer != 0) && (mCurrentState & MEDIA_PLAYER2_STARTED)) {
        status_t ret = mPlayer->pause();
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER2_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER2_PAUSED;
        }
        return ret;
    }
    ALOGE("pause called in state %d, mPlayer(%p)", mCurrentState, mPlayer.get());
    return INVALID_OPERATION;
}

bool MediaPlayer2::isPlaying() {
    Mutex::Autolock _l(mLock);
    if (mPlayer != 0) {
        bool temp = mPlayer->isPlaying();
        ALOGV("isPlaying: %d", temp);
        if ((mCurrentState & MEDIA_PLAYER2_STARTED) && ! temp) {
            ALOGE("internal/external state mismatch corrected");
            mCurrentState = MEDIA_PLAYER2_PAUSED;
        } else if ((mCurrentState & MEDIA_PLAYER2_PAUSED) && temp) {
            ALOGE("internal/external state mismatch corrected");
            mCurrentState = MEDIA_PLAYER2_STARTED;
        }
        return temp;
    }
    ALOGV("isPlaying: no active player");
    return false;
}

mediaplayer2_states MediaPlayer2::getMediaPlayer2State() {
    Mutex::Autolock _l(mLock);
    if (mCurrentState & MEDIA_PLAYER2_STATE_ERROR) {
        return MEDIAPLAYER2_STATE_ERROR;
    }
    if (mPlayer == 0
        || (mCurrentState &
            (MEDIA_PLAYER2_IDLE | MEDIA_PLAYER2_INITIALIZED | MEDIA_PLAYER2_PREPARING))) {
        return MEDIAPLAYER2_STATE_IDLE;
    }
    if (mCurrentState & MEDIA_PLAYER2_STARTED) {
        return MEDIAPLAYER2_STATE_PLAYING;
    }
    if (mCurrentState
        & (MEDIA_PLAYER2_PAUSED | MEDIA_PLAYER2_STOPPED | MEDIA_PLAYER2_PLAYBACK_COMPLETE)) {
        return MEDIAPLAYER2_STATE_PAUSED;
    }
    // now only mCurrentState & MEDIA_PLAYER2_PREPARED is true
    return MEDIAPLAYER2_STATE_PREPARED;
}

status_t MediaPlayer2::setPlaybackSettings(const AudioPlaybackRate& rate) {
    ALOGV("setPlaybackSettings: %f %f %d %d",
            rate.mSpeed, rate.mPitch, rate.mFallbackMode, rate.mStretchMode);
    // Negative speed and pitch does not make sense. Further validation will
    // be done by the respective mediaplayers.
    if (rate.mSpeed <= 0.f || rate.mPitch < 0.f) {
        return BAD_VALUE;
    }
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0 || (mCurrentState & MEDIA_PLAYER2_STOPPED)) {
        return INVALID_OPERATION;
    }

    status_t err = mPlayer->setPlaybackSettings(rate);
    return err;
}

status_t MediaPlayer2::getPlaybackSettings(AudioPlaybackRate* rate /* nonnull */) {
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) {
        return INVALID_OPERATION;
    }
    status_t ret = mPlayer->getPlaybackSettings(rate);
    if (ret == NO_ERROR) {
        ALOGV("getPlaybackSettings(%f, %f, %d, %d)",
                rate->mSpeed, rate->mPitch, rate->mFallbackMode, rate->mStretchMode);
    } else {
        ALOGV("getPlaybackSettings returned %d", ret);
    }
    return ret;
}

status_t MediaPlayer2::setSyncSettings(const AVSyncSettings& sync, float videoFpsHint) {
    ALOGV("setSyncSettings: %u %u %f %f",
            sync.mSource, sync.mAudioAdjustMode, sync.mTolerance, videoFpsHint);
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) return INVALID_OPERATION;
    return mPlayer->setSyncSettings(sync, videoFpsHint);
}

status_t MediaPlayer2::getSyncSettings(
        AVSyncSettings* sync /* nonnull */, float* videoFps /* nonnull */) {
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) {
        return INVALID_OPERATION;
    }
    status_t ret = mPlayer->getSyncSettings(sync, videoFps);
    if (ret == NO_ERROR) {
        ALOGV("getSyncSettings(%u, %u, %f, %f)",
                sync->mSource, sync->mAudioAdjustMode, sync->mTolerance, *videoFps);
    } else {
        ALOGV("getSyncSettings returned %d", ret);
    }
    return ret;

}

status_t MediaPlayer2::getVideoWidth(int *w) {
    ALOGV("getVideoWidth");
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) {
        return INVALID_OPERATION;
    }
    *w = mVideoWidth;
    return NO_ERROR;
}

status_t MediaPlayer2::getVideoHeight(int *h) {
    ALOGV("getVideoHeight");
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) {
        return INVALID_OPERATION;
    }
    *h = mVideoHeight;
    return NO_ERROR;
}

status_t MediaPlayer2::getCurrentPosition(int64_t *msec) {
    ALOGV("getCurrentPosition");
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) {
        return INVALID_OPERATION;
    }
    if (mCurrentPosition >= 0) {
        ALOGV("Using cached seek position: %lld", (long long)mCurrentPosition);
        *msec = mCurrentPosition;
        return NO_ERROR;
    }
    status_t ret = mPlayer->getCurrentPosition(msec);
    if (ret == NO_ERROR) {
        ALOGV("getCurrentPosition = %lld", (long long)*msec);
    } else {
        ALOGE("getCurrentPosition returned %d", ret);
    }
    return ret;
}

status_t MediaPlayer2::getDuration(int64_t *msec) {
    Mutex::Autolock _l(mLock);
    ALOGV("getDuration_l");
    bool isValidState = (mCurrentState & (MEDIA_PLAYER2_PREPARED | MEDIA_PLAYER2_STARTED |
            MEDIA_PLAYER2_PAUSED | MEDIA_PLAYER2_STOPPED | MEDIA_PLAYER2_PLAYBACK_COMPLETE));
    if (mPlayer == 0 || !isValidState) {
        ALOGE("Attempt to call getDuration in wrong state: mPlayer=%p, mCurrentState=%u",
                mPlayer.get(), mCurrentState);
        return INVALID_OPERATION;
    }
    int64_t durationMs;
    status_t ret = mPlayer->getDuration(&durationMs);

    if (ret == NO_ERROR) {
        ALOGV("getDuration = %lld", (long long)durationMs);
    } else {
        ALOGE("getDuration returned %d", ret);
        // Do not enter error state just because no duration was available.
        durationMs = -1;
    }

    if (msec) {
        *msec = durationMs;
    }
    return OK;
}

status_t MediaPlayer2::seekTo_l(int64_t msec, MediaPlayer2SeekMode mode) {
    ALOGV("seekTo (%lld, %d)", (long long)msec, mode);
    if ((mPlayer == 0) || !(mCurrentState & (MEDIA_PLAYER2_STARTED | MEDIA_PLAYER2_PREPARED |
            MEDIA_PLAYER2_PAUSED | MEDIA_PLAYER2_PLAYBACK_COMPLETE))) {
        ALOGE("Attempt to perform seekTo in wrong state: mPlayer=%p, mCurrentState=%u",
              mPlayer.get(), mCurrentState);
        return INVALID_OPERATION;
    }
    if (msec < 0) {
        ALOGW("Attempt to seek to invalid position: %lld", (long long)msec);
        msec = 0;
    }

    int64_t durationMs;
    status_t err = mPlayer->getDuration(&durationMs);

    if (err != OK) {
        ALOGW("Stream has no duration and is therefore not seekable.");
        return err;
    }

    if (msec > durationMs) {
        ALOGW("Attempt to seek to past end of file: request = %lld, durationMs = %lld",
              (long long)msec, (long long)durationMs);

        msec = durationMs;
    }

    // cache duration
    mCurrentPosition = msec;
    mCurrentSeekMode = mode;
    if (mSeekPosition < 0) {
        mSeekPosition = msec;
        mSeekMode = mode;
        return mPlayer->seekTo(msec, mode);
    }
    ALOGV("Seek in progress - queue up seekTo[%lld, %d]", (long long)msec, mode);
    return NO_ERROR;
}

status_t MediaPlayer2::seekTo(int64_t msec, MediaPlayer2SeekMode mode) {
    mLockThreadId = getThreadId();
    Mutex::Autolock _l(mLock);
    status_t result = seekTo_l(msec, mode);
    mLockThreadId = 0;

    return result;
}

status_t MediaPlayer2::notifyAt(int64_t mediaTimeUs) {
    Mutex::Autolock _l(mLock);
    if (mPlayer != 0) {
        return INVALID_OPERATION;
    }

    return mPlayer->notifyAt(mediaTimeUs);
}

status_t MediaPlayer2::reset_l() {
    mLoop = false;
    if (mCurrentState == MEDIA_PLAYER2_IDLE) {
        return NO_ERROR;
    }
    if (mPlayer != 0) {
        status_t ret = mPlayer->reset();
        if (ret != NO_ERROR) {
            ALOGE("reset() failed with return code (%d)", ret);
            mCurrentState = MEDIA_PLAYER2_STATE_ERROR;
        } else {
            mPlayer->setListener(NULL);
            mCurrentState = MEDIA_PLAYER2_IDLE;
        }
        // setDataSource has to be called again to create a
        // new mediaplayer.
        mPlayer = 0;
        return ret;
    }
    clear_l();
    return NO_ERROR;
}

status_t MediaPlayer2::reset() {
    ALOGV("reset");
    mLockThreadId = getThreadId();
    Mutex::Autolock _l(mLock);
    status_t result = reset_l();
    mLockThreadId = 0;

    return result;
}

status_t MediaPlayer2::setAudioStreamType(audio_stream_type_t type) {
    ALOGV("MediaPlayer2::setAudioStreamType");
    Mutex::Autolock _l(mLock);
    if (mStreamType == type) return NO_ERROR;
    if (mCurrentState & ( MEDIA_PLAYER2_PREPARED | MEDIA_PLAYER2_STARTED |
                MEDIA_PLAYER2_PAUSED | MEDIA_PLAYER2_PLAYBACK_COMPLETE ) ) {
        // Can't change the stream type after prepare
        ALOGE("setAudioStream called in state %d", mCurrentState);
        return INVALID_OPERATION;
    }
    // cache
    mStreamType = type;
    return OK;
}

status_t MediaPlayer2::getAudioStreamType(audio_stream_type_t *type) {
    ALOGV("getAudioStreamType");
    Mutex::Autolock _l(mLock);
    *type = mStreamType;
    return OK;
}

status_t MediaPlayer2::setLooping(int loop) {
    ALOGV("MediaPlayer2::setLooping");
    Mutex::Autolock _l(mLock);
    mLoop = (loop != 0);
    if (mPlayer != 0) {
        return mPlayer->setLooping(loop);
    }
    return OK;
}

bool MediaPlayer2::isLooping() {
    ALOGV("isLooping");
    Mutex::Autolock _l(mLock);
    if (mPlayer != 0) {
        return mLoop;
    }
    ALOGV("isLooping: no active player");
    return false;
}

status_t MediaPlayer2::setVolume(float leftVolume, float rightVolume) {
    ALOGV("MediaPlayer2::setVolume(%f, %f)", leftVolume, rightVolume);
    Mutex::Autolock _l(mLock);
    mLeftVolume = leftVolume;
    mRightVolume = rightVolume;
    if (mAudioOutput != 0) {
        mAudioOutput->setVolume(leftVolume, rightVolume);
    }
    return OK;
}

status_t MediaPlayer2::setAudioSessionId(audio_session_t sessionId) {
    ALOGV("MediaPlayer2::setAudioSessionId(%d)", sessionId);
    Mutex::Autolock _l(mLock);
    if (!(mCurrentState & MEDIA_PLAYER2_IDLE)) {
        ALOGE("setAudioSessionId called in state %d", mCurrentState);
        return INVALID_OPERATION;
    }
    if (sessionId < 0) {
        return BAD_VALUE;
    }
    if (sessionId != mAudioSessionId) {
        AudioSystem::acquireAudioSessionId(sessionId, -1);
        AudioSystem::releaseAudioSessionId(mAudioSessionId, -1);
        mAudioSessionId = sessionId;
    }
    return NO_ERROR;
}

audio_session_t MediaPlayer2::getAudioSessionId() {
    Mutex::Autolock _l(mLock);
    return mAudioSessionId;
}

status_t MediaPlayer2::setAuxEffectSendLevel(float level) {
    ALOGV("MediaPlayer2::setAuxEffectSendLevel(%f)", level);
    Mutex::Autolock _l(mLock);
    mSendLevel = level;
    if (mAudioOutput != 0) {
        return mAudioOutput->setAuxEffectSendLevel(level);
    }
    return OK;
}

status_t MediaPlayer2::attachAuxEffect(int effectId) {
    ALOGV("MediaPlayer2::attachAuxEffect(%d)", effectId);
    Mutex::Autolock _l(mLock);
    if (mAudioOutput == 0 ||
        (mCurrentState & MEDIA_PLAYER2_IDLE) ||
        (mCurrentState == MEDIA_PLAYER2_STATE_ERROR )) {
        ALOGE("attachAuxEffect called in state %d, mPlayer(%p)", mCurrentState, mPlayer.get());
        return INVALID_OPERATION;
    }

    return mAudioOutput->attachAuxEffect(effectId);
}

// always call with lock held
status_t MediaPlayer2::checkStateForKeySet_l(int key) {
    switch(key) {
    case MEDIA2_KEY_PARAMETER_AUDIO_ATTRIBUTES:
        if (mCurrentState & ( MEDIA_PLAYER2_PREPARED | MEDIA_PLAYER2_STARTED |
                MEDIA_PLAYER2_PAUSED | MEDIA_PLAYER2_PLAYBACK_COMPLETE) ) {
            // Can't change the audio attributes after prepare
            ALOGE("trying to set audio attributes called in state %d", mCurrentState);
            return INVALID_OPERATION;
        }
        break;
    default:
        // parameter doesn't require player state check
        break;
    }
    return OK;
}

status_t MediaPlayer2::setParameter(int key, const Parcel& request) {
    ALOGV("MediaPlayer2::setParameter(%d)", key);
    status_t status = INVALID_OPERATION;
    Mutex::Autolock _l(mLock);
    if (checkStateForKeySet_l(key) != OK) {
        return status;
    }
    switch (key) {
    case MEDIA2_KEY_PARAMETER_AUDIO_ATTRIBUTES:
        // save the marshalled audio attributes
        if (mAudioAttributesParcel != NULL) {
            delete mAudioAttributesParcel;
        }
        mAudioAttributesParcel = new Parcel();
        mAudioAttributesParcel->appendFrom(&request, 0, request.dataSize());
        status = setAudioAttributes_l(request);
        if (status != OK) {
            return status;
        }
        break;
    default:
        ALOGV_IF(mPlayer == NULL, "setParameter: no active player");
        break;
    }

    if (mPlayer != NULL) {
        status = mPlayer->setParameter(key, request);
    }
    return status;
}

status_t MediaPlayer2::getParameter(int key, Parcel *reply) {
    ALOGV("MediaPlayer2::getParameter(%d)", key);
    Mutex::Autolock _l(mLock);
    if (key == MEDIA2_KEY_PARAMETER_AUDIO_ATTRIBUTES) {
        if (reply == NULL) {
            return BAD_VALUE;
        }
        if (mAudioAttributesParcel != NULL) {
            reply->appendFrom(mAudioAttributesParcel, 0, mAudioAttributesParcel->dataSize());
        }
        return OK;
    }

    if (mPlayer == NULL) {
        ALOGV("getParameter: no active player");
        return INVALID_OPERATION;
    }

    status_t status =  mPlayer->getParameter(key, reply);
    if (status != OK) {
        ALOGD("getParameter returns %d", status);
    }
    return status;
}

bool MediaPlayer2::shouldDropMetadata(media::Metadata::Type code) const {
    Mutex::Autolock lock(mLock);

    if (findMetadata(mMetadataDrop, code)) {
        return true;
    }

    if (mMetadataAllow.isEmpty() || findMetadata(mMetadataAllow, code)) {
        return false;
    } else {
        return true;
    }
}


void MediaPlayer2::addNewMetadataUpdate(media::Metadata::Type metadata_type) {
    Mutex::Autolock lock(mLock);
    if (mMetadataUpdated.indexOf(metadata_type) < 0) {
        mMetadataUpdated.add(metadata_type);
    }
}

void MediaPlayer2::notify(int64_t srcId, int msg, int ext1, int ext2, const Parcel *obj) {
    ALOGV("message received srcId=%lld, msg=%d, ext1=%d, ext2=%d",
          (long long)srcId, msg, ext1, ext2);

    if (MEDIA2_INFO == msg && MEDIA2_INFO_METADATA_UPDATE == ext1) {
        const media::Metadata::Type metadata_type = ext2;

        if(shouldDropMetadata(metadata_type)) {
            return;
        }

        // Update the list of metadata that have changed. getMetadata
        // also access mMetadataUpdated and clears it.
        addNewMetadataUpdate(metadata_type);
    }

    bool send = true;
    bool locked = false;

    // TODO: In the future, we might be on the same thread if the app is
    // running in the same process as the media server. In that case,
    // this will deadlock.
    //
    // The threadId hack below works around this for the care of prepare,
    // seekTo, start, and reset within the same process.
    // FIXME: Remember, this is a hack, it's not even a hack that is applied
    // consistently for all use-cases, this needs to be revisited.
    if (mLockThreadId != getThreadId()) {
        mLock.lock();
        locked = true;
    }

    // Allows calls from JNI in idle state to notify errors
    if (!(msg == MEDIA2_ERROR && mCurrentState == MEDIA_PLAYER2_IDLE) && mPlayer == 0) {
        ALOGV("notify(%lld, %d, %d, %d) callback on disconnected mediaplayer",
              (long long)srcId, msg, ext1, ext2);
        if (locked) mLock.unlock();   // release the lock when done.
        return;
    }

    switch (msg) {
    case MEDIA2_NOP: // interface test message
        break;
    case MEDIA2_PREPARED:
        ALOGV("MediaPlayer2::notify() prepared");
        mCurrentState = MEDIA_PLAYER2_PREPARED;
        break;
    case MEDIA2_DRM_INFO:
        ALOGV("MediaPlayer2::notify() MEDIA2_DRM_INFO(%lld, %d, %d, %d, %p)",
              (long long)srcId, msg, ext1, ext2, obj);
        break;
    case MEDIA2_PLAYBACK_COMPLETE:
        ALOGV("playback complete");
        if (mCurrentState == MEDIA_PLAYER2_IDLE) {
            ALOGE("playback complete in idle state");
        }
        if (!mLoop) {
            mCurrentState = MEDIA_PLAYER2_PLAYBACK_COMPLETE;
        }
        break;
    case MEDIA2_ERROR:
        // Always log errors.
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        ALOGE("error (%d, %d)", ext1, ext2);
        mCurrentState = MEDIA_PLAYER2_STATE_ERROR;
        break;
    case MEDIA2_INFO:
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        if (ext1 != MEDIA2_INFO_VIDEO_TRACK_LAGGING) {
            ALOGW("info/warning (%d, %d)", ext1, ext2);
        }
        break;
    case MEDIA2_SEEK_COMPLETE:
        ALOGV("Received seek complete");
        if (mSeekPosition != mCurrentPosition || (mSeekMode != mCurrentSeekMode)) {
            ALOGV("Executing queued seekTo(%lld, %d)",
                  (long long)mCurrentPosition, mCurrentSeekMode);
            mSeekPosition = -1;
            mSeekMode = MediaPlayer2SeekMode::SEEK_PREVIOUS_SYNC;
            seekTo_l(mCurrentPosition, mCurrentSeekMode);
        }
        else {
            ALOGV("All seeks complete - return to regularly scheduled program");
            mCurrentPosition = mSeekPosition = -1;
            mCurrentSeekMode = mSeekMode = MediaPlayer2SeekMode::SEEK_PREVIOUS_SYNC;
        }
        break;
    case MEDIA2_BUFFERING_UPDATE:
        ALOGV("buffering %d", ext1);
        break;
    case MEDIA2_SET_VIDEO_SIZE:
        ALOGV("New video size %d x %d", ext1, ext2);
        mVideoWidth = ext1;
        mVideoHeight = ext2;
        break;
    case MEDIA2_NOTIFY_TIME:
        ALOGV("Received notify time message");
        break;
    case MEDIA2_TIMED_TEXT:
        ALOGV("Received timed text message");
        break;
    case MEDIA2_SUBTITLE_DATA:
        ALOGV("Received subtitle data message");
        break;
    case MEDIA2_META_DATA:
        ALOGV("Received timed metadata message");
        break;
    default:
        ALOGV("unrecognized message: (%d, %d, %d)", msg, ext1, ext2);
        break;
    }

    sp<MediaPlayer2Listener> listener = mListener;
    if (locked) mLock.unlock();

    // this prevents re-entrant calls into client code
    if ((listener != 0) && send) {
        Mutex::Autolock _l(mNotifyLock);
        ALOGV("callback application");
        listener->notify(srcId, msg, ext1, ext2, obj);
        ALOGV("back from callback");
    }
}

// Modular DRM
status_t MediaPlayer2::prepareDrm(const uint8_t uuid[16], const Vector<uint8_t>& drmSessionId) {
    // TODO change to ALOGV
    ALOGD("prepareDrm: uuid: %p  drmSessionId: %p(%zu)", uuid,
            drmSessionId.array(), drmSessionId.size());
    Mutex::Autolock _l(mLock);
    if (mPlayer == NULL) {
        return NO_INIT;
    }

    // Only allowed it in player's preparing/prepared state.
    // We get here only if MEDIA_DRM_INFO has already arrived (e.g., prepare is half-way through or
    // completed) so the state change to "prepared" might not have happened yet (e.g., buffering).
    // Still, we can allow prepareDrm for the use case of being called in OnDrmInfoListener.
    if (!(mCurrentState & (MEDIA_PLAYER2_PREPARING | MEDIA_PLAYER2_PREPARED))) {
        ALOGE("prepareDrm is called in the wrong state (%d).", mCurrentState);
        return INVALID_OPERATION;
    }

    if (drmSessionId.isEmpty()) {
        ALOGE("prepareDrm: Unexpected. Can't proceed with crypto. Empty drmSessionId.");
        return INVALID_OPERATION;
    }

    // Passing down to mediaserver mainly for creating the crypto
    status_t status = mPlayer->prepareDrm(uuid, drmSessionId);
    ALOGE_IF(status != OK, "prepareDrm: Failed at mediaserver with ret: %d", status);

    // TODO change to ALOGV
    ALOGD("prepareDrm: mediaserver::prepareDrm ret=%d", status);

    return status;
}

status_t MediaPlayer2::releaseDrm() {
    Mutex::Autolock _l(mLock);
    if (mPlayer == NULL) {
        return NO_INIT;
    }

    // Not allowing releaseDrm in an active/resumable state
    if (mCurrentState & (MEDIA_PLAYER2_STARTED |
                         MEDIA_PLAYER2_PAUSED |
                         MEDIA_PLAYER2_PLAYBACK_COMPLETE |
                         MEDIA_PLAYER2_STATE_ERROR)) {
        ALOGE("releaseDrm Unexpected state %d. Can only be called in stopped/idle.", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t status = mPlayer->releaseDrm();
    // TODO change to ALOGV
    ALOGD("releaseDrm: mediaserver::releaseDrm ret: %d", status);
    if (status != OK) {
        ALOGE("releaseDrm: Failed at mediaserver with ret: %d", status);
        // Overriding to OK so the client proceed with its own cleanup
        // Client can't do more cleanup. mediaserver release its crypto at end of session anyway.
        status = OK;
    }

    return status;
}

status_t MediaPlayer2::setOutputDevice(audio_port_handle_t deviceId) {
    Mutex::Autolock _l(mLock);
    if (mAudioOutput == NULL) {
        ALOGV("setOutputDevice: audio sink not init");
        return NO_INIT;
    }
    return mAudioOutput->setOutputDevice(deviceId);
}

audio_port_handle_t MediaPlayer2::getRoutedDeviceId() {
    Mutex::Autolock _l(mLock);
    if (mAudioOutput == NULL) {
        ALOGV("getRoutedDeviceId: audio sink not init");
        return AUDIO_PORT_HANDLE_NONE;
    }
    audio_port_handle_t deviceId;
    status_t status = mAudioOutput->getRoutedDeviceId(&deviceId);
    if (status != NO_ERROR) {
        return AUDIO_PORT_HANDLE_NONE;
    }
    return deviceId;
}

status_t MediaPlayer2::enableAudioDeviceCallback(bool enabled) {
    Mutex::Autolock _l(mLock);
    if (mAudioOutput == NULL) {
        ALOGV("addAudioDeviceCallback: player not init");
        return NO_INIT;
    }
    return mAudioOutput->enableAudioDeviceCallback(enabled);
}

status_t MediaPlayer2::dump(int fd, const Vector<String16>& args) {
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    result.append(" MediaPlayer2\n");
    snprintf(buffer, 255, "  pid(%d), looping(%s)\n", mPid, mLoop?"true": "false");
    result.append(buffer);

    sp<MediaPlayer2Interface> player;
    sp<MediaPlayer2AudioOutput> audioOutput;
    bool locked = false;
    for (int i = 0; i < kDumpLockRetries; ++i) {
        if (mLock.tryLock() == NO_ERROR) {
            locked = true;
            break;
        }
        usleep(kDumpLockSleepUs);
    }

    if (locked) {
        player = mPlayer;
        audioOutput = mAudioOutput;
        mLock.unlock();
    } else {
        result.append("  lock is taken, no dump from player and audio output\n");
    }
    write(fd, result.string(), result.size());

    if (player != NULL) {
        player->dump(fd, args);
    }
    if (audioOutput != 0) {
        audioOutput->dump(fd, args);
    }
    write(fd, "\n", 1);
    return NO_ERROR;
}

} // namespace android
