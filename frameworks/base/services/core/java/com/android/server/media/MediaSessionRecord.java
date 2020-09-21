/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.server.media;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ParceledListSlice;
import android.media.AudioManager;
import android.media.AudioManagerInternal;
import android.media.AudioSystem;
import android.media.MediaDescription;
import android.media.MediaMetadata;
import android.media.Rating;
import android.media.VolumeProvider;
import android.media.session.ISession;
import android.media.session.ISessionCallback;
import android.media.session.ISessionController;
import android.media.session.ISessionControllerCallback;
import android.media.session.MediaController;
import android.media.session.MediaController.PlaybackInfo;
import android.media.session.MediaSession;
import android.media.session.ParcelableVolumeInfo;
import android.media.session.PlaybackState;
import android.media.AudioAttributes;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;
import android.os.DeadObjectException;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.os.RemoteException;
import android.os.ResultReceiver;
import android.os.SystemClock;
import android.util.Log;
import android.util.Slog;
import android.view.KeyEvent;

import com.android.server.LocalServices;

import java.io.PrintWriter;
import java.util.ArrayList;

/**
 * This is the system implementation of a Session. Apps will interact with the
 * MediaSession wrapper class instead.
 */
public class MediaSessionRecord implements IBinder.DeathRecipient {
    private static final String TAG = "MediaSessionRecord";
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);

    /**
     * The amount of time we'll send an assumed volume after the last volume
     * command before reverting to the last reported volume.
     */
    private static final int OPTIMISTIC_VOLUME_TIMEOUT = 1000;

    private final MessageHandler mHandler;

    private final int mOwnerPid;
    private final int mOwnerUid;
    private final int mUserId;
    private final String mPackageName;
    private final String mTag;
    private final ControllerStub mController;
    private final SessionStub mSession;
    private final SessionCb mSessionCb;
    private final MediaSessionService mService;
    private final Context mContext;

    private final Object mLock = new Object();
    private final ArrayList<ISessionControllerCallbackHolder> mControllerCallbackHolders =
            new ArrayList<>();

    private long mFlags;
    private PendingIntent mMediaButtonReceiver;
    private PendingIntent mLaunchIntent;

    // TransportPerformer fields

    private Bundle mExtras;
    private MediaMetadata mMetadata;
    private PlaybackState mPlaybackState;
    private ParceledListSlice mQueue;
    private CharSequence mQueueTitle;
    private int mRatingType;
    // End TransportPerformer fields

    // Volume handling fields
    private AudioAttributes mAudioAttrs;
    private AudioManager mAudioManager;
    private AudioManagerInternal mAudioManagerInternal;
    private int mVolumeType = PlaybackInfo.PLAYBACK_TYPE_LOCAL;
    private int mVolumeControlType = VolumeProvider.VOLUME_CONTROL_ABSOLUTE;
    private int mMaxVolume = 0;
    private int mCurrentVolume = 0;
    private int mOptimisticVolume = -1;
    // End volume handling fields

    private boolean mIsActive = false;
    private boolean mDestroyed = false;

    public MediaSessionRecord(int ownerPid, int ownerUid, int userId, String ownerPackageName,
            ISessionCallback cb, String tag, MediaSessionService service, Looper handlerLooper) {
        mOwnerPid = ownerPid;
        mOwnerUid = ownerUid;
        mUserId = userId;
        mPackageName = ownerPackageName;
        mTag = tag;
        mController = new ControllerStub();
        mSession = new SessionStub();
        mSessionCb = new SessionCb(cb);
        mService = service;
        mContext = mService.getContext();
        mHandler = new MessageHandler(handlerLooper);
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        mAudioManagerInternal = LocalServices.getService(AudioManagerInternal.class);
        mAudioAttrs = new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).build();
    }

    /**
     * Get the binder for the {@link MediaSession}.
     *
     * @return The session binder apps talk to.
     */
    public ISession getSessionBinder() {
        return mSession;
    }

    /**
     * Get the binder for the {@link MediaController}.
     *
     * @return The controller binder apps talk to.
     */
    public ISessionController getControllerBinder() {
        return mController;
    }

    /**
     * Get the info for this session.
     *
     * @return Info that identifies this session.
     */
    public String getPackageName() {
        return mPackageName;
    }

    /**
     * Get the tag for the session.
     *
     * @return The session's tag.
     */
    public String getTag() {
        return mTag;
    }

    /**
     * Get the intent the app set for their media button receiver.
     *
     * @return The pending intent set by the app or null.
     */
    public PendingIntent getMediaButtonReceiver() {
        return mMediaButtonReceiver;
    }

    /**
     * Get this session's flags.
     *
     * @return The flags for this session.
     */
    public long getFlags() {
        return mFlags;
    }

    /**
     * Check if this session has the specified flag.
     *
     * @param flag The flag to check.
     * @return True if this session has that flag set, false otherwise.
     */
    public boolean hasFlag(int flag) {
        return (mFlags & flag) != 0;
    }

    /**
     * Get the UID this session was created for.
     *
     * @return The UID for this session.
     */
    public int getUid() {
        return mOwnerUid;
    }

    /**
     * Get the user id this session was created for.
     *
     * @return The user id for this session.
     */
    public int getUserId() {
        return mUserId;
    }

    /**
     * Check if this session has system priorty and should receive media buttons
     * before any other sessions.
     *
     * @return True if this is a system priority session, false otherwise
     */
    public boolean isSystemPriority() {
        return (mFlags & MediaSession.FLAG_EXCLUSIVE_GLOBAL_PRIORITY) != 0;
    }

    /**
     * Send a volume adjustment to the session owner. Direction must be one of
     * {@link AudioManager#ADJUST_LOWER}, {@link AudioManager#ADJUST_RAISE},
     * {@link AudioManager#ADJUST_SAME}.
     *
     * @param packageName The package that made the original volume request.
     * @param pid The pid that made the original volume request.
     * @param uid The uid that made the original volume request.
     * @param caller caller binder. can be {@code null} if it's from the volume key.
     * @param asSystemService {@code true} if the event sent to the session as if it was come from
     *          the system service instead of the app process. This helps sessions to distinguish
     *          between the key injection by the app and key events from the hardware devices.
     *          Should be used only when the volume key events aren't handled by foreground
     *          activity. {@code false} otherwise to tell session about the real caller.
     * @param direction The direction to adjust volume in.
     * @param flags Any of the flags from {@link AudioManager}.
     * @param useSuggested True to use adjustSuggestedStreamVolume instead of
     */
    public void adjustVolume(String packageName, int pid, int uid,
            ISessionControllerCallback caller, boolean asSystemService, int direction, int flags,
            boolean useSuggested) {
        int previousFlagPlaySound = flags & AudioManager.FLAG_PLAY_SOUND;
        if (isPlaybackActive() || hasFlag(MediaSession.FLAG_EXCLUSIVE_GLOBAL_PRIORITY)) {
            flags &= ~AudioManager.FLAG_PLAY_SOUND;
        }
        if (mVolumeType == PlaybackInfo.PLAYBACK_TYPE_LOCAL) {
            // Adjust the volume with a handler not to be blocked by other system service.
            int stream = AudioAttributes.toLegacyStreamType(mAudioAttrs);
            postAdjustLocalVolume(stream, direction, flags, packageName, uid, useSuggested,
                    previousFlagPlaySound);
        } else {
            if (mVolumeControlType == VolumeProvider.VOLUME_CONTROL_FIXED) {
                // Nothing to do, the volume cannot be changed
                return;
            }
            if (direction == AudioManager.ADJUST_TOGGLE_MUTE
                    || direction == AudioManager.ADJUST_MUTE
                    || direction == AudioManager.ADJUST_UNMUTE) {
                Log.w(TAG, "Muting remote playback is not supported");
                return;
            }
            mSessionCb.adjustVolume(packageName, pid, uid, caller, asSystemService, direction);

            int volumeBefore = (mOptimisticVolume < 0 ? mCurrentVolume : mOptimisticVolume);
            mOptimisticVolume = volumeBefore + direction;
            mOptimisticVolume = Math.max(0, Math.min(mOptimisticVolume, mMaxVolume));
            mHandler.removeCallbacks(mClearOptimisticVolumeRunnable);
            mHandler.postDelayed(mClearOptimisticVolumeRunnable, OPTIMISTIC_VOLUME_TIMEOUT);
            if (volumeBefore != mOptimisticVolume) {
                pushVolumeUpdate();
            }
            mService.notifyRemoteVolumeChanged(flags, this);

            if (DEBUG) {
                Log.d(TAG, "Adjusted optimistic volume to " + mOptimisticVolume + " max is "
                        + mMaxVolume);
            }
        }
    }

    private void setVolumeTo(String packageName, int pid, int uid,
            ISessionControllerCallback caller, int value, int flags) {
        if (mVolumeType == PlaybackInfo.PLAYBACK_TYPE_LOCAL) {
            int stream = AudioAttributes.toLegacyStreamType(mAudioAttrs);
            mAudioManagerInternal.setStreamVolumeForUid(stream, value, flags, packageName, uid);
        } else {
            if (mVolumeControlType != VolumeProvider.VOLUME_CONTROL_ABSOLUTE) {
                // Nothing to do. The volume can't be set directly.
                return;
            }
            value = Math.max(0, Math.min(value, mMaxVolume));
            mSessionCb.setVolumeTo(packageName, pid, uid, caller, value);

            int volumeBefore = (mOptimisticVolume < 0 ? mCurrentVolume : mOptimisticVolume);
            mOptimisticVolume = Math.max(0, Math.min(value, mMaxVolume));
            mHandler.removeCallbacks(mClearOptimisticVolumeRunnable);
            mHandler.postDelayed(mClearOptimisticVolumeRunnable, OPTIMISTIC_VOLUME_TIMEOUT);
            if (volumeBefore != mOptimisticVolume) {
                pushVolumeUpdate();
            }
            mService.notifyRemoteVolumeChanged(flags, this);

            if (DEBUG) {
                Log.d(TAG, "Set optimistic volume to " + mOptimisticVolume + " max is "
                        + mMaxVolume);
            }
        }
    }

    /**
     * Check if this session has been set to active by the app.
     *
     * @return True if the session is active, false otherwise.
     */
    public boolean isActive() {
        return mIsActive && !mDestroyed;
    }

    /**
     * Get the playback state.
     *
     * @return The current playback state.
     */
    public PlaybackState getPlaybackState() {
        return mPlaybackState;
    }

    /**
     * Check if the session is currently performing playback.
     *
     * @return True if the session is performing playback, false otherwise.
     */
    public boolean isPlaybackActive() {
        int state = mPlaybackState == null ? PlaybackState.STATE_NONE : mPlaybackState.getState();
        return MediaSession.isActiveState(state);
    }

    /**
     * Get the type of playback, either local or remote.
     *
     * @return The current type of playback.
     */
    public int getPlaybackType() {
        return mVolumeType;
    }

    /**
     * Get the local audio stream being used. Only valid if playback type is
     * local.
     *
     * @return The audio stream the session is using.
     */
    public AudioAttributes getAudioAttributes() {
        return mAudioAttrs;
    }

    /**
     * Get the type of volume control. Only valid if playback type is remote.
     *
     * @return The volume control type being used.
     */
    public int getVolumeControl() {
        return mVolumeControlType;
    }

    /**
     * Get the max volume that can be set. Only valid if playback type is
     * remote.
     *
     * @return The max volume that can be set.
     */
    public int getMaxVolume() {
        return mMaxVolume;
    }

    /**
     * Get the current volume for this session. Only valid if playback type is
     * remote.
     *
     * @return The current volume of the remote playback.
     */
    public int getCurrentVolume() {
        return mCurrentVolume;
    }

    /**
     * Get the volume we'd like it to be set to. This is only valid for a short
     * while after a call to adjust or set volume.
     *
     * @return The current optimistic volume or -1.
     */
    public int getOptimisticVolume() {
        return mOptimisticVolume;
    }

    public boolean isTransportControlEnabled() {
        return hasFlag(MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS);
    }

    @Override
    public void binderDied() {
        mService.sessionDied(this);
    }

    /**
     * Finish cleaning up this session, including disconnecting if connected and
     * removing the death observer from the callback binder.
     */
    public void onDestroy() {
        synchronized (mLock) {
            if (mDestroyed) {
                return;
            }
            mDestroyed = true;
            mHandler.post(MessageHandler.MSG_DESTROYED);
        }
    }

    public ISessionCallback getCallback() {
        return mSessionCb.mCb;
    }

    public void sendMediaButton(String packageName, int pid, int uid, boolean asSystemService,
            KeyEvent ke, int sequenceId, ResultReceiver cb) {
        mSessionCb.sendMediaButton(packageName, pid, uid, asSystemService, ke, sequenceId, cb);
    }

    public void dump(PrintWriter pw, String prefix) {
        pw.println(prefix + mTag + " " + this);

        final String indent = prefix + "  ";
        pw.println(indent + "ownerPid=" + mOwnerPid + ", ownerUid=" + mOwnerUid
                + ", userId=" + mUserId);
        pw.println(indent + "package=" + mPackageName);
        pw.println(indent + "launchIntent=" + mLaunchIntent);
        pw.println(indent + "mediaButtonReceiver=" + mMediaButtonReceiver);
        pw.println(indent + "active=" + mIsActive);
        pw.println(indent + "flags=" + mFlags);
        pw.println(indent + "rating type=" + mRatingType);
        pw.println(indent + "controllers: " + mControllerCallbackHolders.size());
        pw.println(indent + "state=" + (mPlaybackState == null ? null : mPlaybackState.toString()));
        pw.println(indent + "audioAttrs=" + mAudioAttrs);
        pw.println(indent + "volumeType=" + mVolumeType + ", controlType=" + mVolumeControlType
                + ", max=" + mMaxVolume + ", current=" + mCurrentVolume);
        pw.println(indent + "metadata:" + getShortMetadataString());
        pw.println(indent + "queueTitle=" + mQueueTitle + ", size="
                + (mQueue == null ? 0 : mQueue.getList().size()));
    }

    @Override
    public String toString() {
        return mPackageName + "/" + mTag + " (userId=" + mUserId + ")";
    }

    private void postAdjustLocalVolume(final int stream, final int direction, final int flags,
            final String packageName, final int uid, final boolean useSuggested,
            final int previousFlagPlaySound) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    if (useSuggested) {
                        if (AudioSystem.isStreamActive(stream, 0)) {
                            mAudioManagerInternal.adjustSuggestedStreamVolumeForUid(stream,
                                    direction, flags, packageName, uid);
                        } else {
                            mAudioManagerInternal.adjustSuggestedStreamVolumeForUid(
                                    AudioManager.USE_DEFAULT_STREAM_TYPE, direction,
                                    flags | previousFlagPlaySound, packageName, uid);
                        }
                    } else {
                        mAudioManagerInternal.adjustStreamVolumeForUid(stream, direction, flags,
                                packageName, uid);
                    }
                } catch (IllegalArgumentException e) {
                    Log.e(TAG, "Cannot adjust volume: direction=" + direction + ", stream="
                            + stream + ", flags=" + flags + ", packageName=" + packageName
                            + ", uid=" + uid + ", useSuggested=" + useSuggested
                            + ", previousFlagPlaySound=" + previousFlagPlaySound, e);
                }
            }
        });
    }

    private String getShortMetadataString() {
        int fields = mMetadata == null ? 0 : mMetadata.size();
        MediaDescription description = mMetadata == null ? null : mMetadata
                .getDescription();
        return "size=" + fields + ", description=" + description;
    }

    private void logCallbackException(
            String msg, ISessionControllerCallbackHolder holder, Exception e) {
        Log.v(TAG, msg + ", this=" + this + ", callback package=" + holder.mPackageName
                + ", exception=" + e);
    }

    private void pushPlaybackStateUpdate() {
        synchronized (mLock) {
            if (mDestroyed) {
                return;
            }
            for (int i = mControllerCallbackHolders.size() - 1; i >= 0; i--) {
                ISessionControllerCallbackHolder holder = mControllerCallbackHolders.get(i);
                try {
                    holder.mCallback.onPlaybackStateChanged(mPlaybackState);
                } catch (DeadObjectException e) {
                    mControllerCallbackHolders.remove(i);
                    logCallbackException("Removed dead callback in pushPlaybackStateUpdate",
                            holder, e);
                } catch (RemoteException e) {
                    logCallbackException("unexpected exception in pushPlaybackStateUpdate",
                            holder, e);
                }
            }
        }
    }

    private void pushMetadataUpdate() {
        synchronized (mLock) {
            if (mDestroyed) {
                return;
            }
            for (int i = mControllerCallbackHolders.size() - 1; i >= 0; i--) {
                ISessionControllerCallbackHolder holder = mControllerCallbackHolders.get(i);
                try {
                    holder.mCallback.onMetadataChanged(mMetadata);
                } catch (DeadObjectException e) {
                    logCallbackException("Removing dead callback in pushMetadataUpdate", holder, e);
                    mControllerCallbackHolders.remove(i);
                } catch (RemoteException e) {
                    logCallbackException("unexpected exception in pushMetadataUpdate", holder, e);
                }
            }
        }
    }

    private void pushQueueUpdate() {
        synchronized (mLock) {
            if (mDestroyed) {
                return;
            }
            for (int i = mControllerCallbackHolders.size() - 1; i >= 0; i--) {
                ISessionControllerCallbackHolder holder = mControllerCallbackHolders.get(i);
                try {
                    holder.mCallback.onQueueChanged(mQueue);
                } catch (DeadObjectException e) {
                    mControllerCallbackHolders.remove(i);
                    logCallbackException("Removed dead callback in pushQueueUpdate", holder, e);
                } catch (RemoteException e) {
                    logCallbackException("unexpected exception in pushQueueUpdate", holder, e);
                }
            }
        }
    }

    private void pushQueueTitleUpdate() {
        synchronized (mLock) {
            if (mDestroyed) {
                return;
            }
            for (int i = mControllerCallbackHolders.size() - 1; i >= 0; i--) {
                ISessionControllerCallbackHolder holder = mControllerCallbackHolders.get(i);
                try {
                    holder.mCallback.onQueueTitleChanged(mQueueTitle);
                } catch (DeadObjectException e) {
                    mControllerCallbackHolders.remove(i);
                    logCallbackException("Removed dead callback in pushQueueTitleUpdate",
                            holder, e);
                } catch (RemoteException e) {
                    logCallbackException("unexpected exception in pushQueueTitleUpdate",
                            holder, e);
                }
            }
        }
    }

    private void pushExtrasUpdate() {
        synchronized (mLock) {
            if (mDestroyed) {
                return;
            }
            for (int i = mControllerCallbackHolders.size() - 1; i >= 0; i--) {
                ISessionControllerCallbackHolder holder = mControllerCallbackHolders.get(i);
                try {
                    holder.mCallback.onExtrasChanged(mExtras);
                } catch (DeadObjectException e) {
                    mControllerCallbackHolders.remove(i);
                    logCallbackException("Removed dead callback in pushExtrasUpdate", holder, e);
                } catch (RemoteException e) {
                    logCallbackException("unexpected exception in pushExtrasUpdate", holder, e);
                }
            }
        }
    }

    private void pushVolumeUpdate() {
        synchronized (mLock) {
            if (mDestroyed) {
                return;
            }
            ParcelableVolumeInfo info = mController.getVolumeAttributes();
            for (int i = mControllerCallbackHolders.size() - 1; i >= 0; i--) {
                ISessionControllerCallbackHolder holder = mControllerCallbackHolders.get(i);
                try {
                    holder.mCallback.onVolumeInfoChanged(info);
                } catch (DeadObjectException e) {
                    mControllerCallbackHolders.remove(i);
                    logCallbackException("Removing dead callback in pushVolumeUpdate", holder, e);
                } catch (RemoteException e) {
                    logCallbackException("Unexpected exception in pushVolumeUpdate", holder, e);
                }
            }
        }
    }

    private void pushEvent(String event, Bundle data) {
        synchronized (mLock) {
            if (mDestroyed) {
                return;
            }
            for (int i = mControllerCallbackHolders.size() - 1; i >= 0; i--) {
                ISessionControllerCallbackHolder holder = mControllerCallbackHolders.get(i);
                try {
                    holder.mCallback.onEvent(event, data);
                } catch (DeadObjectException e) {
                    mControllerCallbackHolders.remove(i);
                    logCallbackException("Removing dead callback in pushEvent", holder, e);
                } catch (RemoteException e) {
                    logCallbackException("unexpected exception in pushEvent", holder, e);
                }
            }
        }
    }

    private void pushSessionDestroyed() {
        synchronized (mLock) {
            // This is the only method that may be (and can only be) called
            // after the session is destroyed.
            if (!mDestroyed) {
                return;
            }
            for (int i = mControllerCallbackHolders.size() - 1; i >= 0; i--) {
                ISessionControllerCallbackHolder holder = mControllerCallbackHolders.get(i);
                try {
                    holder.mCallback.onSessionDestroyed();
                } catch (DeadObjectException e) {
                    logCallbackException("Removing dead callback in pushEvent", holder, e);
                    mControllerCallbackHolders.remove(i);
                } catch (RemoteException e) {
                    logCallbackException("unexpected exception in pushEvent", holder, e);
                }
            }
            // After notifying clear all listeners
            mControllerCallbackHolders.clear();
        }
    }

    private PlaybackState getStateWithUpdatedPosition() {
        PlaybackState state;
        long duration = -1;
        synchronized (mLock) {
            state = mPlaybackState;
            if (mMetadata != null && mMetadata.containsKey(MediaMetadata.METADATA_KEY_DURATION)) {
                duration = mMetadata.getLong(MediaMetadata.METADATA_KEY_DURATION);
            }
        }
        PlaybackState result = null;
        if (state != null) {
            if (state.getState() == PlaybackState.STATE_PLAYING
                    || state.getState() == PlaybackState.STATE_FAST_FORWARDING
                    || state.getState() == PlaybackState.STATE_REWINDING) {
                long updateTime = state.getLastPositionUpdateTime();
                long currentTime = SystemClock.elapsedRealtime();
                if (updateTime > 0) {
                    long position = (long) (state.getPlaybackSpeed()
                            * (currentTime - updateTime)) + state.getPosition();
                    if (duration >= 0 && position > duration) {
                        position = duration;
                    } else if (position < 0) {
                        position = 0;
                    }
                    PlaybackState.Builder builder = new PlaybackState.Builder(state);
                    builder.setState(state.getState(), position, state.getPlaybackSpeed(),
                            currentTime);
                    result = builder.build();
                }
            }
        }
        return result == null ? state : result;
    }

    private int getControllerHolderIndexForCb(ISessionControllerCallback cb) {
        IBinder binder = cb.asBinder();
        for (int i = mControllerCallbackHolders.size() - 1; i >= 0; i--) {
            if (binder.equals(mControllerCallbackHolders.get(i).mCallback.asBinder())) {
                return i;
            }
        }
        return -1;
    }

    private final Runnable mClearOptimisticVolumeRunnable = new Runnable() {
        @Override
        public void run() {
            boolean needUpdate = (mOptimisticVolume != mCurrentVolume);
            mOptimisticVolume = -1;
            if (needUpdate) {
                pushVolumeUpdate();
            }
        }
    };

    private final class SessionStub extends ISession.Stub {
        @Override
        public void destroy() {
            final long token = Binder.clearCallingIdentity();
            try {
                mService.destroySession(MediaSessionRecord.this);
            } finally {
                Binder.restoreCallingIdentity(token);
            }
        }

        @Override
        public void sendEvent(String event, Bundle data) {
            mHandler.post(MessageHandler.MSG_SEND_EVENT, event,
                    data == null ? null : new Bundle(data));
        }

        @Override
        public ISessionController getController() {
            return mController;
        }

        @Override
        public void setActive(boolean active) {
            mIsActive = active;
            final long token = Binder.clearCallingIdentity();
            try {
                mService.updateSession(MediaSessionRecord.this);
            } finally {
                Binder.restoreCallingIdentity(token);
            }
            mHandler.post(MessageHandler.MSG_UPDATE_SESSION_STATE);
        }

        @Override
        public void setFlags(int flags) {
            if ((flags & MediaSession.FLAG_EXCLUSIVE_GLOBAL_PRIORITY) != 0) {
                int pid = getCallingPid();
                int uid = getCallingUid();
                mService.enforcePhoneStatePermission(pid, uid);
            }
            mFlags = flags;
            if ((flags & MediaSession.FLAG_EXCLUSIVE_GLOBAL_PRIORITY) != 0) {
                final long token = Binder.clearCallingIdentity();
                try {
                    mService.setGlobalPrioritySession(MediaSessionRecord.this);
                } finally {
                    Binder.restoreCallingIdentity(token);
                }
            }
            mHandler.post(MessageHandler.MSG_UPDATE_SESSION_STATE);
        }

        @Override
        public void setMediaButtonReceiver(PendingIntent pi) {
            mMediaButtonReceiver = pi;
            final long token = Binder.clearCallingIdentity();
            try {
                mService.onMediaButtonReceiverChanged(MediaSessionRecord.this);
            } finally {
                Binder.restoreCallingIdentity(token);
            }
        }

        @Override
        public void setLaunchPendingIntent(PendingIntent pi) {
            mLaunchIntent = pi;
        }

        @Override
        public void setMetadata(MediaMetadata metadata) {
            synchronized (mLock) {
                MediaMetadata temp = metadata == null ? null : new MediaMetadata.Builder(metadata)
                        .build();
                // This is to guarantee that the underlying bundle is unparceled
                // before we set it to prevent concurrent reads from throwing an
                // exception
                if (temp != null) {
                    temp.size();
                }
                mMetadata = temp;
            }
            mHandler.post(MessageHandler.MSG_UPDATE_METADATA);
        }

        @Override
        public void setPlaybackState(PlaybackState state) {
            int oldState = mPlaybackState == null
                    ? PlaybackState.STATE_NONE : mPlaybackState.getState();
            int newState = state == null
                    ? PlaybackState.STATE_NONE : state.getState();
            synchronized (mLock) {
                mPlaybackState = state;
            }
            final long token = Binder.clearCallingIdentity();
            try {
                mService.onSessionPlaystateChanged(MediaSessionRecord.this, oldState, newState);
            } finally {
                Binder.restoreCallingIdentity(token);
            }
            mHandler.post(MessageHandler.MSG_UPDATE_PLAYBACK_STATE);
        }

        @Override
        public void setQueue(ParceledListSlice queue) {
            synchronized (mLock) {
                mQueue = queue;
            }
            mHandler.post(MessageHandler.MSG_UPDATE_QUEUE);
        }

        @Override
        public void setQueueTitle(CharSequence title) {
            mQueueTitle = title;
            mHandler.post(MessageHandler.MSG_UPDATE_QUEUE_TITLE);
        }

        @Override
        public void setExtras(Bundle extras) {
            synchronized (mLock) {
                mExtras = extras == null ? null : new Bundle(extras);
            }
            mHandler.post(MessageHandler.MSG_UPDATE_EXTRAS);
        }

        @Override
        public void setRatingType(int type) {
            mRatingType = type;
        }

        @Override
        public void setCurrentVolume(int volume) {
            mCurrentVolume = volume;
            mHandler.post(MessageHandler.MSG_UPDATE_VOLUME);
        }

        @Override
        public void setPlaybackToLocal(AudioAttributes attributes) {
            boolean typeChanged;
            synchronized (mLock) {
                typeChanged = mVolumeType == PlaybackInfo.PLAYBACK_TYPE_REMOTE;
                mVolumeType = PlaybackInfo.PLAYBACK_TYPE_LOCAL;
                if (attributes != null) {
                    mAudioAttrs = attributes;
                } else {
                    Log.e(TAG, "Received null audio attributes, using existing attributes");
                }
            }
            if (typeChanged) {
                final long token = Binder.clearCallingIdentity();
                try {
                    mService.onSessionPlaybackTypeChanged(MediaSessionRecord.this);
                } finally {
                    Binder.restoreCallingIdentity(token);
                }
                mHandler.post(MessageHandler.MSG_UPDATE_VOLUME);
            }
        }

        @Override
        public void setPlaybackToRemote(int control, int max) {
            boolean typeChanged;
            synchronized (mLock) {
                typeChanged = mVolumeType == PlaybackInfo.PLAYBACK_TYPE_LOCAL;
                mVolumeType = PlaybackInfo.PLAYBACK_TYPE_REMOTE;
                mVolumeControlType = control;
                mMaxVolume = max;
            }
            if (typeChanged) {
                final long token = Binder.clearCallingIdentity();
                try {
                    mService.onSessionPlaybackTypeChanged(MediaSessionRecord.this);
                } finally {
                    Binder.restoreCallingIdentity(token);
                }
                mHandler.post(MessageHandler.MSG_UPDATE_VOLUME);
            }
        }
    }

    class SessionCb {
        private final ISessionCallback mCb;

        public SessionCb(ISessionCallback cb) {
            mCb = cb;
        }

        public boolean sendMediaButton(String packageName, int pid, int uid,
                boolean asSystemService, KeyEvent keyEvent, int sequenceId, ResultReceiver cb) {
            try {
                if (asSystemService) {
                    mCb.onMediaButton(mContext.getPackageName(), Process.myPid(),
                            Process.SYSTEM_UID, createMediaButtonIntent(keyEvent), sequenceId, cb);
                } else {
                    mCb.onMediaButton(packageName, pid, uid,
                            createMediaButtonIntent(keyEvent), sequenceId, cb);
                }
                return true;
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in sendMediaRequest.", e);
            }
            return false;
        }

        public boolean sendMediaButton(String packageName, int pid, int uid,
                ISessionControllerCallback caller, boolean asSystemService,
                KeyEvent keyEvent) {
            try {
                if (asSystemService) {
                    mCb.onMediaButton(mContext.getPackageName(), Process.myPid(),
                            Process.SYSTEM_UID, createMediaButtonIntent(keyEvent), 0, null);
                } else {
                    mCb.onMediaButtonFromController(packageName, pid, uid, caller,
                            createMediaButtonIntent(keyEvent));
                }
                return true;
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in sendMediaRequest.", e);
            }
            return false;
        }

        public void sendCommand(String packageName, int pid, int uid,
                ISessionControllerCallback caller, String command, Bundle args, ResultReceiver cb) {
            try {
                mCb.onCommand(packageName, pid, uid, caller, command, args, cb);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in sendCommand.", e);
            }
        }

        public void sendCustomAction(String packageName, int pid, int uid,
                ISessionControllerCallback caller, String action,
                Bundle args) {
            try {
                mCb.onCustomAction(packageName, pid, uid, caller, action, args);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in sendCustomAction.", e);
            }
        }

        public void prepare(String packageName, int pid, int uid,
                ISessionControllerCallback caller) {
            try {
                mCb.onPrepare(packageName, pid, uid, caller);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in prepare.", e);
            }
        }

        public void prepareFromMediaId(String packageName, int pid, int uid,
                ISessionControllerCallback caller, String mediaId, Bundle extras) {
            try {
                mCb.onPrepareFromMediaId(packageName, pid, uid, caller, mediaId, extras);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in prepareFromMediaId.", e);
            }
        }

        public void prepareFromSearch(String packageName, int pid, int uid,
                ISessionControllerCallback caller, String query, Bundle extras) {
            try {
                mCb.onPrepareFromSearch(packageName, pid, uid, caller, query, extras);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in prepareFromSearch.", e);
            }
        }

        public void prepareFromUri(String packageName, int pid, int uid,
                ISessionControllerCallback caller, Uri uri, Bundle extras) {
            try {
                mCb.onPrepareFromUri(packageName, pid, uid, caller, uri, extras);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in prepareFromUri.", e);
            }
        }

        public void play(String packageName, int pid, int uid, ISessionControllerCallback caller) {
            try {
                mCb.onPlay(packageName, pid, uid, caller);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in play.", e);
            }
        }

        public void playFromMediaId(String packageName, int pid, int uid,
                ISessionControllerCallback caller, String mediaId, Bundle extras) {
            try {
                mCb.onPlayFromMediaId(packageName, pid, uid, caller, mediaId, extras);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in playFromMediaId.", e);
            }
        }

        public void playFromSearch(String packageName, int pid, int uid,
                ISessionControllerCallback caller, String query, Bundle extras) {
            try {
                mCb.onPlayFromSearch(packageName, pid, uid, caller, query, extras);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in playFromSearch.", e);
            }
        }

        public void playFromUri(String packageName, int pid, int uid,
                ISessionControllerCallback caller, Uri uri, Bundle extras) {
            try {
                mCb.onPlayFromUri(packageName, pid, uid, caller, uri, extras);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in playFromUri.", e);
            }
        }

        public void skipToTrack(String packageName, int pid, int uid,
                ISessionControllerCallback caller, long id) {
            try {
                mCb.onSkipToTrack(packageName, pid, uid, caller, id);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in skipToTrack", e);
            }
        }

        public void pause(String packageName, int pid, int uid, ISessionControllerCallback caller) {
            try {
                mCb.onPause(packageName, pid, uid, caller);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in pause.", e);
            }
        }

        public void stop(String packageName, int pid, int uid, ISessionControllerCallback caller) {
            try {
                mCb.onStop(packageName, pid, uid, caller);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in stop.", e);
            }
        }

        public void next(String packageName, int pid, int uid, ISessionControllerCallback caller) {
            try {
                mCb.onNext(packageName, pid, uid, caller);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in next.", e);
            }
        }

        public void previous(String packageName, int pid, int uid,
                ISessionControllerCallback caller) {
            try {
                mCb.onPrevious(packageName, pid, uid, caller);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in previous.", e);
            }
        }

        public void fastForward(String packageName, int pid, int uid,
                ISessionControllerCallback caller) {
            try {
                mCb.onFastForward(packageName, pid, uid, caller);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in fastForward.", e);
            }
        }

        public void rewind(String packageName, int pid, int uid,
                ISessionControllerCallback caller) {
            try {
                mCb.onRewind(packageName, pid, uid, caller);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in rewind.", e);
            }
        }

        public void seekTo(String packageName, int pid, int uid, ISessionControllerCallback caller,
                long pos) {
            try {
                mCb.onSeekTo(packageName, pid, uid, caller, pos);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in seekTo.", e);
            }
        }

        public void rate(String packageName, int pid, int uid, ISessionControllerCallback caller,
                Rating rating) {
            try {
                mCb.onRate(packageName, pid, uid, caller, rating);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in rate.", e);
            }
        }

        public void adjustVolume(String packageName, int pid, int uid,
                ISessionControllerCallback caller, boolean asSystemService, int direction) {
            try {
                if (asSystemService) {
                    mCb.onAdjustVolume(mContext.getPackageName(), Process.myPid(),
                            Process.SYSTEM_UID, null, direction);
                } else {
                    mCb.onAdjustVolume(packageName, pid, uid, caller, direction);
                }
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in adjustVolume.", e);
            }
        }

        public void setVolumeTo(String packageName, int pid, int uid,
                ISessionControllerCallback caller, int value) {
            try {
                mCb.onSetVolumeTo(packageName, pid, uid, caller, value);
            } catch (RemoteException e) {
                Slog.e(TAG, "Remote failure in setVolumeTo.", e);
            }
        }

        private Intent createMediaButtonIntent(KeyEvent keyEvent) {
            Intent mediaButtonIntent = new Intent(Intent.ACTION_MEDIA_BUTTON);
            mediaButtonIntent.putExtra(Intent.EXTRA_KEY_EVENT, keyEvent);
            return mediaButtonIntent;
        }
    }

    class ControllerStub extends ISessionController.Stub {
        @Override
        public void sendCommand(String packageName, ISessionControllerCallback caller,
                String command, Bundle args, ResultReceiver cb) {
            mSessionCb.sendCommand(packageName, Binder.getCallingPid(), Binder.getCallingUid(),
                    caller, command, args, cb);
        }

        @Override
        public boolean sendMediaButton(String packageName, ISessionControllerCallback cb,
                boolean asSystemService, KeyEvent keyEvent) {
            return mSessionCb.sendMediaButton(packageName, Binder.getCallingPid(),
                    Binder.getCallingUid(), cb, asSystemService, keyEvent);
        }

        @Override
        public void registerCallbackListener(String packageName, ISessionControllerCallback cb) {
            synchronized (mLock) {
                // If this session is already destroyed tell the caller and
                // don't add them.
                if (mDestroyed) {
                    try {
                        cb.onSessionDestroyed();
                    } catch (Exception e) {
                        // ignored
                    }
                    return;
                }
                if (getControllerHolderIndexForCb(cb) < 0) {
                    mControllerCallbackHolders.add(new ISessionControllerCallbackHolder(cb,
                            packageName, Binder.getCallingUid()));
                    if (DEBUG) {
                        Log.d(TAG, "registering controller callback " + cb + " from controller"
                                + packageName);
                    }
                }
            }
        }

        @Override
        public void unregisterCallbackListener(ISessionControllerCallback cb) {
            synchronized (mLock) {
                int index = getControllerHolderIndexForCb(cb);
                if (index != -1) {
                    mControllerCallbackHolders.remove(index);
                }
                if (DEBUG) {
                    Log.d(TAG, "unregistering callback " + cb.asBinder());
                }
            }
        }

        @Override
        public String getPackageName() {
            return mPackageName;
        }

        @Override
        public String getTag() {
            return mTag;
        }

        @Override
        public PendingIntent getLaunchPendingIntent() {
            return mLaunchIntent;
        }

        @Override
        public long getFlags() {
            return mFlags;
        }

        @Override
        public ParcelableVolumeInfo getVolumeAttributes() {
            int volumeType;
            AudioAttributes attributes;
            synchronized (mLock) {
                if (mVolumeType == PlaybackInfo.PLAYBACK_TYPE_REMOTE) {
                    int current = mOptimisticVolume != -1 ? mOptimisticVolume : mCurrentVolume;
                    return new ParcelableVolumeInfo(
                            mVolumeType, mAudioAttrs, mVolumeControlType, mMaxVolume, current);
                }
                volumeType = mVolumeType;
                attributes = mAudioAttrs;
            }
            int stream = AudioAttributes.toLegacyStreamType(attributes);
            int max = mAudioManager.getStreamMaxVolume(stream);
            int current = mAudioManager.getStreamVolume(stream);
            return new ParcelableVolumeInfo(
                    volumeType, attributes, VolumeProvider.VOLUME_CONTROL_ABSOLUTE, max, current);
        }

        @Override
        public void adjustVolume(String packageName, ISessionControllerCallback caller,
                boolean asSystemService, int direction, int flags) {
            int pid = Binder.getCallingPid();
            int uid = Binder.getCallingUid();
            final long token = Binder.clearCallingIdentity();
            try {
                MediaSessionRecord.this.adjustVolume(packageName, pid, uid, caller, asSystemService,
                        direction, flags, false /* useSuggested */);
            } finally {
                Binder.restoreCallingIdentity(token);
            }
        }

        @Override
        public void setVolumeTo(String packageName, ISessionControllerCallback caller,
                int value, int flags) {
            int pid = Binder.getCallingPid();
            int uid = Binder.getCallingUid();
            final long token = Binder.clearCallingIdentity();
            try {
                MediaSessionRecord.this.setVolumeTo(packageName, pid, uid, caller, value, flags);
            } finally {
                Binder.restoreCallingIdentity(token);
            }
        }

        @Override
        public void prepare(String packageName, ISessionControllerCallback caller) {
            mSessionCb.prepare(packageName, Binder.getCallingPid(), Binder.getCallingUid(), caller);
        }

        @Override
        public void prepareFromMediaId(String packageName, ISessionControllerCallback caller,
                String mediaId, Bundle extras) {
            mSessionCb.prepareFromMediaId(packageName, Binder.getCallingPid(),
                    Binder.getCallingUid(), caller, mediaId, extras);
        }

        @Override
        public void prepareFromSearch(String packageName, ISessionControllerCallback caller,
                String query, Bundle extras) {
            mSessionCb.prepareFromSearch(packageName, Binder.getCallingPid(),
                    Binder.getCallingUid(), caller, query, extras);
        }

        @Override
        public void prepareFromUri(String packageName, ISessionControllerCallback caller,
                Uri uri, Bundle extras) {
            mSessionCb.prepareFromUri(packageName, Binder.getCallingPid(), Binder.getCallingUid(),
                    caller, uri, extras);
        }

        @Override
        public void play(String packageName, ISessionControllerCallback caller) {
            mSessionCb.play(packageName, Binder.getCallingPid(), Binder.getCallingUid(), caller);
        }

        @Override
        public void playFromMediaId(String packageName, ISessionControllerCallback caller,
                String mediaId, Bundle extras) {
            mSessionCb.playFromMediaId(packageName, Binder.getCallingPid(), Binder.getCallingUid(),
                    caller, mediaId, extras);
        }

        @Override
        public void playFromSearch(String packageName, ISessionControllerCallback caller,
                String query, Bundle extras) {
            mSessionCb.playFromSearch(packageName, Binder.getCallingPid(), Binder.getCallingUid(),
                    caller, query, extras);
        }

        @Override
        public void playFromUri(String packageName, ISessionControllerCallback caller,
                Uri uri, Bundle extras) {
            mSessionCb.playFromUri(packageName, Binder.getCallingPid(), Binder.getCallingUid(),
                    caller, uri, extras);
        }

        @Override
        public void skipToQueueItem(String packageName, ISessionControllerCallback caller,
                long id) {
            mSessionCb.skipToTrack(packageName, Binder.getCallingPid(), Binder.getCallingUid(),
                    caller, id);
        }

        @Override
        public void pause(String packageName, ISessionControllerCallback caller) {
            mSessionCb.pause(packageName, Binder.getCallingPid(), Binder.getCallingUid(), caller);
        }

        @Override
        public void stop(String packageName, ISessionControllerCallback caller) {
            mSessionCb.stop(packageName, Binder.getCallingPid(), Binder.getCallingUid(), caller);
        }

        @Override
        public void next(String packageName, ISessionControllerCallback caller) {
            mSessionCb.next(packageName, Binder.getCallingPid(), Binder.getCallingUid(), caller);
        }

        @Override
        public void previous(String packageName, ISessionControllerCallback caller) {
            mSessionCb.previous(packageName, Binder.getCallingPid(), Binder.getCallingUid(),
                    caller);
        }

        @Override
        public void fastForward(String packageName, ISessionControllerCallback caller) {
            mSessionCb.fastForward(packageName, Binder.getCallingPid(), Binder.getCallingUid(),
                    caller);
        }

        @Override
        public void rewind(String packageName, ISessionControllerCallback caller) {
            mSessionCb.rewind(packageName, Binder.getCallingPid(), Binder.getCallingUid(), caller);
        }

        @Override
        public void seekTo(String packageName, ISessionControllerCallback caller, long pos) {
            mSessionCb.seekTo(packageName, Binder.getCallingPid(), Binder.getCallingUid(), caller,
                    pos);
        }

        @Override
        public void rate(String packageName, ISessionControllerCallback caller, Rating rating) {
            mSessionCb.rate(packageName, Binder.getCallingPid(), Binder.getCallingUid(), caller,
                    rating);
        }

        @Override
        public void sendCustomAction(String packageName, ISessionControllerCallback caller,
                String action, Bundle args) {
            mSessionCb.sendCustomAction(packageName, Binder.getCallingPid(), Binder.getCallingUid(),
                    caller, action, args);
        }

        @Override
        public MediaMetadata getMetadata() {
            synchronized (mLock) {
                return mMetadata;
            }
        }

        @Override
        public PlaybackState getPlaybackState() {
            return getStateWithUpdatedPosition();
        }

        @Override
        public ParceledListSlice getQueue() {
            synchronized (mLock) {
                return mQueue;
            }
        }

        @Override
        public CharSequence getQueueTitle() {
            return mQueueTitle;
        }

        @Override
        public Bundle getExtras() {
            synchronized (mLock) {
                return mExtras;
            }
        }

        @Override
        public int getRatingType() {
            return mRatingType;
        }

        @Override
        public boolean isTransportControlEnabled() {
            return MediaSessionRecord.this.isTransportControlEnabled();
        }
    }

    private class ISessionControllerCallbackHolder {
        private final ISessionControllerCallback mCallback;
        private final String mPackageName;
        private final int mUid;

        ISessionControllerCallbackHolder(ISessionControllerCallback callback, String packageName,
                int uid) {
            mCallback = callback;
            mPackageName = packageName;
            mUid = uid;
        }
    }

    private class MessageHandler extends Handler {
        private static final int MSG_UPDATE_METADATA = 1;
        private static final int MSG_UPDATE_PLAYBACK_STATE = 2;
        private static final int MSG_UPDATE_QUEUE = 3;
        private static final int MSG_UPDATE_QUEUE_TITLE = 4;
        private static final int MSG_UPDATE_EXTRAS = 5;
        private static final int MSG_SEND_EVENT = 6;
        private static final int MSG_UPDATE_SESSION_STATE = 7;
        private static final int MSG_UPDATE_VOLUME = 8;
        private static final int MSG_DESTROYED = 9;

        public MessageHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_UPDATE_METADATA:
                    pushMetadataUpdate();
                    break;
                case MSG_UPDATE_PLAYBACK_STATE:
                    pushPlaybackStateUpdate();
                    break;
                case MSG_UPDATE_QUEUE:
                    pushQueueUpdate();
                    break;
                case MSG_UPDATE_QUEUE_TITLE:
                    pushQueueTitleUpdate();
                    break;
                case MSG_UPDATE_EXTRAS:
                    pushExtrasUpdate();
                    break;
                case MSG_SEND_EVENT:
                    pushEvent((String) msg.obj, msg.getData());
                    break;
                case MSG_UPDATE_SESSION_STATE:
                    // TODO add session state
                    break;
                case MSG_UPDATE_VOLUME:
                    pushVolumeUpdate();
                    break;
                case MSG_DESTROYED:
                    pushSessionDestroyed();
            }
        }

        public void post(int what) {
            post(what, null);
        }

        public void post(int what, Object obj) {
            obtainMessage(what, obj).sendToTarget();
        }

        public void post(int what, Object obj, Bundle data) {
            Message msg = obtainMessage(what, obj);
            msg.setData(data);
            msg.sendToTarget();
        }
    }

}
