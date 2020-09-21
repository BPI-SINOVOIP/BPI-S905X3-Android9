/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

package com.droidlogic;

import android.annotation.IntDef;
import android.annotation.Nullable;
import android.annotation.StringDef;
import android.util.Log;
import android.util.SparseArray;
import android.util.Xml;

import com.android.internal.util.HexDump;

import java.io.IOException;
import java.io.InputStream;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Objects;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

public final class HdmiUtils {

    private static final String TAG = "HdmiUtils";

	private HdmiUtils() { /* cannot be instantiated */ }

    public static class Constants {
        // Audio Format Codes
        // Refer to CEA Standard (CEA-861-D), Table 37 Audio Format Codes.
        @Retention(RetentionPolicy.SOURCE)
        @IntDef({
            AUDIO_CODEC_NONE,
            AUDIO_CODEC_LPCM,
            AUDIO_CODEC_DD,
            AUDIO_CODEC_MPEG1,
            AUDIO_CODEC_MP3,
            AUDIO_CODEC_MPEG2,
            AUDIO_CODEC_AAC,
            AUDIO_CODEC_DTS,
            AUDIO_CODEC_ATRAC,
            AUDIO_CODEC_ONEBITAUDIO,
            AUDIO_CODEC_DDP,
            AUDIO_CODEC_DTSHD,
            AUDIO_CODEC_TRUEHD,
            AUDIO_CODEC_DST,
            AUDIO_CODEC_WMAPRO,
            AUDIO_CODEC_MAX,
        })
        public @interface AudioCodec {}

        static final int AUDIO_CODEC_NONE = 0x0;
        static final int AUDIO_CODEC_LPCM = 0x1; // Support LPCMs
        static final int AUDIO_CODEC_DD = 0x2; // Support DD
        static final int AUDIO_CODEC_MPEG1 = 0x3; // Support MPEG1
        static final int AUDIO_CODEC_MP3 = 0x4; // Support MP3
        static final int AUDIO_CODEC_MPEG2 = 0x5; // Support MPEG2
        static final int AUDIO_CODEC_AAC = 0x6; // Support AAC
        static final int AUDIO_CODEC_DTS = 0x7; // Support DTS
        static final int AUDIO_CODEC_ATRAC = 0x8; // Support ATRAC
        static final int AUDIO_CODEC_ONEBITAUDIO = 0x9; // Support One-Bit Audio
        static final int AUDIO_CODEC_DDP = 0xA; // Support DDP
        static final int AUDIO_CODEC_DTSHD = 0xB; // Support DTSHD
        static final int AUDIO_CODEC_TRUEHD = 0xC; // Support MLP/TRUE-HD
        static final int AUDIO_CODEC_DST = 0xD; // Support DST
        static final int AUDIO_CODEC_WMAPRO = 0xE; // Support WMA-Pro
        static final int AUDIO_CODEC_MAX = 0xF;

        @StringDef({
            AUDIO_DEVICE_ARC_IN,
            AUDIO_DEVICE_SPDIF,
        })
        public @interface AudioDevice {}

        static final String AUDIO_DEVICE_ARC_IN = "ARC_IN";
        static final String AUDIO_DEVICE_SPDIF = "SPDIF";
    }

	public static class ShortAudioDescriptorXmlParser {
        // We don't use namespaces
        private static final String NS = null;

        // return a list of devices config
        public static List<DeviceConfig> parse(InputStream in)
                throws XmlPullParserException, IOException {
            XmlPullParser parser = Xml.newPullParser();
            parser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, false);
            parser.setInput(in, null);
            parser.nextTag();
            return readDevices(parser);
        }

        private static void skip(XmlPullParser parser) throws XmlPullParserException, IOException {
            if (parser.getEventType() != XmlPullParser.START_TAG) {
                throw new IllegalStateException();
            }
            int depth = 1;
            while (depth != 0) {
                switch (parser.next()) {
                    case XmlPullParser.END_TAG:
                        depth--;
                        break;
                    case XmlPullParser.START_TAG:
                        depth++;
                        break;
                }
            }
        }

        private static List<DeviceConfig> readDevices(XmlPullParser parser)
                throws XmlPullParserException, IOException {
            List<DeviceConfig> devices = new ArrayList<>();

            parser.require(XmlPullParser.START_TAG, NS, "config");
            while (parser.next() != XmlPullParser.END_TAG) {
                if (parser.getEventType() != XmlPullParser.START_TAG) {
                    continue;
                }
                String name = parser.getName();
                // Starts by looking for the device tag
                if (name.equals("device")) {
                    String deviceType = parser.getAttributeValue(null, "type");
                    DeviceConfig config = null;
                    if (deviceType != null) {
                        config = readDeviceConfig(parser, deviceType);
                    }
                    if (config != null) {
                        devices.add(config);
                    }
                } else {
                    skip(parser);
                }
            }
            return devices;
        }

        // Processes device tags in the config.
        @Nullable
        private static DeviceConfig readDeviceConfig(XmlPullParser parser, String deviceType)
                throws XmlPullParserException, IOException {
            List<CodecSad> codecSads = new ArrayList<>();
            int format;
            byte[] descriptor;

            parser.require(XmlPullParser.START_TAG, NS, "device");
            while (parser.next() != XmlPullParser.END_TAG) {
                if (parser.getEventType() != XmlPullParser.START_TAG) {
                    continue;
                }
                String tagName = parser.getName();

                // Starts by looking for the supportedFormat tag
                if (tagName.equals("supportedFormat")) {
                    String codecAttriValue = parser.getAttributeValue(null, "format");
                    String sadAttriValue = parser.getAttributeValue(null, "descriptor");
                    format = (codecAttriValue) == null
                            ? Constants.AUDIO_CODEC_NONE : formatNameToNum(codecAttriValue);
                    descriptor = readSad(sadAttriValue);
                    if (format != Constants.AUDIO_CODEC_NONE && descriptor != null) {
                        codecSads.add(new CodecSad(format, descriptor));
                    }
                    parser.nextTag();
                    parser.require(XmlPullParser.END_TAG, NS, "supportedFormat");
                } else {
                    skip(parser);
                }
            }
            if (codecSads.size() == 0) {
                return null;
            }
            return new DeviceConfig(deviceType, codecSads);
        }

        // Processes sad attribute in the supportedFormat.
        @Nullable
        private static byte[] readSad(String sad) {
            if (sad == null || sad.length() == 0) {
                return null;
            }
            byte[] sadBytes = HexDump.hexStringToByteArray(sad);
            if (sadBytes.length != 3) {
                Log.w(TAG, "SAD byte array length is not 3. Length = " + sadBytes.length);
                return null;
            }
            return sadBytes;
        }

        private static int formatNameToNum(String codecAttriValue) {
            switch (codecAttriValue) {
                case "AUDIO_FORMAT_NONE":
                    return Constants.AUDIO_CODEC_NONE;
                case "AUDIO_FORMAT_LPCM":
                    return Constants.AUDIO_CODEC_LPCM;
                case "AUDIO_FORMAT_DD":
                    return Constants.AUDIO_CODEC_DD;
                case "AUDIO_FORMAT_MPEG1":
                    return Constants.AUDIO_CODEC_MPEG1;
                case "AUDIO_FORMAT_MP3":
                    return Constants.AUDIO_CODEC_MP3;
                case "AUDIO_FORMAT_MPEG2":
                    return Constants.AUDIO_CODEC_MPEG2;
                case "AUDIO_FORMAT_AAC":
                    return Constants.AUDIO_CODEC_AAC;
                case "AUDIO_FORMAT_DTS":
                    return Constants.AUDIO_CODEC_DTS;
                case "AUDIO_FORMAT_ATRAC":
                    return Constants.AUDIO_CODEC_ATRAC;
                case "AUDIO_FORMAT_ONEBITAUDIO":
                    return Constants.AUDIO_CODEC_ONEBITAUDIO;
                case "AUDIO_FORMAT_DDP":
                    return Constants.AUDIO_CODEC_DDP;
                case "AUDIO_FORMAT_DTSHD":
                    return Constants.AUDIO_CODEC_DTSHD;
                case "AUDIO_FORMAT_TRUEHD":
                    return Constants.AUDIO_CODEC_TRUEHD;
                case "AUDIO_FORMAT_DST":
                    return Constants.AUDIO_CODEC_DST;
                case "AUDIO_FORMAT_WMAPRO":
                    return Constants.AUDIO_CODEC_WMAPRO;
                case "AUDIO_FORMAT_MAX":
                    return Constants.AUDIO_CODEC_MAX;
                default:
                    return Constants.AUDIO_CODEC_NONE;
            }
        }
    }

    // Device configuration of its supported Codecs and their Short Audio Descriptors.
    public static class DeviceConfig {
        /** Name of the device. Should be {@link Constants.AudioDevice}. **/
        public final String name;
        /** List of a {@link CodecSad}. **/
        public final List<CodecSad> supportedCodecs;

        public DeviceConfig(String name, List<CodecSad> supportedCodecs) {
            this.name = name;
            this.supportedCodecs = supportedCodecs;
        }

        @Override
        public boolean equals(Object obj) {
            if (obj instanceof DeviceConfig) {
                DeviceConfig that = (DeviceConfig) obj;
                return that.name.equals(this.name)
                    && that.supportedCodecs.equals(this.supportedCodecs);
            }
            return false;
        }

        @Override
        public int hashCode() {
            return Objects.hash(
                name,
                supportedCodecs.hashCode());
        }
    }

    // Short Audio Descriptor of a specific Codec
    public static class CodecSad {
        /** Audio Codec. Should be {@link Constants.AudioCodec}. **/
        public final int audioCodec;
        /**
         * Three-byte Short Audio Descriptor. See HDMI Specification 1.4b CEC 13.15.3 and
         * ANSI-CTA-861-F-FINAL 7.5.2 Audio Data Block for more details.
         */
        public final byte[] sad;

        public CodecSad(int audioCodec, byte[] sad) {
            this.audioCodec = audioCodec;
            this.sad = sad;
        }

        public CodecSad(int audioCodec, String sad) {
            this.audioCodec = audioCodec;
            this.sad = HexDump.hexStringToByteArray(sad);
        }

        @Override
        public boolean equals(Object obj) {
            if (obj instanceof CodecSad) {
                CodecSad that = (CodecSad) obj;
                return that.audioCodec == this.audioCodec
                    && Arrays.equals(that.sad, this.sad);
            }
            return false;
        }

        @Override
        public int hashCode() {
            return Objects.hash(
                audioCodec,
                Arrays.hashCode(sad));
        }
    }
}