/**
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.broadcastradio.support.platform;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.hardware.radio.ProgramSelector;
import android.hardware.radio.ProgramSelector.Identifier;
import android.hardware.radio.RadioManager;
import android.net.Uri;
import android.util.Log;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiConsumer;
import java.util.function.BiFunction;

/**
 * Proposed extensions to android.hardware.radio.ProgramSelector.
 *
 * They might eventually get pushed to the framework.
 */
public class ProgramSelectorExt {
    private static final String TAG = "BcRadioApp.pselext";

    /**
     * If this is AM/FM channel (or any other technology using different modulations),
     * don't return modulation part.
     */
    public static final int NAME_NO_MODULATION = 1 << 0;

    /**
     * Return only modulation part of channel name.
     *
     * If this is not a radio technology using modulation, return nothing
     * (unless combined with other _ONLY flags in the future).
     *
     * If this returns non-null string, it's guaranteed that {@link #NAME_NO_MODULATION}
     * will return the complement of channel name.
     */
    public static final int NAME_MODULATION_ONLY = 1 << 1;

    /**
     * Flags to control how channel values are converted to string with {@link #getDisplayName}.
     *
     * Upper 16 bits are reserved for {@link ProgramInfoExt#NameFlag}.
     */
    @IntDef(prefix = { "NAME_" }, flag = true, value = {
        NAME_NO_MODULATION,
        NAME_MODULATION_ONLY,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface NameFlag {}

    private static final String URI_SCHEME_BROADCASTRADIO = "broadcastradio";
    private static final String URI_AUTHORITY_PROGRAM = "program";
    private static final String URI_VENDOR_PREFIX = "VENDOR_";
    private static final String URI_HEX_PREFIX = "0x";

    private static final DecimalFormat FORMAT_FM = new DecimalFormat("###.#");

    private static final Map<Integer, String> ID_TO_URI = new HashMap<>();
    private static final Map<String, Integer> URI_TO_ID = new HashMap<>();

    /**
     * New proposed constructor for {@link ProgramSelector}.
     *
     * As opposed to the current platform API, this one matches more closely simplified HAL 2.0.
     *
     * @param primaryId primary program identifier.
     * @param secondaryIds list of secondary program identifiers.
     */
    public static @NonNull ProgramSelector newProgramSelector(@NonNull Identifier primaryId,
            @Nullable Identifier[] secondaryIds) {
        return new ProgramSelector(
                identifierToProgramType(primaryId),
                primaryId, secondaryIds, null);
    }

    // when pushed to the framework, remove similar code from HAL 2.0 service
    private static @ProgramSelector.ProgramType int identifierToProgramType(
            @NonNull Identifier primaryId) {
        int idType = primaryId.getType();
        switch (idType) {
            case ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY:
                if (isAmFrequency(primaryId.getValue())) {
                    return ProgramSelector.PROGRAM_TYPE_AM;
                } else {
                    return ProgramSelector.PROGRAM_TYPE_FM;
                }
            case ProgramSelector.IDENTIFIER_TYPE_RDS_PI:
                return ProgramSelector.PROGRAM_TYPE_FM;
            case ProgramSelector.IDENTIFIER_TYPE_HD_STATION_ID_EXT:
                if (isAmFrequency(IdentifierExt.asHdPrimary(primaryId).getFrequency())) {
                    return ProgramSelector.PROGRAM_TYPE_AM_HD;
                } else {
                    return ProgramSelector.PROGRAM_TYPE_FM_HD;
                }
            case ProgramSelector.IDENTIFIER_TYPE_DAB_SIDECC:
            case ProgramSelector.IDENTIFIER_TYPE_DAB_ENSEMBLE:
            case ProgramSelector.IDENTIFIER_TYPE_DAB_SCID:
            case ProgramSelector.IDENTIFIER_TYPE_DAB_FREQUENCY:
                return ProgramSelector.PROGRAM_TYPE_DAB;
            case ProgramSelector.IDENTIFIER_TYPE_DRMO_SERVICE_ID:
            case ProgramSelector.IDENTIFIER_TYPE_DRMO_FREQUENCY:
                return ProgramSelector.PROGRAM_TYPE_DRMO;
            case ProgramSelector.IDENTIFIER_TYPE_SXM_SERVICE_ID:
            case ProgramSelector.IDENTIFIER_TYPE_SXM_CHANNEL:
                return ProgramSelector.PROGRAM_TYPE_SXM;
        }
        if (idType >= ProgramSelector.IDENTIFIER_TYPE_VENDOR_PRIMARY_START
                && idType <= ProgramSelector.IDENTIFIER_TYPE_VENDOR_PRIMARY_END) {
            return idType;
        }
        return ProgramSelector.PROGRAM_TYPE_INVALID;
    }

    /**
     * Checks, if a given AM frequency is roughly valid and in correct unit.
     *
     * It does not check the range precisely: it may provide false positives, but not false
     * negatives. In particular, it may be way off for certain regions.
     * The main purpose is to avoid passing inproper units, ie. MHz instead of kHz.
     * It also can be used to check if a given frequency is likely to be used
     * with AM or FM modulation.
     *
     * @param frequencyKhz the frequency in kHz.
     * @return true, if the frequency is rougly valid.
     */
    public static boolean isAmFrequency(long frequencyKhz) {
        return frequencyKhz > 150 && frequencyKhz <= 30000;
    }

    /**
     * Checks, if a given FM frequency is roughly valid and in correct unit.
     *
     * It does not check the range precisely: it may provide false positives, but not false
     * negatives. In particular, it may be way off for certain regions.
     * The main purpose is to avoid passing inproper units, ie. MHz instead of kHz.
     * It also can be used to check if a given frequency is likely to be used
     * with AM or FM modulation.
     *
     * @param frequencyKhz the frequency in kHz.
     * @return true, if the frequency is rougly valid.
     */
    public static boolean isFmFrequency(long frequencyKhz) {
        return frequencyKhz > 60000 && frequencyKhz < 110000;
    }

    /**
     * Provides human-readable representation of AM/FM frequency.
     *
     * @param frequencyKhz the frequency in kHz.
     * @param flags flags that affect display format
     * @return human-readable formatted frequency
     */
    public static @Nullable String formatAmFmFrequency(long frequencyKhz, @NameFlag int flags) {
        String channel;
        String modulation;

        if (isAmFrequency(frequencyKhz)) {
            channel = Long.toString(frequencyKhz);
            modulation = "AM";
        } else if (isFmFrequency(frequencyKhz)) {
            channel = FORMAT_FM.format(frequencyKhz / 1000f);
            modulation = "FM";
        } else {
            Log.w(TAG, "AM/FM frequency out of range: " + frequencyKhz);
            return null;
        }

        if ((flags & NAME_MODULATION_ONLY) != 0) return modulation;
        if ((flags & NAME_NO_MODULATION) != 0) return channel;
        return channel + ' ' + modulation;
    }

    /**
     * Builds new ProgramSelector for AM/FM frequency.
     *
     * @param frequencyKhz the frequency in kHz.
     * @return new ProgramSelector object representing given frequency.
     * @throws IllegalArgumentException if provided frequency is out of bounds.
     */
    public static @NonNull ProgramSelector createAmFmSelector(long frequencyKhz) {
        if (frequencyKhz < 0 || frequencyKhz > Integer.MAX_VALUE) {
            throw new IllegalArgumentException("illegal frequency value: " + frequencyKhz);
        }
        return ProgramSelector.createAmFmSelector(RadioManager.BAND_INVALID, (int) frequencyKhz);
    }

    /**
     * Checks, if {@link ProgramSelector} contains an id of a given type.
     *
     * @param sel selector being checked
     * @param type identifier type to check for
     * @return true, if sel contains any identifier of a given type
     */
    public static boolean hasId(@NonNull ProgramSelector sel,
            @ProgramSelector.IdentifierType int type) {
        try {
            sel.getFirstId(type);
            return true;
        } catch (IllegalArgumentException e) {
            return false;
        }
    }

    /**
     * Checks, if {@link ProgramSelector} is a AM/FM program.
     *
     * @return true, if the primary identifier of a selector belongs to one of the following
     *         technologies:
     *          - Analogue AM/FM
     *          - FM RDS
     *          - HD Radio AM/FM
     */
    public static boolean isAmFmProgram(@NonNull ProgramSelector sel) {
        int priType = sel.getPrimaryId().getType();
        return priType == ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY
                || priType == ProgramSelector.IDENTIFIER_TYPE_RDS_PI
                || priType == ProgramSelector.IDENTIFIER_TYPE_HD_STATION_ID_EXT;
    }

    /**
     * Returns a channel name that can be displayed to the user.
     *
     * It's implemented only for radio technologies where the channel is meant
     * to be presented to the user.
     *
     * @param sel the program selector
     * @return Channel name or null, if radio technology doesn't present channel names to the user.
     */
    public static @Nullable String getDisplayName(@NonNull ProgramSelector sel,
            @NameFlag int flags) {
        if (isAmFmProgram(sel)) {
            if (!hasId(sel, ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY)) return null;
            long freq = sel.getFirstId(ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY);
            return formatAmFmFrequency(freq, flags);
        }

        if ((flags & NAME_MODULATION_ONLY) != 0) return null;

        if (sel.getPrimaryId().getType() == ProgramSelector.IDENTIFIER_TYPE_SXM_SERVICE_ID) {
            if (!hasId(sel, ProgramSelector.IDENTIFIER_TYPE_SXM_CHANNEL)) return null;
            return Long.toString(sel.getFirstId(ProgramSelector.IDENTIFIER_TYPE_SXM_CHANNEL));
        }

        return null;
    }

    static {
        BiConsumer<Integer, String> add = (idType, name) -> {
            ID_TO_URI.put(idType, name);
            URI_TO_ID.put(name, idType);
        };

        add.accept(ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY, "AMFM_FREQUENCY");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_RDS_PI, "RDS_PI");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_HD_STATION_ID_EXT, "HD_STATION_ID_EXT");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_HD_STATION_NAME, "HD_STATION_NAME");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_DAB_SID_EXT, "DAB_SID_EXT");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_DAB_ENSEMBLE, "DAB_ENSEMBLE");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_DAB_SCID, "DAB_SCID");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_DAB_FREQUENCY, "DAB_FREQUENCY");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_DRMO_SERVICE_ID, "DRMO_SERVICE_ID");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_DRMO_FREQUENCY, "DRMO_FREQUENCY");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_SXM_SERVICE_ID, "SXM_SERVICE_ID");
        add.accept(ProgramSelector.IDENTIFIER_TYPE_SXM_CHANNEL, "SXM_CHANNEL");
    }

    private static @Nullable String typeToUri(int identifierType) {
        if (identifierType >= ProgramSelector.IDENTIFIER_TYPE_VENDOR_START
                && identifierType <= ProgramSelector.IDENTIFIER_TYPE_VENDOR_END) {
            int idx = identifierType - ProgramSelector.IDENTIFIER_TYPE_VENDOR_START;
            return URI_VENDOR_PREFIX + idx;
        }
        return ID_TO_URI.get(identifierType);
    }

    private static int uriToType(@Nullable String typeUri) {
        if (typeUri == null) return ProgramSelector.IDENTIFIER_TYPE_INVALID;
        if (typeUri.startsWith(URI_VENDOR_PREFIX)) {
            int idx;
            try {
                idx = Integer.parseInt(typeUri.substring(URI_VENDOR_PREFIX.length()));
            } catch (NumberFormatException ex) {
                return ProgramSelector.IDENTIFIER_TYPE_INVALID;
            }
            if (idx > ProgramSelector.IDENTIFIER_TYPE_VENDOR_END
                    - ProgramSelector.IDENTIFIER_TYPE_VENDOR_START) {
                return ProgramSelector.IDENTIFIER_TYPE_INVALID;
            }
            return ProgramSelector.IDENTIFIER_TYPE_VENDOR_START + idx;
        }
        return URI_TO_ID.get(typeUri);
    }

    private static @NonNull String valueToUri(@NonNull Identifier id) {
        long val = id.getValue();
        switch (id.getType()) {
            case ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY:
            case ProgramSelector.IDENTIFIER_TYPE_DAB_FREQUENCY:
            case ProgramSelector.IDENTIFIER_TYPE_DRMO_FREQUENCY:
            case ProgramSelector.IDENTIFIER_TYPE_SXM_CHANNEL:
                return Long.toString(val);
            default:
                return URI_HEX_PREFIX + Long.toHexString(val);
        }
    }

    private static @Nullable Long uriToValue(@Nullable String valUri) {
        if (valUri == null) return null;
        try {
            if (valUri.startsWith(URI_HEX_PREFIX)) {
                return Long.parseLong(valUri.substring(URI_HEX_PREFIX.length()), 16);
            } else {
                return Long.parseLong(valUri, 10);
            }
        } catch (NumberFormatException ex) {
            return null;
        }
    }

    /**
     * Serialize {@link ProgramSelector} to URI.
     *
     * @param sel selector to serialize
     * @return serialized form of selector
     */
    public static @Nullable Uri toUri(@NonNull ProgramSelector sel) {
        Identifier pri = sel.getPrimaryId();
        String priType = typeToUri(pri.getType());
        // unsupported primary identifier, might be from future HAL revision
        if (priType == null) return null;

        Uri.Builder uri = new Uri.Builder()
                .scheme(URI_SCHEME_BROADCASTRADIO)
                .authority(URI_AUTHORITY_PROGRAM)
                .appendPath(priType)
                .appendPath(valueToUri(pri));

        for (Identifier sec : sel.getSecondaryIds()) {
            String secType = typeToUri(sec.getType());
            if (secType == null) continue;  // skip unsupported secondary identifier
            uri.appendQueryParameter(secType, valueToUri(sec));
        }
        return uri.build();
    }

    /**
     * Parse serialized {@link ProgramSelector}.
     *
     * @param uri URI-zed form of ProgramSelector
     * @return de-serialized object or null, if couldn't parse
     */
    public static @Nullable ProgramSelector fromUri(@Nullable Uri uri) {
        if (uri == null) return null;

        if (!URI_SCHEME_BROADCASTRADIO.equals(uri.getScheme())) return null;
        if (!URI_AUTHORITY_PROGRAM.equals(uri.getAuthority())) {
            Log.w(TAG, "Unknown URI authority part (might be a future, unsupported version): "
                    + uri.getAuthority());
            return null;
        }

        BiFunction<String, String, Identifier> parseComponents = (typeStr, valueStr) -> {
            int type = uriToType(typeStr);
            Long value = uriToValue(valueStr);
            if (type == ProgramSelector.IDENTIFIER_TYPE_INVALID || value == null) return null;
            return new Identifier(type, value);
        };

        List<String> priUri = uri.getPathSegments();
        if (priUri.size() != 2) return null;
        Identifier pri = parseComponents.apply(priUri.get(0), priUri.get(1));
        if (pri == null) return null;

        String query = uri.getQuery();
        List<Identifier> secIds = new ArrayList<>();
        if (query != null) {
            for (String secPair : query.split("&")) {
                String[] secStr = secPair.split("=");
                if (secStr.length != 2) continue;
                Identifier sec = parseComponents.apply(secStr[0], secStr[1]);
                if (sec != null) secIds.add(sec);
            }
        }

        return newProgramSelector(pri, secIds.toArray(new Identifier[secIds.size()]));
    }

    /**
     * Proposed extensions to android.hardware.radio.ProgramSelector.Identifier.
     *
     * They might eventually get pushed to the framework.
     */
    public static class IdentifierExt {
        /**
         * Decode {@link ProgramSelector#IDENTIFIER_TYPE_HD_STATION_ID_EXT} value.
         *
         * @param id identifier to decode
         * @return value decoder
         */
        public static @Nullable HdPrimary asHdPrimary(@NonNull Identifier id) {
            if (id.getType() == ProgramSelector.IDENTIFIER_TYPE_HD_STATION_ID_EXT) {
                return new HdPrimary(id.getValue());
            }
            return null;
        }

        /**
         * Decoder of {@link ProgramSelector#IDENTIFIER_TYPE_HD_STATION_ID_EXT} value.
         *
         * When pushed to the framework, it will be non-static class referring
         * to the original value.
         */
        public static class HdPrimary {
            /* For mValue format (bit shifts and bit masks), please refer to
             * HD_STATION_ID_EXT from broadcastradio HAL 2.0.
             */
            private final long mValue;

            private HdPrimary(long value) {
                mValue = value;
            }

            public long getStationId() {
                return mValue & 0xFFFFFFFF;
            }

            public int getSubchannel() {
                return (int) ((mValue >>> 32) & 0xF);
            }

            public int getFrequency() {
                return (int) ((mValue >>> (32 + 4)) & 0x3FFFF);
            }
        }
    }
}
