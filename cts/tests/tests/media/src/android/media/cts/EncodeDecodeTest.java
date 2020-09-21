/*
 * Copyright (C) 2013 The Android Open Source Project
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

package android.media.cts;

import android.graphics.ImageFormat;
import android.media.Image;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.opengl.GLES20;
import android.support.test.filters.SmallTest;
import android.platform.test.annotations.RequiresDevice;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.compatibility.common.util.MediaUtils;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;

import javax.microedition.khronos.opengles.GL10;

/**
 * Generates a series of video frames, encodes them, decodes them, and tests for significant
 * divergence from the original.
 * <p>
 * We copy the data from the encoder's output buffers to the decoder's input buffers, running
 * them in parallel.  The first buffer output for video/avc contains codec configuration data,
 * which we must carefully forward to the decoder.
 * <p>
 * An alternative approach would be to save the output of the decoder as an mpeg4 video
 * file, and read it back in from disk.  The data we're generating is just an elementary
 * stream, so we'd need to perform additional steps to make that happen.
 */
@SmallTest
@RequiresDevice
public class EncodeDecodeTest extends AndroidTestCase {
    private static final String TAG = "EncodeDecodeTest";
    private static final boolean VERBOSE = false;           // lots of logging
    private static final boolean DEBUG_SAVE_FILE = false;   // save copy of encoded movie
    private static final String DEBUG_FILE_NAME_BASE = "/sdcard/test.";

    // parameters for the encoder
                                                            // H.264 Advanced Video Coding
    private static final String MIME_TYPE_AVC = MediaFormat.MIMETYPE_VIDEO_AVC;
    private static final String MIME_TYPE_VP8 = MediaFormat.MIMETYPE_VIDEO_VP8;
    private static final int FRAME_RATE = 15;               // 15fps
    private static final int IFRAME_INTERVAL = 10;          // 10 seconds between I-frames

    // movie length, in frames
    private static final int NUM_FRAMES = 30;               // two seconds of video

    private static final int TEST_Y = 120;                  // YUV values for colored rect
    private static final int TEST_U = 160;
    private static final int TEST_V = 200;
    private static final int TEST_R0 = 0;                   // RGB equivalent of {0,0,0} (BT.601)
    private static final int TEST_G0 = 136;
    private static final int TEST_B0 = 0;
    private static final int TEST_R1 = 236;                 // RGB equivalent of {120,160,200} (BT.601)
    private static final int TEST_G1 = 50;
    private static final int TEST_B1 = 186;
    private static final int TEST_R0_BT709 = 0;             // RGB equivalent of {0,0,0} (BT.709)
    private static final int TEST_G0_BT709 = 77;
    private static final int TEST_B0_BT709 = 0;
    private static final int TEST_R1_BT709 = 250;           // RGB equivalent of {120,160,200} (BT.709)
    private static final int TEST_G1_BT709 = 76;
    private static final int TEST_B1_BT709 = 189;
    private static final boolean USE_NDK = true;

    // size of a frame, in pixels
    private int mWidth = -1;
    private int mHeight = -1;
    // bit rate, in bits per second
    private int mBitRate = -1;
    private String mMimeType = MIME_TYPE_AVC;

    // largest color component delta seen (i.e. actual vs. expected)
    private int mLargestColorDelta;

    // validate YUV->RGB decoded frames against BT.601 and/or BT.709
    private boolean mAllowBT601 = true;
    private boolean mAllowBT709 = false;

    /**
     * Tests streaming of AVC video through the encoder and decoder.  Data is encoded from
     * a series of byte[] buffers and decoded into ByteBuffers.  The output is checked for
     * validity.
     */
    public void testEncodeDecodeVideoFromBufferToBufferQCIF() throws Exception {
        setParameters(176, 144, 1000000, MIME_TYPE_AVC, true, false);
        encodeDecodeVideoFromBuffer(false);
    }
    public void testEncodeDecodeVideoFromBufferToBufferQVGA() throws Exception {
        setParameters(320, 240, 2000000, MIME_TYPE_AVC, true, false);
        encodeDecodeVideoFromBuffer(false);
    }
    public void testEncodeDecodeVideoFromBufferToBuffer720p() throws Exception {
        setParameters(1280, 720, 6000000, MIME_TYPE_AVC, true, false);
        encodeDecodeVideoFromBuffer(false);
    }

    /**
     * Tests streaming of VP8 video through the encoder and decoder.  Data is encoded from
     * a series of byte[] buffers and decoded into ByteBuffers.  The output is checked for
     * validity.
     */
    public void testVP8EncodeDecodeVideoFromBufferToBufferQCIF() throws Exception {
        setParameters(176, 144, 1000000, MIME_TYPE_VP8, true, false);
        encodeDecodeVideoFromBuffer(false);
    }
    public void testVP8EncodeDecodeVideoFromBufferToBufferQVGA() throws Exception {
        setParameters(320, 240, 2000000, MIME_TYPE_VP8, true, false);
        encodeDecodeVideoFromBuffer(false);
    }
    public void testVP8EncodeDecodeVideoFromBufferToBuffer720p() throws Exception {
        setParameters(1280, 720, 6000000, MIME_TYPE_VP8, true, false);
        encodeDecodeVideoFromBuffer(false);
    }

    /**
     * Tests streaming of AVC video through the encoder and decoder.  Data is encoded from
     * a series of byte[] buffers and decoded into Surfaces.  The output is checked for
     * validity.
     * <p>
     * Because of the way SurfaceTexture.OnFrameAvailableListener works, we need to run this
     * test on a thread that doesn't have a Looper configured.  If we don't, the test will
     * pass, but we won't actually test the output because we'll never receive the "frame
     * available" notifications".  The CTS test framework seems to be configuring a Looper on
     * the test thread, so we have to hand control off to a new thread for the duration of
     * the test.
     */
    public void testEncodeDecodeVideoFromBufferToSurfaceQCIF() throws Throwable {
        setParameters(176, 144, 1000000, MIME_TYPE_AVC, true, false);
        BufferToSurfaceWrapper.runTest(this);
    }
    public void testEncodeDecodeVideoFromBufferToSurfaceQVGA() throws Throwable {
        setParameters(320, 240, 2000000, MIME_TYPE_AVC, true, false);
        BufferToSurfaceWrapper.runTest(this);
    }
    public void testEncodeDecodeVideoFromBufferToSurface720p() throws Throwable {
        setParameters(1280, 720, 6000000, MIME_TYPE_AVC, true, true);
        BufferToSurfaceWrapper.runTest(this);
    }

    /**
     * Tests streaming of VP8 video through the encoder and decoder.  Data is encoded from
     * a series of byte[] buffers and decoded into Surfaces.  The output is checked for
     * validity.
     */
    public void testVP8EncodeDecodeVideoFromBufferToSurfaceQCIF() throws Throwable {
        setParameters(176, 144, 1000000, MIME_TYPE_VP8, true, false);
        BufferToSurfaceWrapper.runTest(this);
    }
    public void testVP8EncodeDecodeVideoFromBufferToSurfaceQVGA() throws Throwable {
        setParameters(320, 240, 2000000, MIME_TYPE_VP8, true, false);
        BufferToSurfaceWrapper.runTest(this);
    }
    public void testVP8EncodeDecodeVideoFromBufferToSurface720p() throws Throwable {
        setParameters(1280, 720, 6000000, MIME_TYPE_VP8, true, true);
        BufferToSurfaceWrapper.runTest(this);
    }

    /** Wraps testEncodeDecodeVideoFromBuffer(true) */
    private static class BufferToSurfaceWrapper implements Runnable {
        private Throwable mThrowable;
        private EncodeDecodeTest mTest;

        private BufferToSurfaceWrapper(EncodeDecodeTest test) {
            mTest = test;
        }

        @Override
        public void run() {
            try {
                mTest.encodeDecodeVideoFromBuffer(true);
            } catch (Throwable th) {
                mThrowable = th;
            }
        }

        /**
         * Entry point.
         */
        public static void runTest(EncodeDecodeTest obj) throws Throwable {
            BufferToSurfaceWrapper wrapper = new BufferToSurfaceWrapper(obj);
            Thread th = new Thread(wrapper, "codec test");
            th.start();
            th.join();
            if (wrapper.mThrowable != null) {
                throw wrapper.mThrowable;
            }
        }
    }

    /**
     * Tests streaming of AVC video through the encoder and decoder.  Data is provided through
     * a Surface and decoded onto a Surface.  The output is checked for validity.
     */
    public void testEncodeDecodeVideoFromSurfaceToSurfaceQCIF() throws Throwable {
        setParameters(176, 144, 1000000, MIME_TYPE_AVC, true, false);
        SurfaceToSurfaceWrapper.runTest(this, false, false);
    }
    public void testEncodeDecodeVideoFromSurfaceToSurfaceQVGA() throws Throwable {
        setParameters(320, 240, 2000000, MIME_TYPE_AVC, true, false);
        SurfaceToSurfaceWrapper.runTest(this, false, false);
    }
    public void testEncodeDecodeVideoFromSurfaceToSurface720p() throws Throwable {
        setParameters(1280, 720, 6000000, MIME_TYPE_AVC, true, false);
        SurfaceToSurfaceWrapper.runTest(this, false, false);
    }
    public void testEncodeDecodeVideoFromSurfaceToSurface720pNdk() throws Throwable {
        setParameters(1280, 720, 6000000, MIME_TYPE_AVC, true, false);
        SurfaceToSurfaceWrapper.runTest(this, false, USE_NDK);
    }

    /**
     * Tests streaming of AVC video through the encoder and decoder.  Data is provided through
     * a PersistentSurface and decoded onto a Surface.  The output is checked for validity.
     */
    public void testEncodeDecodeVideoFromPersistentSurfaceToSurfaceQCIF() throws Throwable {
        setParameters(176, 144, 1000000, MIME_TYPE_AVC, true, false);
        SurfaceToSurfaceWrapper.runTest(this, true, false);
    }
    public void testEncodeDecodeVideoFromPersistentSurfaceToSurfaceQVGA() throws Throwable {
        setParameters(320, 240, 2000000, MIME_TYPE_AVC, true, false);
        SurfaceToSurfaceWrapper.runTest(this, true, false);
    }
    public void testEncodeDecodeVideoFromPersistentSurfaceToSurface720p() throws Throwable {
        setParameters(1280, 720, 6000000, MIME_TYPE_AVC, true, false);
        SurfaceToSurfaceWrapper.runTest(this, true, false);
    }
    public void testEncodeDecodeVideoFromPersistentSurfaceToSurface720pNdk() throws Throwable {
        setParameters(1280, 720, 6000000, MIME_TYPE_AVC, true, false);
        SurfaceToSurfaceWrapper.runTest(this, true, USE_NDK);
    }

    /**
     * Tests streaming of VP8 video through the encoder and decoder.  Data is provided through
     * a Surface and decoded onto a Surface.  The output is checked for validity.
     */
    public void testVP8EncodeDecodeVideoFromSurfaceToSurfaceQCIF() throws Throwable {
        setParameters(176, 144, 1000000, MIME_TYPE_VP8, true, false);
        SurfaceToSurfaceWrapper.runTest(this, false, false);
    }
    public void testVP8EncodeDecodeVideoFromSurfaceToSurfaceQVGA() throws Throwable {
        setParameters(320, 240, 2000000, MIME_TYPE_VP8, true, false);
        SurfaceToSurfaceWrapper.runTest(this, false, false);
    }
    public void testVP8EncodeDecodeVideoFromSurfaceToSurface720p() throws Throwable {
        setParameters(1280, 720, 6000000, MIME_TYPE_VP8, true, false);
        SurfaceToSurfaceWrapper.runTest(this, false, false);
    }
    public void testVP8EncodeDecodeVideoFromSurfaceToSurface720pNdk() throws Throwable {
        setParameters(1280, 720, 6000000, MIME_TYPE_VP8, true, false);
        SurfaceToSurfaceWrapper.runTest(this, false, USE_NDK);
    }

    /**
     * Tests streaming of VP8 video through the encoder and decoder.  Data is provided through
     * a PersistentSurface and decoded onto a Surface.  The output is checked for validity.
     */
    public void testVP8EncodeDecodeVideoFromPersistentSurfaceToSurfaceQCIF() throws Throwable {
        setParameters(176, 144, 1000000, MIME_TYPE_VP8, true, false);
        SurfaceToSurfaceWrapper.runTest(this, true, false);
    }
    public void testVP8EncodeDecodeVideoFromPersistentSurfaceToSurfaceQVGA() throws Throwable {
        setParameters(320, 240, 2000000, MIME_TYPE_VP8, true, false);
        SurfaceToSurfaceWrapper.runTest(this, true, false);
    }
    public void testVP8EncodeDecodeVideoFromPersistentSurfaceToSurface720p() throws Throwable {
        setParameters(1280, 720, 6000000, MIME_TYPE_VP8, true, false);
        SurfaceToSurfaceWrapper.runTest(this, true, false);
    }
    public void testVP8EncodeDecodeVideoFromPersistentSurfaceToSurface720pNdk() throws Throwable {
        setParameters(1280, 720, 6000000, MIME_TYPE_VP8, true, false);
        SurfaceToSurfaceWrapper.runTest(this, true, USE_NDK);
    }

    /** Wraps testEncodeDecodeVideoFromSurfaceToSurface() */
    private static class SurfaceToSurfaceWrapper implements Runnable {
        private Throwable mThrowable;
        private EncodeDecodeTest mTest;
        private boolean mUsePersistentInput;
        private boolean mUseNdk;

        private SurfaceToSurfaceWrapper(EncodeDecodeTest test, boolean persistent, boolean useNdk) {
            mTest = test;
            mUsePersistentInput = persistent;
            mUseNdk = useNdk;
        }

        @Override
        public void run() {
            if (mTest.shouldSkip()) {
                return;
            }

            InputSurfaceInterface inputSurface = null;
            try {
                if (!mUsePersistentInput) {
                    mTest.encodeDecodeVideoFromSurfaceToSurface(null, mUseNdk);
                } else {
                    Log.d(TAG, "creating persistent surface");
                    if (mUseNdk) {
                        inputSurface = NdkMediaCodec.createPersistentInputSurface();
                    } else {
                        inputSurface = new InputSurface(MediaCodec.createPersistentInputSurface());
                    }

                    for (int i = 0; i < 3; i++) {
                        Log.d(TAG, "test persistent surface - round " + i);
                        mTest.encodeDecodeVideoFromSurfaceToSurface(inputSurface, mUseNdk);
                    }
                }
            } catch (Throwable th) {
                mThrowable = th;
            } finally {
                if (inputSurface != null) {
                    inputSurface.release();
                }
            }
        }

        /**
         * Entry point.
         */
        public static void runTest(EncodeDecodeTest obj, boolean persisent, boolean useNdk)
                throws Throwable {
            SurfaceToSurfaceWrapper wrapper =
                    new SurfaceToSurfaceWrapper(obj, persisent, useNdk);
            Thread th = new Thread(wrapper, "codec test");
            th.start();
            th.join();
            if (wrapper.mThrowable != null) {
                throw wrapper.mThrowable;
            }
        }
    }

    /**
     * Sets the desired frame size and bit rate.
     */
    protected void setParameters(int width, int height, int bitRate, String mimeType,
	        boolean allowBT601, boolean allowBT709) {
        if ((width % 16) != 0 || (height % 16) != 0) {
            Log.w(TAG, "WARNING: width or height not multiple of 16");
        }
        mWidth = width;
        mHeight = height;
        mBitRate = bitRate;
        mMimeType = mimeType;
        mAllowBT601 = allowBT601;
        mAllowBT709 = allowBT709;
    }

    private boolean shouldSkip() {
        if (!MediaUtils.hasEncoder(mMimeType)) {
            return true;
        }

        MediaFormat format = MediaFormat.createVideoFormat(mMimeType, mWidth, mHeight);
        format.setInteger(MediaFormat.KEY_BIT_RATE, mBitRate);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, IFRAME_INTERVAL);
        if (!MediaUtils.checkEncoderForFormat(format)) {
            return true;
        }

        return false;
    }

    /**
     * Tests encoding and subsequently decoding video from frames generated into a buffer.
     * <p>
     * We encode several frames of a video test pattern using MediaCodec, then decode the
     * output with MediaCodec and do some simple checks.
     * <p>
     * See http://b.android.com/37769 for a discussion of input format pitfalls.
     */
    private void encodeDecodeVideoFromBuffer(boolean toSurface) throws Exception {
        if (shouldSkip()) {
            return;
        }

        MediaCodec encoder = null;
        MediaCodec decoder = null;

        mLargestColorDelta = -1;

        try {
            // We avoid the device-specific limitations on width and height by using values that
            // are multiples of 16, which all tested devices seem to be able to handle.
            MediaFormat format = MediaFormat.createVideoFormat(mMimeType, mWidth, mHeight);
            MediaCodecList mcl = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
            String codec = mcl.findEncoderForFormat(format);
            if (codec == null) {
                // Don't fail CTS if they don't have an AVC codec (not here, anyway).
                Log.e(TAG, "Unable to find an appropriate codec for " + format);
                return;
            }
            if (VERBOSE) Log.d(TAG, "found codec: " + codec);

            String codec_decoder = mcl.findDecoderForFormat(format);
            if (codec_decoder == null) {
                Log.e(TAG, "Unable to find an appropriate codec for " + format);
                return;
            }

            // Create a MediaCodec for the desired codec, then configure it as an encoder with
            // our desired properties.
            encoder = MediaCodec.createByCodecName(codec);

            int colorFormat = selectColorFormat(encoder.getCodecInfo(), mMimeType);
            if (VERBOSE) Log.d(TAG, "found colorFormat: " + colorFormat);

            // Set some properties.  Failing to specify some of these can cause the MediaCodec
            // configure() call to throw an unhelpful exception.
            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, colorFormat);
            format.setInteger(MediaFormat.KEY_BIT_RATE, mBitRate);
            format.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);
            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, IFRAME_INTERVAL);
            if (VERBOSE) Log.d(TAG, "format: " + format);

            encoder.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            encoder.start();

            // Create a MediaCodec for the decoder, just based on the MIME type.  The various
            // format details will be passed through the csd-0 meta-data later on.
            decoder = MediaCodec.createByCodecName(codec_decoder);
            if (VERBOSE) Log.d(TAG, "got decoder: " + decoder.getName());

            doEncodeDecodeVideoFromBuffer(encoder, colorFormat, decoder, toSurface);
        } finally {
            if (VERBOSE) Log.d(TAG, "releasing codecs");
            if (encoder != null) {
                encoder.stop();
                encoder.release();
            }
            if (decoder != null) {
                decoder.stop();
                decoder.release();
            }

            Log.i(TAG, "Largest color delta: " + mLargestColorDelta);
        }
    }

    /**
     * Tests encoding and subsequently decoding video from frames generated into a buffer.
     * <p>
     * We encode several frames of a video test pattern using MediaCodec, then decode the
     * output with MediaCodec and do some simple checks.
     */
    private void encodeDecodeVideoFromSurfaceToSurface(InputSurfaceInterface inSurf, boolean useNdk) throws Exception {
        MediaCodecWrapper encoder = null;
        MediaCodec decoder = null;
        InputSurfaceInterface inputSurface = inSurf;
        OutputSurface outputSurface = null;

        mLargestColorDelta = -1;

        try {
            // We avoid the device-specific limitations on width and height by using values that
            // are multiples of 16, which all tested devices seem to be able to handle.
            MediaFormat format = MediaFormat.createVideoFormat(mMimeType, mWidth, mHeight);
            MediaCodecList mcl = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
            String codec = mcl.findEncoderForFormat(format);
            if (codec == null) {
                // Don't fail CTS if they don't have an AVC codec (not here, anyway).
                Log.e(TAG, "Unable to find an appropriate codec for " + format);
                return;
            }
            if (VERBOSE) Log.d(TAG, "found codec: " + codec);

            int colorFormat = MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface;

            // Set some properties.  Failing to specify some of these can cause the MediaCodec
            // configure() call to throw an unhelpful exception.
            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, colorFormat);
            format.setInteger(MediaFormat.KEY_BIT_RATE, mBitRate);
            format.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);
            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, IFRAME_INTERVAL);
            if (VERBOSE) Log.d(TAG, "format: " + format);

            // Create the output surface.
            outputSurface = new OutputSurface(mWidth, mHeight);

            // Create a MediaCodec for the decoder, just based on the MIME type.  The various
            // format details will be passed through the csd-0 meta-data later on.
            String codec_decoder = mcl.findDecoderForFormat(format);
            if (codec_decoder == null) {
                Log.e(TAG, "Unable to find an appropriate codec for " + format);
                return;
            }
            decoder = MediaCodec.createByCodecName(codec_decoder);
            if (VERBOSE) Log.d(TAG, "got decoder: " + decoder.getName());
            decoder.configure(format, outputSurface.getSurface(), null, 0);
            decoder.start();

            // Create a MediaCodec for the desired codec, then configure it as an encoder with
            // our desired properties.  Request a Surface to use for input.
            if (useNdk) {
                encoder = new NdkMediaCodec(codec);
            }else {
                encoder = new SdkMediaCodec(MediaCodec.createByCodecName(codec));
            }
            encoder.configure(format, MediaCodec.CONFIGURE_FLAG_ENCODE);
            if (inSurf != null) {
                Log.d(TAG, "using persistent surface");
                encoder.setInputSurface(inputSurface);
                inputSurface.updateSize(mWidth, mHeight);
            } else {
                inputSurface = encoder.createInputSurface();
            }
            encoder.start();

            doEncodeDecodeVideoFromSurfaceToSurface(encoder, inputSurface, decoder, outputSurface);
        } finally {
            if (VERBOSE) Log.d(TAG, "releasing codecs");
            if (inSurf == null && inputSurface != null) {
                inputSurface.release();
            }
            if (outputSurface != null) {
                outputSurface.release();
            }
            if (encoder != null) {
                encoder.stop();
                encoder.release();
            }
            if (decoder != null) {
                decoder.stop();
                decoder.release();
            }

            Log.i(TAG, "Largest color delta: " + mLargestColorDelta);
        }
    }

    /**
     * Returns a color format that is supported by the codec and by this test code.  If no
     * match is found, this throws a test failure -- the set of formats known to the test
     * should be expanded for new platforms.
     */
    private static int selectColorFormat(MediaCodecInfo codecInfo, String mimeType) {
        MediaCodecInfo.CodecCapabilities capabilities = codecInfo.getCapabilitiesForType(mimeType);
        for (int i = 0; i < capabilities.colorFormats.length; i++) {
            int colorFormat = capabilities.colorFormats[i];
            if (isRecognizedFormat(colorFormat)) {
                return colorFormat;
            }
        }
        fail("couldn't find a good color format for " + codecInfo.getName() + " / " + mimeType);
        return 0;   // not reached
    }

    /**
     * Returns true if this is a color format that this test code understands (i.e. we know how
     * to read and generate frames in this format).
     */
    private static boolean isRecognizedFormat(int colorFormat) {
        switch (colorFormat) {
            // these are the formats we know how to handle for this test
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedSemiPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_TI_FormatYUV420PackedSemiPlanar:
                return true;
            default:
                return false;
        }
    }

    /**
     * Returns true if the specified color format is semi-planar YUV.  Throws an exception
     * if the color format is not recognized (e.g. not YUV).
     */
    private static boolean isSemiPlanarYUV(int colorFormat) {
        switch (colorFormat) {
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedPlanar:
                return false;
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedSemiPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_TI_FormatYUV420PackedSemiPlanar:
                return true;
            default:
                throw new RuntimeException("unknown format " + colorFormat);
        }
    }

    /**
     * Does the actual work for encoding frames from buffers of byte[].
     */
    private void doEncodeDecodeVideoFromBuffer(MediaCodec encoder, int encoderColorFormat,
            MediaCodec decoder, boolean toSurface) {
        final int TIMEOUT_USEC = 10000;
        ByteBuffer[] encoderInputBuffers = encoder.getInputBuffers();
        ByteBuffer[] encoderOutputBuffers = encoder.getOutputBuffers();
        ByteBuffer[] decoderInputBuffers = null;
        ByteBuffer[] decoderOutputBuffers = null;
        MediaCodec.BufferInfo decoderInfo = new MediaCodec.BufferInfo();
        MediaCodec.BufferInfo encoderInfo = new MediaCodec.BufferInfo();
        MediaFormat decoderOutputFormat = null;
        int generateIndex = 0;
        int checkIndex = 0;
        int badFrames = 0;
        boolean decoderConfigured = false;
        OutputSurface outputSurface = null;

        // The size of a frame of video data, in the formats we handle, is stride*sliceHeight
        // for Y, and (stride/2)*(sliceHeight/2) for each of the Cb and Cr channels.  Application
        // of algebra and assuming that stride==width and sliceHeight==height yields:
        byte[] frameData = new byte[mWidth * mHeight * 3 / 2];

        // Just out of curiosity.
        long rawSize = 0;
        long encodedSize = 0;

        // Save a copy to disk.  Useful for debugging the test.  Note this is a raw elementary
        // stream, not a .mp4 file, so not all players will know what to do with it.
        FileOutputStream outputStream = null;
        if (DEBUG_SAVE_FILE) {
            String fileName = DEBUG_FILE_NAME_BASE + mWidth + "x" + mHeight + ".mp4";
            try {
                outputStream = new FileOutputStream(fileName);
                Log.d(TAG, "encoded output will be saved as " + fileName);
            } catch (IOException ioe) {
                Log.w(TAG, "Unable to create debug output file " + fileName);
                throw new RuntimeException(ioe);
            }
        }

        if (toSurface) {
            outputSurface = new OutputSurface(mWidth, mHeight);
        }

        // Loop until the output side is done.
        boolean inputDone = false;
        boolean encoderDone = false;
        boolean outputDone = false;
        int encoderStatus = -1;
        while (!outputDone) {
            if (VERBOSE) Log.d(TAG, "loop");


            // If we're not done submitting frames, generate a new one and submit it.  By
            // doing this on every loop we're working to ensure that the encoder always has
            // work to do.
            //
            // We don't really want a timeout here, but sometimes there's a delay opening
            // the encoder device, so a short timeout can keep us from spinning hard.
            if (!inputDone) {
                int inputBufIndex = encoder.dequeueInputBuffer(TIMEOUT_USEC);
                if (VERBOSE) Log.d(TAG, "inputBufIndex=" + inputBufIndex);
                if (inputBufIndex >= 0) {
                    long ptsUsec = computePresentationTime(generateIndex);
                    if (generateIndex == NUM_FRAMES) {
                        // Send an empty frame with the end-of-stream flag set.  If we set EOS
                        // on a frame with data, that frame data will be ignored, and the
                        // output will be short one frame.
                        encoder.queueInputBuffer(inputBufIndex, 0, 0, ptsUsec,
                                MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        inputDone = true;
                        if (VERBOSE) Log.d(TAG, "sent input EOS (with zero-length frame)");
                    } else {
                        generateFrame(generateIndex, encoderColorFormat, frameData);

                        ByteBuffer inputBuf = encoder.getInputBuffer(inputBufIndex);
                        // the buffer should be sized to hold one full frame
                        assertTrue(inputBuf.capacity() >= frameData.length);
                        inputBuf.clear();
                        inputBuf.put(frameData);

                        encoder.queueInputBuffer(inputBufIndex, 0, frameData.length, ptsUsec, 0);
                        if (VERBOSE) Log.d(TAG, "submitted frame " + generateIndex + " to enc");
                    }
                    generateIndex++;
                } else {
                    // either all in use, or we timed out during initial setup
                    if (VERBOSE) Log.d(TAG, "input buffer not available");
                }
            }

            // Check for output from the encoder.  If there's no output yet, we either need to
            // provide more input, or we need to wait for the encoder to work its magic.  We
            // can't actually tell which is the case, so if we can't get an output buffer right
            // away we loop around and see if it wants more input.
            //
            // Once we get EOS from the encoder, we don't need to do this anymore.
            if (!encoderDone) {
                MediaCodec.BufferInfo info = encoderInfo;
                if (encoderStatus < 0) {
                    encoderStatus = encoder.dequeueOutputBuffer(info, TIMEOUT_USEC);
                }
                if (encoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
                    // no output available yet
                    if (VERBOSE) Log.d(TAG, "no output from encoder available");
                } else if (encoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                    // not expected for an encoder
                    encoderOutputBuffers = encoder.getOutputBuffers();
                    if (VERBOSE) Log.d(TAG, "encoder output buffers changed");
                } else if (encoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    // expected on API 18+
                    MediaFormat newFormat = encoder.getOutputFormat();
                    if (VERBOSE) Log.d(TAG, "encoder output format changed: " + newFormat);
                } else if (encoderStatus < 0) {
                    fail("unexpected result from encoder.dequeueOutputBuffer: " + encoderStatus);
                } else { // encoderStatus >= 0
                    ByteBuffer encodedData = encoder.getOutputBuffer(encoderStatus);
                    if (encodedData == null) {
                        fail("encoderOutputBuffer " + encoderStatus + " was null");
                    }

                    // It's usually necessary to adjust the ByteBuffer values to match BufferInfo.
                    encodedData.position(info.offset);
                    encodedData.limit(info.offset + info.size);

                    boolean releaseBuffer = false;
                    if (!decoderConfigured) {
                        // Codec config info.  Only expected on first packet.  One way to
                        // handle this is to manually stuff the data into the MediaFormat
                        // and pass that to configure().  We do that here to exercise the API.
                        // For codecs that don't have codec config data (such as VP8),
                        // initialize the decoder before trying to decode the first packet.
                        assertTrue((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0 ||
                                   mMimeType.equals(MIME_TYPE_VP8));
                        MediaFormat format =
                                MediaFormat.createVideoFormat(mMimeType, mWidth, mHeight);
                        if ((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0)
                            format.setByteBuffer("csd-0", encodedData);
                        decoder.configure(format, toSurface ? outputSurface.getSurface() : null,
                                null, 0);
                        decoder.start();
                        decoderInputBuffers = decoder.getInputBuffers();
                        decoderOutputBuffers = decoder.getOutputBuffers();
                        decoderConfigured = true;
                        if (VERBOSE) Log.d(TAG, "decoder configured (" + info.size + " bytes)");
                    }
                    if ((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0) {
                        // Get a decoder input buffer
                        assertTrue(decoderConfigured);
                        int inputBufIndex = decoder.dequeueInputBuffer(TIMEOUT_USEC);
                        if (inputBufIndex >= 0) {
                            ByteBuffer inputBuf = decoderInputBuffers[inputBufIndex];
                            inputBuf.clear();
                            inputBuf.put(encodedData);
                            decoder.queueInputBuffer(inputBufIndex, 0, info.size,
                                    info.presentationTimeUs, info.flags);

                            encoderDone = (info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0;
                            if (VERBOSE) Log.d(TAG, "passed " + info.size + " bytes to decoder"
                                    + (encoderDone ? " (EOS)" : ""));
                            releaseBuffer = true;
                        }
                    } else {
                        releaseBuffer = true;
                    }
                    if (releaseBuffer) {
                        encodedSize += info.size;
                        if (outputStream != null) {
                            byte[] data = new byte[info.size];
                            encodedData.position(info.offset);
                            encodedData.get(data);
                            try {
                                outputStream.write(data);
                            } catch (IOException ioe) {
                                Log.w(TAG, "failed writing debug data to file");
                                throw new RuntimeException(ioe);
                            }
                        }
                        encoder.releaseOutputBuffer(encoderStatus, false);
                        encoderStatus = -1;
                    }

                }
            }

            // Check for output from the decoder.  We want to do this on every loop to avoid
            // the possibility of stalling the pipeline.  We use a short timeout to avoid
            // burning CPU if the decoder is hard at work but the next frame isn't quite ready.
            //
            // If we're decoding to a Surface, we'll get notified here as usual but the
            // ByteBuffer references will be null.  The data is sent to Surface instead.
            if (decoderConfigured) {
                MediaCodec.BufferInfo info = decoderInfo;
                int decoderStatus = decoder.dequeueOutputBuffer(info, TIMEOUT_USEC);
                if (decoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
                    // no output available yet
                    if (VERBOSE) Log.d(TAG, "no output from decoder available");
                } else if (decoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                    // The storage associated with the direct ByteBuffer may already be unmapped,
                    // so attempting to access data through the old output buffer array could
                    // lead to a native crash.
                    if (VERBOSE) Log.d(TAG, "decoder output buffers changed");
                    decoderOutputBuffers = decoder.getOutputBuffers();
                } else if (decoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    // this happens before the first frame is returned
                    decoderOutputFormat = decoder.getOutputFormat();
                    if (VERBOSE) Log.d(TAG, "decoder output format changed: " +
                            decoderOutputFormat);
                } else if (decoderStatus < 0) {
                    fail("unexpected result from deocder.dequeueOutputBuffer: " + decoderStatus);
                } else {  // decoderStatus >= 0
                    if (!toSurface) {
                        ByteBuffer outputFrame = decoderOutputBuffers[decoderStatus];
                        Image outputImage = (checkIndex % 2 == 0) ? null : decoder.getOutputImage(decoderStatus);

                        outputFrame.position(info.offset);
                        outputFrame.limit(info.offset + info.size);

                        rawSize += info.size;
                        if (info.size == 0) {
                            if (VERBOSE) Log.d(TAG, "got empty frame");
                        } else {
                            if (VERBOSE) Log.d(TAG, "decoded, checking frame " + checkIndex);
                            assertEquals("Wrong time stamp", computePresentationTime(checkIndex),
                                    info.presentationTimeUs);
                            if (!checkFrame(checkIndex++, decoderOutputFormat, outputFrame, outputImage)) {
                                badFrames++;
                            }
                        }
                        if (outputImage != null) {
                            outputImage.close();
                        }

                        if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                            if (VERBOSE) Log.d(TAG, "output EOS");
                            outputDone = true;
                        }
                        decoder.releaseOutputBuffer(decoderStatus, false /*render*/);
                    } else {
                        if (VERBOSE) Log.d(TAG, "surface decoder given buffer " + decoderStatus +
                                " (size=" + info.size + ")");
                        rawSize += info.size;
                        if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                            if (VERBOSE) Log.d(TAG, "output EOS");
                            outputDone = true;
                        }

                        boolean doRender = (info.size != 0);

                        // As soon as we call releaseOutputBuffer, the buffer will be forwarded
                        // to SurfaceTexture to convert to a texture.  The API doesn't guarantee
                        // that the texture will be available before the call returns, so we
                        // need to wait for the onFrameAvailable callback to fire.
                        decoder.releaseOutputBuffer(decoderStatus, doRender);
                        if (doRender) {
                            if (VERBOSE) Log.d(TAG, "awaiting frame " + checkIndex);
                            assertEquals("Wrong time stamp", computePresentationTime(checkIndex),
                                    info.presentationTimeUs);
                            outputSurface.awaitNewImage();
                            outputSurface.drawImage();
                            if (!checkSurfaceFrame(checkIndex++)) {
                                badFrames++;
                            }
                        }
                    }
                }
            }
        }

        if (VERBOSE) Log.d(TAG, "decoded " + checkIndex + " frames at "
                + mWidth + "x" + mHeight + ": raw=" + rawSize + ", enc=" + encodedSize);
        if (outputStream != null) {
            try {
                outputStream.close();
            } catch (IOException ioe) {
                Log.w(TAG, "failed closing debug file");
                throw new RuntimeException(ioe);
            }
        }

        if (outputSurface != null) {
            outputSurface.release();
        }

        if (checkIndex != NUM_FRAMES) {
            fail("expected " + NUM_FRAMES + " frames, only decoded " + checkIndex);
        }
        if (badFrames != 0) {
            fail("Found " + badFrames + " bad frames");
        }
    }

    /**
     * Does the actual work for encoding and decoding from Surface to Surface.
     */
    private void doEncodeDecodeVideoFromSurfaceToSurface(MediaCodecWrapper encoder,
            InputSurfaceInterface inputSurface, MediaCodec decoder,
            OutputSurface outputSurface) {
        final int TIMEOUT_USEC = 10000;
        ByteBuffer[] encoderOutputBuffers = encoder.getOutputBuffers();
        ByteBuffer[] decoderInputBuffers = decoder.getInputBuffers();
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        int generateIndex = 0;
        int checkIndex = 0;
        int badFrames = 0;

        // Save a copy to disk.  Useful for debugging the test.  Note this is a raw elementary
        // stream, not a .mp4 file, so not all players will know what to do with it.
        FileOutputStream outputStream = null;
        if (DEBUG_SAVE_FILE) {
            String fileName = DEBUG_FILE_NAME_BASE + mWidth + "x" + mHeight + ".mp4";
            try {
                outputStream = new FileOutputStream(fileName);
                Log.d(TAG, "encoded output will be saved as " + fileName);
            } catch (IOException ioe) {
                Log.w(TAG, "Unable to create debug output file " + fileName);
                throw new RuntimeException(ioe);
            }
        }

        // Loop until the output side is done.
        boolean inputDone = false;
        boolean encoderDone = false;
        boolean outputDone = false;
        while (!outputDone) {
            if (VERBOSE) Log.d(TAG, "loop");

            // If we're not done submitting frames, generate a new one and submit it.  The
            // eglSwapBuffers call will block if the input is full.
            if (!inputDone) {
                if (generateIndex == NUM_FRAMES) {
                    // Send an empty frame with the end-of-stream flag set.
                    if (VERBOSE) Log.d(TAG, "signaling input EOS");
                    encoder.signalEndOfInputStream();
                    inputDone = true;
                } else {
                    inputSurface.makeCurrent();
                    generateSurfaceFrame(generateIndex);
                    inputSurface.setPresentationTime(computePresentationTime(generateIndex) * 1000);
                    if (VERBOSE) Log.d(TAG, "inputSurface swapBuffers");
                    inputSurface.swapBuffers();
                }
                generateIndex++;
            }

            // Assume output is available.  Loop until both assumptions are false.
            boolean decoderOutputAvailable = true;
            boolean encoderOutputAvailable = !encoderDone;
            while (decoderOutputAvailable || encoderOutputAvailable) {
                // Start by draining any pending output from the decoder.  It's important to
                // do this before we try to stuff any more data in.
                int decoderStatus = decoder.dequeueOutputBuffer(info, TIMEOUT_USEC);
                if (decoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
                    // no output available yet
                    if (VERBOSE) Log.d(TAG, "no output from decoder available");
                    decoderOutputAvailable = false;
                } else if (decoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                    if (VERBOSE) Log.d(TAG, "decoder output buffers changed (but we don't care)");
                } else if (decoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    // this happens before the first frame is returned
                    MediaFormat decoderOutputFormat = decoder.getOutputFormat();
                    if (VERBOSE) Log.d(TAG, "decoder output format changed: " +
                            decoderOutputFormat);
                } else if (decoderStatus < 0) {
                    fail("unexpected result from deocder.dequeueOutputBuffer: " + decoderStatus);
                } else {  // decoderStatus >= 0
                    if (VERBOSE) Log.d(TAG, "surface decoder given buffer " + decoderStatus +
                            " (size=" + info.size + ")");
                    if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                        if (VERBOSE) Log.d(TAG, "output EOS");
                        outputDone = true;
                    }

                    // The ByteBuffers are null references, but we still get a nonzero size for
                    // the decoded data.
                    boolean doRender = (info.size != 0);

                    // As soon as we call releaseOutputBuffer, the buffer will be forwarded
                    // to SurfaceTexture to convert to a texture.  The API doesn't guarantee
                    // that the texture will be available before the call returns, so we
                    // need to wait for the onFrameAvailable callback to fire.  If we don't
                    // wait, we risk dropping frames.
                    outputSurface.makeCurrent();
                    decoder.releaseOutputBuffer(decoderStatus, doRender);
                    if (doRender) {
                        assertEquals("Wrong time stamp", computePresentationTime(checkIndex),
                                info.presentationTimeUs);
                        if (VERBOSE) Log.d(TAG, "awaiting frame " + checkIndex);
                        outputSurface.awaitNewImage();
                        outputSurface.drawImage();
                        if (!checkSurfaceFrame(checkIndex++)) {
                            badFrames++;
                        }
                    }
                }
                if (decoderStatus != MediaCodec.INFO_TRY_AGAIN_LATER) {
                    // Continue attempts to drain output.
                    continue;
                }

                // Decoder is drained, check to see if we've got a new buffer of output from
                // the encoder.
                if (!encoderDone) {
                    int encoderStatus = encoder.dequeueOutputBuffer(info, TIMEOUT_USEC);
                    if (encoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
                        // no output available yet
                        if (VERBOSE) Log.d(TAG, "no output from encoder available");
                        encoderOutputAvailable = false;
                    } else if (encoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                        // not expected for an encoder
                        encoderOutputBuffers = encoder.getOutputBuffers();
                        if (VERBOSE) Log.d(TAG, "encoder output buffers changed");
                    } else if (encoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                        // expected on API 18+
                        String newFormat = encoder.getOutputFormatString();
                        if (VERBOSE) Log.d(TAG, "encoder output format changed: " + newFormat);
                    } else if (encoderStatus < 0) {
                        fail("unexpected result from encoder.dequeueOutputBuffer: " + encoderStatus);
                    } else { // encoderStatus >= 0
                        ByteBuffer encodedData = encoder.getOutputBuffer(encoderStatus);
                        if (encodedData == null) {
                            fail("encoderOutputBuffer " + encoderStatus + " was null");
                        }

                        // It's usually necessary to adjust the ByteBuffer values to match BufferInfo.
                        encodedData.position(info.offset);
                        encodedData.limit(info.offset + info.size);

                        if (outputStream != null) {
                            byte[] data = new byte[info.size];
                            encodedData.get(data);
                            encodedData.position(info.offset);
                            try {
                                outputStream.write(data);
                            } catch (IOException ioe) {
                                Log.w(TAG, "failed writing debug data to file");
                                throw new RuntimeException(ioe);
                            }
                        }

                        // Get a decoder input buffer, blocking until it's available.  We just
                        // drained the decoder output, so we expect there to be a free input
                        // buffer now or in the near future (i.e. this should never deadlock
                        // if the codec is meeting requirements).
                        //
                        // The first buffer of data we get will have the BUFFER_FLAG_CODEC_CONFIG
                        // flag set; the decoder will see this and finish configuring itself.
                        int inputBufIndex = decoder.dequeueInputBuffer(-1);
                        ByteBuffer inputBuf = decoderInputBuffers[inputBufIndex];
                        inputBuf.clear();
                        inputBuf.put(encodedData);
                        decoder.queueInputBuffer(inputBufIndex, 0, info.size,
                                info.presentationTimeUs, info.flags);

                        // If everything from the encoder has been passed to the decoder, we
                        // can stop polling the encoder output.  (This just an optimization.)
                        if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                            encoderDone = true;
                            encoderOutputAvailable = false;
                        }
                        if (VERBOSE) Log.d(TAG, "passed " + info.size + " bytes to decoder"
                                + (encoderDone ? " (EOS)" : ""));

                        encoder.releaseOutputBuffer(encoderStatus, false);
                    }
                }
            }
        }

        if (outputStream != null) {
            try {
                outputStream.close();
            } catch (IOException ioe) {
                Log.w(TAG, "failed closing debug file");
                throw new RuntimeException(ioe);
            }
        }

        if (checkIndex != NUM_FRAMES) {
            fail("expected " + NUM_FRAMES + " frames, only decoded " + checkIndex);
        }
        if (badFrames != 0) {
            fail("Found " + badFrames + " bad frames");
        }
    }


    /**
     * Generates data for frame N into the supplied buffer.  We have an 8-frame animation
     * sequence that wraps around.  It looks like this:
     * <pre>
     *   0 1 2 3
     *   7 6 5 4
     * </pre>
     * We draw one of the eight rectangles and leave the rest set to the zero-fill color.
     */
    private void generateFrame(int frameIndex, int colorFormat, byte[] frameData) {
        final int HALF_WIDTH = mWidth / 2;
        boolean semiPlanar = isSemiPlanarYUV(colorFormat);

        // Set to zero.  In YUV this is a dull green.
        Arrays.fill(frameData, (byte) 0);

        int startX, startY;

        frameIndex %= 8;
        //frameIndex = (frameIndex / 8) % 8;    // use this instead for debug -- easier to see
        if (frameIndex < 4) {
            startX = frameIndex * (mWidth / 4);
            startY = 0;
        } else {
            startX = (7 - frameIndex) * (mWidth / 4);
            startY = mHeight / 2;
        }

        for (int y = startY + (mHeight/2) - 1; y >= startY; --y) {
            for (int x = startX + (mWidth/4) - 1; x >= startX; --x) {
                if (semiPlanar) {
                    // full-size Y, followed by UV pairs at half resolution
                    // e.g. Nexus 4 OMX.qcom.video.encoder.avc COLOR_FormatYUV420SemiPlanar
                    // e.g. Galaxy Nexus OMX.TI.DUCATI1.VIDEO.H264E
                    //        OMX_TI_COLOR_FormatYUV420PackedSemiPlanar
                    frameData[y * mWidth + x] = (byte) TEST_Y;
                    if ((x & 0x01) == 0 && (y & 0x01) == 0) {
                        frameData[mWidth*mHeight + y * HALF_WIDTH + x] = (byte) TEST_U;
                        frameData[mWidth*mHeight + y * HALF_WIDTH + x + 1] = (byte) TEST_V;
                    }
                } else {
                    // full-size Y, followed by quarter-size U and quarter-size V
                    // e.g. Nexus 10 OMX.Exynos.AVC.Encoder COLOR_FormatYUV420Planar
                    // e.g. Nexus 7 OMX.Nvidia.h264.encoder COLOR_FormatYUV420Planar
                    frameData[y * mWidth + x] = (byte) TEST_Y;
                    if ((x & 0x01) == 0 && (y & 0x01) == 0) {
                        frameData[mWidth*mHeight + (y/2) * HALF_WIDTH + (x/2)] = (byte) TEST_U;
                        frameData[mWidth*mHeight + HALF_WIDTH * (mHeight / 2) +
                                  (y/2) * HALF_WIDTH + (x/2)] = (byte) TEST_V;
                    }
                }
            }
        }
    }

    /**
     * Performs a simple check to see if the frame is more or less right.
     * <p>
     * See {@link #generateFrame} for a description of the layout.  The idea is to sample
     * one pixel from the middle of the 8 regions, and verify that the correct one has
     * the non-background color.  We can't know exactly what the video encoder has done
     * with our frames, so we just check to see if it looks like more or less the right thing.
     *
     * @return true if the frame looks good
     */
    private boolean checkFrame(int frameIndex, MediaFormat format, ByteBuffer frameData, Image image) {
        // Check for color formats we don't understand.  There is no requirement for video
        // decoders to use a "mundane" format, so we just give a pass on proprietary formats.
        // e.g. Nexus 4 0x7FA30C03 OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka
        int colorFormat = format.getInteger(MediaFormat.KEY_COLOR_FORMAT);
        if (!isRecognizedFormat(colorFormat)) {
            Log.d(TAG, "unable to check frame contents for colorFormat=" +
                    Integer.toHexString(colorFormat));
            return true;
        }

        boolean frameFailed = false;
        boolean semiPlanar = isSemiPlanarYUV(colorFormat);
        int width = format.getInteger(MediaFormat.KEY_STRIDE,
                format.getInteger(MediaFormat.KEY_WIDTH));
        int height = format.getInteger(MediaFormat.KEY_SLICE_HEIGHT,
                format.getInteger(MediaFormat.KEY_HEIGHT));
        int halfWidth = width / 2;
        int cropLeft = format.getInteger("crop-left");
        int cropRight = format.getInteger("crop-right");
        int cropTop = format.getInteger("crop-top");
        int cropBottom = format.getInteger("crop-bottom");
        if (image != null) {
            cropLeft = image.getCropRect().left;
            cropRight = image.getCropRect().right - 1;
            cropTop = image.getCropRect().top;
            cropBottom = image.getCropRect().bottom - 1;
        }
        int cropWidth = cropRight - cropLeft + 1;
        int cropHeight = cropBottom - cropTop + 1;

        assertEquals(mWidth, cropWidth);
        assertEquals(mHeight, cropHeight);

        for (int i = 0; i < 8; i++) {
            int x, y;
            if (i < 4) {
                x = i * (mWidth / 4) + (mWidth / 8);
                y = mHeight / 4;
            } else {
                x = (7 - i) * (mWidth / 4) + (mWidth / 8);
                y = (mHeight * 3) / 4;
            }

            y += cropTop;
            x += cropLeft;

            int testY, testU, testV;
            if (image != null) {
                Image.Plane[] planes = image.getPlanes();
                if (planes.length == 3 && image.getFormat() == ImageFormat.YUV_420_888) {
                    testY = planes[0].getBuffer().get(y * planes[0].getRowStride() + x * planes[0].getPixelStride()) & 0xff;
                    testU = planes[1].getBuffer().get((y/2) * planes[1].getRowStride() + (x/2) * planes[1].getPixelStride()) & 0xff;
                    testV = planes[2].getBuffer().get((y/2) * planes[2].getRowStride() + (x/2) * planes[2].getPixelStride()) & 0xff;
                } else {
                    testY = testU = testV = 0;
                }
            } else {
                int off = frameData.position();
                if (semiPlanar) {
                    // Galaxy Nexus uses OMX_TI_COLOR_FormatYUV420PackedSemiPlanar
                    testY = frameData.get(off + y * width + x) & 0xff;
                    testU = frameData.get(off + width*height + 2*(y/2) * halfWidth + 2*(x/2)) & 0xff;
                    testV = frameData.get(off + width*height + 2*(y/2) * halfWidth + 2*(x/2) + 1) & 0xff;
                } else {
                    // Nexus 10, Nexus 7 use COLOR_FormatYUV420Planar
                    testY = frameData.get(off + y * width + x) & 0xff;
                    testU = frameData.get(off + width*height + (y/2) * halfWidth + (x/2)) & 0xff;
                    testV = frameData.get(off + width*height + halfWidth * (height / 2) +
                            (y/2) * halfWidth + (x/2)) & 0xff;
                }
            }

            int expY, expU, expV;
            if (i == frameIndex % 8) {
                // colored rect
                expY = TEST_Y;
                expU = TEST_U;
                expV = TEST_V;
            } else {
                // should be our zeroed-out buffer
                expY = expU = expV = 0;
            }
            if (!isColorClose(testY, expY) ||
                    !isColorClose(testU, expU) ||
                    !isColorClose(testV, expV)) {
                Log.w(TAG, "Bad frame " + frameIndex + " (rect=" + i + ": yuv=" + testY +
                        "," + testU + "," + testV + " vs. expected " + expY + "," + expU +
                        "," + expV + ")");
                frameFailed = true;
            }
        }

        return !frameFailed;
    }

    /**
     * Generates a frame of data using GL commands.
     */
    private void generateSurfaceFrame(int frameIndex) {
        frameIndex %= 8;

        int startX, startY;
        if (frameIndex < 4) {
            // (0,0) is bottom-left in GL
            startX = frameIndex * (mWidth / 4);
            startY = mHeight / 2;
        } else {
            startX = (7 - frameIndex) * (mWidth / 4);
            startY = 0;
        }

        GLES20.glDisable(GLES20.GL_SCISSOR_TEST);
        GLES20.glClearColor(TEST_R0 / 255.0f, TEST_G0 / 255.0f, TEST_B0 / 255.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glEnable(GLES20.GL_SCISSOR_TEST);
        GLES20.glScissor(startX, startY, mWidth / 4, mHeight / 2);
        GLES20.glClearColor(TEST_R1 / 255.0f, TEST_G1 / 255.0f, TEST_B1 / 255.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
    }

    /**
     * Checks the frame for correctness.  Similar to {@link #checkFrame}, but uses GL to
     * read pixels from the current surface.
     *
     * @return true if the frame looks good
     */
    private boolean checkSurfaceFrame(int frameIndex) {
        ByteBuffer pixelBuf = ByteBuffer.allocateDirect(4); // TODO - reuse this
        boolean frameFailed = false;

        for (int i = 0; i < 8; i++) {
            // Note the coordinates are inverted on the Y-axis in GL.
            int x, y;
            if (i < 4) {
                x = i * (mWidth / 4) + (mWidth / 8);
                y = (mHeight * 3) / 4;
            } else {
                x = (7 - i) * (mWidth / 4) + (mWidth / 8);
                y = mHeight / 4;
            }

            GLES20.glReadPixels(x, y, 1, 1, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, pixelBuf);
            int r = pixelBuf.get(0) & 0xff;
            int g = pixelBuf.get(1) & 0xff;
            int b = pixelBuf.get(2) & 0xff;
            //Log.d(TAG, "GOT(" + frameIndex + "/" + i + "): r=" + r + " g=" + g + " b=" + b);

            int expR, expG, expB, expR_bt709, expG_bt709, expB_bt709;
            if (i == frameIndex % 8) {
                // colored rect
                expR = TEST_R1;
                expG = TEST_G1;
                expB = TEST_B1;
                expR_bt709 = TEST_R1_BT709;
                expG_bt709 = TEST_G1_BT709;
                expB_bt709 = TEST_B1_BT709;
            } else {
                // zero background color
                expR = TEST_R0;
                expG = TEST_G0;
                expB = TEST_B0;
                expR_bt709 = TEST_R0_BT709;
                expG_bt709 = TEST_G0_BT709;
                expB_bt709 = TEST_B0_BT709;
            }

            // Some decoders use BT.709 when converting HD (i.e. >= 720p)
            // frames from YUV to RGB, so check against both BT.601 and BT.709
            if (mAllowBT601 &&
                    isColorClose(r, expR) &&
                    isColorClose(g, expG) &&
                    isColorClose(b, expB)) {
                // frame OK on BT.601
                mAllowBT709 = false;
            } else if (mAllowBT709 &&
                           isColorClose(r, expR_bt709) &&
                           isColorClose(g, expG_bt709) &&
                           isColorClose(b, expB_bt709)) {
                // frame OK on BT.709
                mAllowBT601 = false;
            } else {
                Log.w(TAG, "Bad frame " + frameIndex + " (rect=" + i + " @ " + x + " " + y + ": rgb=" + r +
                        "," + g + "," + b + " vs. expected " + expR + "," + expG +
                        "," + expB + ")");
                frameFailed = true;
            }
        }

        return !frameFailed;
    }

    /**
     * Returns true if the actual color value is close to the expected color value.  Updates
     * mLargestColorDelta.
     */
    boolean isColorClose(int actual, int expected) {
        final int MAX_DELTA = 8;
        int delta = Math.abs(actual - expected);
        if (delta > mLargestColorDelta) {
            mLargestColorDelta = delta;
        }
        return (delta <= MAX_DELTA);
    }

    /**
     * Generates the presentation time for frame N, in microseconds.
     */
    private static long computePresentationTime(int frameIndex) {
        return 132 + frameIndex * 1000000 / FRAME_RATE;
    }
}
