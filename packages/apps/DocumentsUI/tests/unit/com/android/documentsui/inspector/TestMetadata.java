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
package com.android.documentsui.inspector;

import android.media.ExifInterface;
import android.media.MediaMetadata;
import android.os.Bundle;
import android.provider.DocumentsContract;

import com.android.documentsui.base.Shared;

final class TestMetadata {
    private TestMetadata() {}

    static void populateExifData(Bundle container) {
        Bundle data = new Bundle();
        container.putBundle(DocumentsContract.METADATA_EXIF, data);

        data.putInt(ExifInterface.TAG_IMAGE_WIDTH, 3840);
        data.putInt(ExifInterface.TAG_IMAGE_LENGTH, 2160);
        data.putString(ExifInterface.TAG_DATETIME, "Jan 01, 1970, 12:16 AM");
        data.putString(ExifInterface.TAG_GPS_LATITUDE, "33/1,59/1,4530/100");
        data.putString(ExifInterface.TAG_GPS_LONGITUDE, "118/1,28/1,3124/100");
        data.putString(ExifInterface.TAG_GPS_LATITUDE_REF, "N");
        data.putString(ExifInterface.TAG_GPS_LONGITUDE_REF, "W");
        data.putDouble(ExifInterface.TAG_GPS_ALTITUDE, 1244);
        data.putString(ExifInterface.TAG_MAKE, "Google");
        data.putString(ExifInterface.TAG_MODEL, "Pixel");
        data.putDouble(ExifInterface.TAG_SHUTTER_SPEED_VALUE, 6.643);
        data.putDouble(ExifInterface.TAG_APERTURE, 2.0);
        data.putInt(ExifInterface.TAG_ISO_SPEED_RATINGS, 120);
        data.putDouble(ExifInterface.TAG_FOCAL_LENGTH, 4.27);
    }

    static void populateVideoData(Bundle container) {
        Bundle data = new Bundle();
        container.putBundle(Shared.METADATA_KEY_VIDEO, data);

        // By convention we reuse exif tags for dimensions.
        data.putInt(ExifInterface.TAG_IMAGE_WIDTH, 1920);
        data.putInt(ExifInterface.TAG_IMAGE_LENGTH, 1080);
        data.putInt(MediaMetadata.METADATA_KEY_DURATION, 72000);
    }

    static void populateAudioData(Bundle container) {
        Bundle data = new Bundle();
        container.putBundle(Shared.METADATA_KEY_AUDIO, data);

        data.putInt(MediaMetadata.METADATA_KEY_DURATION, 72000);
        data.putString(MediaMetadata.METADATA_KEY_ARTIST, "artist");
        data.putString(MediaMetadata.METADATA_KEY_COMPOSER, "composer");
        data.putString(MediaMetadata.METADATA_KEY_ALBUM, "album");
    }
}
