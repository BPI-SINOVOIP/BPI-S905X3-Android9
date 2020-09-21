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

import static com.android.internal.util.Preconditions.checkNotNull;

import android.media.ExifInterface;
import android.os.Bundle;
import android.provider.DocumentsContract;

import com.android.documentsui.base.Shared;

import javax.annotation.Nullable;

final class MetadataUtils {

    private MetadataUtils() {}

    static boolean hasGeoCoordinates(@Nullable Bundle metadata) {
        if (metadata == null) {
            return false;
        }
        return hasVideoCoordinates(metadata.getBundle(Shared.METADATA_KEY_VIDEO))
                || hasExifGpsFields(metadata.getBundle(DocumentsContract.METADATA_EXIF));
    }

    static boolean hasVideoCoordinates(@Nullable Bundle data) {
        return data != null && (data.containsKey(Shared.METADATA_VIDEO_LATITUDE)
                && data.containsKey(Shared.METADATA_VIDEO_LONGITUTE));
    }

    static boolean hasExifGpsFields(@Nullable Bundle exif) {
        return exif != null && (exif.containsKey(ExifInterface.TAG_GPS_LATITUDE)
                && exif.containsKey(ExifInterface.TAG_GPS_LONGITUDE)
                && exif.containsKey(ExifInterface.TAG_GPS_LATITUDE_REF)
                && exif.containsKey(ExifInterface.TAG_GPS_LONGITUDE_REF));
    }

    static float[] getGeoCoordinates(Bundle metadata) {
        assert hasGeoCoordinates(metadata);
        checkNotNull(metadata);

        Bundle bundle = metadata.getBundle(DocumentsContract.METADATA_EXIF);
        if (hasExifGpsFields(bundle)) {
            return getExifGpsCoords(bundle);
        }

        bundle = metadata.getBundle(Shared.METADATA_KEY_VIDEO);
        if (hasVideoCoordinates(bundle)) {
            return getVideoCoords(bundle);
        }

        // This should never happen, because callers should always check w/ hasGeoCoordinates first.
        throw new IllegalArgumentException("Invalid metadata bundle: " + metadata);
    }

    static float[] getExifGpsCoords(Bundle exif) {
        assert hasExifGpsFields(exif);

        String lat = exif.getString(ExifInterface.TAG_GPS_LATITUDE);
        String lon = exif.getString(ExifInterface.TAG_GPS_LONGITUDE);
        String latRef = exif.getString(ExifInterface.TAG_GPS_LATITUDE_REF);
        String lonRef = exif.getString(ExifInterface.TAG_GPS_LONGITUDE_REF);

        return new float[] {
            ExifInterface.convertRationalLatLonToFloat(lat, latRef),
            ExifInterface.convertRationalLatLonToFloat(lon, lonRef)
        };
    }

    static float[] getVideoCoords(Bundle data) {
        assert hasVideoCoordinates(data);
        return new float[] {
                data.getFloat(Shared.METADATA_VIDEO_LATITUDE),
                data.getFloat(Shared.METADATA_VIDEO_LONGITUTE)
        };
    }
}
