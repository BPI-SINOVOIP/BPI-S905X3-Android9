/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.media;

import android.os.Bundle;
import android.os.ResultReceiver;
import android.net.Uri;

import com.android.media.IMediaController2;

/**
 * Interface from MediaController2 to MediaSession2.
 * <p>
 * Keep this interface oneway. Otherwise a malicious app may implement fake version of this,
 * and holds calls from session to make session owner(s) frozen.
 */
 // TODO(jaewan): (Post P) Handle when the playlist becomes too huge.
 //               Note that ParcelledSliceList isn't a good idea for the purpose. (see: b/37493677)
oneway interface IMediaSession2 {
    // TODO(jaewan): add onCommand() to send private command

    // TODO(jaewan): (Post P) We may consider to add another binder just for the connection
    //               not to expose other methods to the controller whose connection wasn't accepted.
    //               But this would be enough for now because it's the same as existing
    //               MediaBrowser and MediaBrowserService.
    void connect(IMediaController2 caller, String callingPackage);
    void release(IMediaController2 caller);

    void setVolumeTo(IMediaController2 caller, int value, int flags);
    void adjustVolume(IMediaController2 caller, int direction, int flags);

    //////////////////////////////////////////////////////////////////////////////////////////////
    // send command
    //////////////////////////////////////////////////////////////////////////////////////////////
    void sendTransportControlCommand(IMediaController2 caller,
            int commandCode, in Bundle args);
    void sendCustomCommand(IMediaController2 caller, in Bundle command, in Bundle args,
            in ResultReceiver receiver);

    void prepareFromUri(IMediaController2 caller, in Uri uri, in Bundle extras);
    void prepareFromSearch(IMediaController2 caller, String query, in Bundle extras);
    void prepareFromMediaId(IMediaController2 caller, String mediaId, in Bundle extras);
    void playFromUri(IMediaController2 caller, in Uri uri, in Bundle extras);
    void playFromSearch(IMediaController2 caller, String query, in Bundle extras);
    void playFromMediaId(IMediaController2 caller, String mediaId, in Bundle extras);
    void setRating(IMediaController2 caller, String mediaId, in Bundle rating);

    void setPlaylist(IMediaController2 caller, in List<Bundle> playlist, in Bundle metadata);
    void updatePlaylistMetadata(IMediaController2 caller, in Bundle metadata);
    void addPlaylistItem(IMediaController2 caller, int index, in Bundle mediaItem);
    void removePlaylistItem(IMediaController2 caller, in Bundle mediaItem);
    void replacePlaylistItem(IMediaController2 caller, int index, in Bundle mediaItem);
    void skipToPlaylistItem(IMediaController2 caller, in Bundle mediaItem);
    void skipToPreviousItem(IMediaController2 caller);
    void skipToNextItem(IMediaController2 caller);
    void setRepeatMode(IMediaController2 caller, int repeatMode);
    void setShuffleMode(IMediaController2 caller, int shuffleMode);

    //////////////////////////////////////////////////////////////////////////////////////////////
    // library service specific
    //////////////////////////////////////////////////////////////////////////////////////////////
    void getLibraryRoot(IMediaController2 caller, in Bundle rootHints);
    void getItem(IMediaController2 caller, String mediaId);
    void getChildren(IMediaController2 caller, String parentId, int page, int pageSize,
            in Bundle extras);
    void search(IMediaController2 caller, String query, in Bundle extras);
    void getSearchResult(IMediaController2 caller, String query, int page, int pageSize,
            in Bundle extras);
    void subscribe(IMediaController2 caller, String parentId, in Bundle extras);
    void unsubscribe(IMediaController2 caller, String parentId);
}
