/* Copyright (C) 2014 The Android Open Source Project
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

package android.media.session;

import android.content.Intent;
import android.media.Rating;
import android.media.session.ISessionControllerCallback;
import android.net.Uri;
import android.os.Bundle;
import android.os.ResultReceiver;

/**
 * @hide
 */
oneway interface ISessionCallback {
    void onCommand(String packageName, int pid, int uid, ISessionControllerCallback caller,
            String command, in Bundle args, in ResultReceiver cb);
    void onMediaButton(String packageName, int pid, int uid, in Intent mediaButtonIntent,
            int sequenceNumber, in ResultReceiver cb);
    void onMediaButtonFromController(String packageName, int pid, int uid,
            ISessionControllerCallback caller, in Intent mediaButtonIntent);

    // These callbacks are for the TransportPerformer
    void onPrepare(String packageName, int pid, int uid, ISessionControllerCallback caller);
    void onPrepareFromMediaId(String packageName, int pid, int uid,
            ISessionControllerCallback caller, String mediaId, in Bundle extras);
    void onPrepareFromSearch(String packageName, int pid, int uid,
            ISessionControllerCallback caller, String query, in Bundle extras);
    void onPrepareFromUri(String packageName, int pid, int uid, ISessionControllerCallback caller,
            in Uri uri, in Bundle extras);
    void onPlay(String packageName, int pid, int uid, ISessionControllerCallback caller);
    void onPlayFromMediaId(String packageName, int pid, int uid, ISessionControllerCallback caller,
            String mediaId, in Bundle extras);
    void onPlayFromSearch(String packageName, int pid, int uid, ISessionControllerCallback caller,
            String query, in Bundle extras);
    void onPlayFromUri(String packageName, int pid, int uid, ISessionControllerCallback caller,
            in Uri uri, in Bundle extras);
    void onSkipToTrack(String packageName, int pid, int uid, ISessionControllerCallback caller,
            long id);
    void onPause(String packageName, int pid, int uid, ISessionControllerCallback caller);
    void onStop(String packageName, int pid, int uid, ISessionControllerCallback caller);
    void onNext(String packageName, int pid, int uid, ISessionControllerCallback caller);
    void onPrevious(String packageName, int pid, int uid, ISessionControllerCallback caller);
    void onFastForward(String packageName, int pid, int uid, ISessionControllerCallback caller);
    void onRewind(String packageName, int pid, int uid, ISessionControllerCallback caller);
    void onSeekTo(String packageName, int pid, int uid, ISessionControllerCallback caller,
            long pos);
    void onRate(String packageName, int pid, int uid, ISessionControllerCallback caller,
            in Rating rating);
    void onCustomAction(String packageName, int pid, int uid, ISessionControllerCallback caller,
            String action, in Bundle args);

    // These callbacks are for volume handling
    void onAdjustVolume(String packageName, int pid, int uid, ISessionControllerCallback caller,
            int direction);
    void onSetVolumeTo(String packageName, int pid, int uid,
            ISessionControllerCallback caller, int value);
}
