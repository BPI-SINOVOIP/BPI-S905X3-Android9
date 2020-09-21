/*
 * Copyright (C) 2016 The Android Open Source Project
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

import android.media.cts.R;

import android.app.Instrumentation;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.media.cts.DecoderTest.AudioParameter;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecInfo.CodecCapabilities;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

import com.android.compatibility.common.util.CtsAndroidTestCase;

import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class DecoderTestAacDrc {
    private static final String TAG = "DecoderTestAacDrc";

    private Resources mResources;

    @Before
    public void setUp() throws Exception {
        final Instrumentation inst = InstrumentationRegistry.getInstrumentation();
        assertNotNull(inst);
        mResources = inst.getContext().getResources();
    }

    /**
     * Verify correct decoding of MPEG-4 AAC with output level normalization to -23dBFS.
     */
    @Test
    public void testDecodeAacDrcLevelM4a() throws Exception {
        AudioParameter decParams = new AudioParameter();
        // full boost, full cut, target ref level: -23dBFS, heavy compression: no
        DrcParams drcParams = new DrcParams(127, 127, 92, 0);
        short[] decSamples = decodeToMemory(decParams, R.raw.sine_2ch_48khz_aot5_drclevel_mp4,
                -1, null, drcParams, null /*decoderName: use default decoder*/);
        DecoderTest decTester = new DecoderTest();
        decTester.checkEnergy(decSamples, decParams, 2, 0.70f);
    }

    /**
     * Verify correct decoding of MPEG-4 AAC with Dynamic Range Control (DRC) metadata.
     * Fully apply light compression DRC (default settings).
     */
    @Test
    public void testDecodeAacDrcFullM4a() throws Exception {
        AudioParameter decParams = new AudioParameter();
        short[] decSamples = decodeToMemory(decParams, R.raw.sine_2ch_48khz_aot5_drcfull_mp4,
                -1, null, null, null /*decoderName: use default decoder*/);
        DecoderTest decTester = new DecoderTest();
        decTester.checkEnergy(decSamples, decParams, 2, 0.80f);
    }

    /**
     * Verify correct decoding of MPEG-4 AAC with Dynamic Range Control (DRC) metadata.
     * Apply only half of the light compression DRC and normalize to -20dBFS output level.
     */
    @Test
    public void testDecodeAacDrcHalfM4a() throws Exception {
        AudioParameter decParams = new AudioParameter();
        // half boost, half cut, target ref level: -20dBFS, heavy compression: no
        DrcParams drcParams = new DrcParams(63, 63, 80, 0);
        short[] decSamples = decodeToMemory(decParams, R.raw.sine_2ch_48khz_aot2_drchalf_mp4,
                -1, null, drcParams, null /*decoderName: use default decoder*/);
        DecoderTest decTester = new DecoderTest();
        decTester.checkEnergy(decSamples, decParams, 2, 0.80f);
    }

    /**
     * Verify correct decoding of MPEG-4 AAC with Dynamic Range Control (DRC) metadata.
     * Disable light compression DRC to test if MediaFormat keys reach the decoder.
     */
    @Test
    public void testDecodeAacDrcOffM4a() throws Exception {
        AudioParameter decParams = new AudioParameter();
        // no boost, no cut, target ref level: -16dBFS, heavy compression: no
        DrcParams drcParams = new DrcParams(0, 0, 64, 0);       // normalize to -16dBFS
        short[] decSamples = decodeToMemory(decParams, R.raw.sine_2ch_48khz_aot5_drcoff_mp4,
                -1, null, drcParams, null /*decoderName: use default decoder*/);
        DecoderTest decTester = new DecoderTest();
        decTester.checkEnergy(decSamples, decParams, 2, 0.80f);
    }

    /**
     * Verify correct decoding of MPEG-4 AAC with Dynamic Range Control (DRC) metadata.
     * Apply heavy compression gains and normalize to -16dBFS output level.
     */
    @Test
    public void testDecodeAacDrcHeavyM4a() throws Exception {
        AudioParameter decParams = new AudioParameter();
        // full boost, full cut, target ref level: -16dBFS, heavy compression: yes
        DrcParams drcParams = new DrcParams(127, 127, 64, 1);
        short[] decSamples = decodeToMemory(decParams, R.raw.sine_2ch_48khz_aot2_drcheavy_mp4,
                -1, null, drcParams, null /*decoderName: use default decoder*/);
        DecoderTest decTester = new DecoderTest();
        decTester.checkEnergy(decSamples, decParams, 2, 0.80f);
    }

    /**
     * Test signal limiting (without clipping) of MPEG-4 AAC decoder with the help of DRC metadata.
     * Uses a two channel 248 Hz sine tone at 48 kHz sampling rate for input.
     */
    @Test
    public void testDecodeAacDrcClipM4a() throws Exception {
        AudioParameter decParams = new AudioParameter();
        short[] decSamples = decodeToMemory(decParams, R.raw.sine_2ch_48khz_aot5_drcclip_mp4,
                -1, null, null, null /*decoderName: use default decoder*/);
        checkClipping(decSamples, decParams, 248.0f /* Hz */);
    }

    /**
     * Default decoder target level.
     * The actual default value used by the decoder can differ between platforms, or even devices,
     * but tests will measure energy relative to this value.
     */
    public static final int DEFAULT_DECODER_TARGET_LEVEL = 64; // -16.0 dBFs

    /**
     * Test USAC decoder with different target loudness levels
     */
    @Test
    public void testDecodeUsacLoudnessM4a() throws Exception {
        Log.v(TAG, "START testDecodeUsacLoudnessM4a");

        ArrayList<String> aacDecoderNames = DecoderTestXheAac.initAacDecoderNames();
        assertTrue("No AAC decoder found", aacDecoderNames.size() > 0);

        for (String aacDecName : aacDecoderNames) {
            // test default loudness
            // decoderTargetLevel = 64 --> target output level = -16.0 dBFs
            try {
                checkUsacLoudness(DEFAULT_DECODER_TARGET_LEVEL, 1, 1.0f, aacDecName);
            } catch (Exception e) {
                Log.v(TAG, "testDecodeUsacLoudnessM4a for default loudness failed for " +
                        aacDecName);
                throw new RuntimeException(e);
            }

            // test loudness boost
            // decoderTargetLevel = 40 --> target output level = -10.0 dBFs
            // normFactor = 1/(10^(-6/10)) = 3.98f
            //   where "-6" is the difference between the default level (-16), and -10 for this test
            try {
                checkUsacLoudness(40, 1, (float)(1.0f/Math.pow(10.0f, -6.0f/10.0f)), aacDecName);
            } catch (Exception e) {
                Log.v(TAG, "testDecodeUsacLoudnessM4a for loudness boost failed for " + aacDecName);
                throw new RuntimeException(e);
            }

            // test loudness attenuation
            // decoderTargetLevel = 96 --> target output level = -24.0 dBFs
            // normFactor = 1/(10^(8/10)) = 0.15f
            //     where 8 is the difference between the default level (-16), and -24 for this test
            try {
                checkUsacLoudness(96, 0, (float)(1.0f/Math.pow(10.0f, 8.0f/10.0f)), aacDecName);
            } catch (Exception e) {
                Log.v(TAG, "testDecodeUsacLoudnessM4a for loudness attenuation failed for "
                        + aacDecName);
                throw new RuntimeException(e);
            }
        }
    }

    /**
     *  Internal utilities
     */

    /**
     * The test routine performs a THD+N (Total Harmonic Distortion + Noise) analysis on a given
     * audio signal (decSamples). The THD+N value is defined here as harmonic distortion (+ noise)
     * RMS over full signal RMS.
     *
     * After the energy measurement of the unprocessed signal the routine creates and applies a
     * notch filter at the given frequency (sineFrequency). Afterwards the signal energy is
     * measured again. Then the THD+N value is calculated as the ratio of the filtered and the full
     * signal energy.
     *
     * The test passes if the THD+N value is lower than -60 dB. Otherwise it fails.
     *
     * @param decSamples the decoded audio samples to be tested
     * @param decParams the audio parameters of the given audio samples (decSamples)
     * @param sineFrequency frequency of the test signal tone used for testing
     * @throws RuntimeException
     */
    private void checkClipping(short[] decSamples, AudioParameter decParams, float sineFrequency)
            throws RuntimeException
    {
        final double threshold_clipping = -60.0; // dB
        final int numChannels = decParams.getNumChannels();
        final int startSample = 2 * 2048 * numChannels;          // exclude signal on- & offset to
        final int stopSample = decSamples.length - startSample;  // ... measure only the stationary
                                                                 // ... sine tone
        // get full energy of signal (all channels)
        double nrgFull = getEnergy(decSamples, startSample, stopSample);

        // create notch filter to suppress sine-tone at 248 Hz
        Biquad filter = new Biquad(sineFrequency, decParams.getSamplingRate());
        for (int channel = 0; channel < numChannels; channel++) {
            // apply notch-filter on buffer for each channel to filter out the sine tone.
            // only the harmonics (and noise) remain. */
            filter.apply(decSamples, channel, numChannels);
        }

        // get energy of harmonic distortion (signal without sine-tone)
        double nrgHd = getEnergy(decSamples, startSample, stopSample);

        // Total Harmonic Distortion + Noise, defined here as harmonic distortion (+ noise) RMS
        // over full signal RMS, given in dB
        double THDplusN = 10 * Math.log10(nrgHd / nrgFull);
        assertTrue("signal has clipping samples", THDplusN <= threshold_clipping);
    }

    /**
     * Measure the energy of a given signal over all channels within a given signal range.
     * @param signal audio signal samples
     * @param start start offset of the measuring range
     * @param stop stop sample which is the last sample of the measuring range
     * @return the signal energy in the given range
     */
    private double getEnergy(short[] signal, int start, int stop) {
        double nrg = 0.0;
        for (int sample = start; sample < stop; sample++) {
            double v = signal[sample];
            nrg += v * v;
        }
        return nrg;
    }

    // Notch filter implementation
    private class Biquad {
        // filter coefficients for biquad filter (2nd order IIR filter)
        float[] a;
        float[] b;
        // filter states
        float[] state_ff;
        float[] state_fb;

        protected float alpha = 0.95f;

        public Biquad(float f_notch, float f_s) {
            // Create filter coefficients of notch filter which suppresses a sine tone with f_notch
            // Hz at sampling frequency f_s. Zeros placed at unit circle at f_notch, poles placed
            // nearby the unit circle at f_notch.
            state_ff = new float[2];
            state_fb = new float[2];
            state_ff[0] = state_ff[1] = state_fb[0] = state_fb[1] = 0.0f;

            a = new float[3];
            b = new float[3];
            double omega = 2.0 * Math.PI * f_notch / f_s;
            a[0] = b[0] = b[2] = 1.0f;
            a[1] = -2.0f * alpha * (float)Math.cos(omega);
            a[2] = alpha * alpha;
            b[1] = -2.0f * (float)Math.cos(omega);
        }

        public void apply(short[] signal, int offset, int stride) {
            // reset states
            state_ff[0] = state_ff[1] = 0.0f;
            state_fb[0] = state_fb[1] = 0.0f;
            // process 2nd order IIR filter in Direct Form I
            float x_0, x_1, x_2, y_0, y_1, y_2;
            x_2 = state_ff[0];  // x[n-2]
            x_1 = state_ff[1];  // x[n-1]
            y_2 = state_fb[0];  // y[n-2]
            y_1 = state_fb[1];  // y[n-1]
            for (int sample = offset; sample < signal.length; sample += stride) {
                x_0 = signal[sample];
                y_0 = b[0] * x_0 + b[1] * x_1 + b[2] * x_2
                        - a[1] * y_1 - a[2] * y_2;
                x_2 = x_1;
                x_1 = x_0;
                y_2 = y_1;
                y_1 = y_0;
                signal[sample] = (short)y_0;
            }
            state_ff[0] = x_2;  // next x[n-2]
            state_ff[1] = x_1;  // next x[n-1]
            state_fb[0] = y_2;  // next y[n-2]
            state_fb[1] = y_1;  // next y[n-1]
        }
    }

    /**
     * USAC test DRC loudness
     */
    private void checkUsacLoudness(int decoderTargetLevel, int heavy, float normFactor,
            String decoderName) throws Exception {
        AudioParameter decParams = new AudioParameter();
        DrcParams drcParams_def  = new DrcParams(127, 127, DEFAULT_DECODER_TARGET_LEVEL, 1);
        DrcParams drcParams_test = new DrcParams(127, 127, decoderTargetLevel, heavy);

        short[] decSamples_def = decodeToMemory(decParams, R.raw.noise_2ch_48khz_aot42_19_lufs_mp4,
                -1, null, drcParams_def, decoderName);
        short[] decSamples_test = decodeToMemory(decParams, R.raw.noise_2ch_48khz_aot42_19_lufs_mp4,
                -1, null, drcParams_test, decoderName);

        DecoderTestXheAac decTesterXheAac = new DecoderTestXheAac();
        float[] nrg_def  = decTesterXheAac.checkEnergyUSAC(decSamples_def, decParams, 2, 1);
        float[] nrg_test = decTesterXheAac.checkEnergyUSAC(decSamples_test, decParams, 2, 1);

        float[] nrgThreshold = {2602510595620.0f, 2354652443657.0f};

        // Check default loudness behavior
        if (nrg_def[0] > nrgThreshold[0] || nrg_def[0] < nrgThreshold[1]) {
            throw new Exception("Default loudness behavior not as expected");
        }

        float nrgRatio = nrg_def[0]/nrg_test[0];

        // Check for loudness boost/attenuation if decoderTargetLevel deviates from default value
        // used in these tests (note that the default target level can change from platform
        // to platform, or device to device)
        if ((decoderTargetLevel < DEFAULT_DECODER_TARGET_LEVEL) // boosted loudness
                && (nrg_def[0] > nrg_test[0])) {
            throw new Exception("Signal not attenuated");
        } else if ((decoderTargetLevel > DEFAULT_DECODER_TARGET_LEVEL) // attenuated loudness
                && (nrg_def[0] < nrg_test[0])) {
            throw new Exception("Signal not boosted");
        }
        nrgRatio = nrgRatio * normFactor;

        // Check whether loudness behavior is as expected
        if (nrgRatio > 1.05f || nrgRatio < 0.95f ){
            throw new Exception("Loudness behavior not as expected");
        }
    }


    /**
     *  Class handling all MPEG-4 and MPEG-D Dynamic Range Control (DRC) parameter relevant for testing
     */
    protected static class DrcParams {
        int mBoost;                          // scaling of boosting gains
        int mCut;                            // scaling of compressing gains
        int mDecoderTargetLevel;             // desired target output level (for normalization)
        int mHeavy;                          // en-/disable heavy compression
        int mEffectType;                     // MPEG-D DRC Effect Type

        public DrcParams() {
            mBoost = 127;               // no scaling
            mCut   = 127;               // no scaling
            mHeavy = 1;                 // enabled
        }

        public DrcParams(int boost, int cut, int decoderTargetLevel, int heavy) {
            mBoost = boost;
            mCut = cut;
            mDecoderTargetLevel = decoderTargetLevel;
            mHeavy = heavy;
        }

        public DrcParams(int boost, int cut, int decoderTargetLevel, int heavy, int effectType) {
            mBoost = boost;
            mCut = cut;
            mDecoderTargetLevel = decoderTargetLevel;
            mHeavy = heavy;
            mEffectType = effectType;
        }
    }


    // TODO: code is the same as in DecoderTest, differences are:
    //          - addition of application of DRC parameters
    //          - no need/use of resetMode, configMode
    //       Split method so code can be shared
    private short[] decodeToMemory(AudioParameter audioParams, int testinput,
            int eossample, List<Long> timestamps, DrcParams drcParams, String decoderName)
            throws IOException
    {
        String localTag = TAG + "#decodeToMemory";
        short [] decoded = new short[0];
        int decodedIdx = 0;

        AssetFileDescriptor testFd = mResources.openRawResourceFd(testinput);

        MediaExtractor extractor;
        MediaCodec codec;
        ByteBuffer[] codecInputBuffers;
        ByteBuffer[] codecOutputBuffers;

        extractor = new MediaExtractor();
        extractor.setDataSource(testFd.getFileDescriptor(), testFd.getStartOffset(),
                testFd.getLength());
        testFd.close();

        assertEquals("wrong number of tracks", 1, extractor.getTrackCount());
        MediaFormat format = extractor.getTrackFormat(0);
        String mime = format.getString(MediaFormat.KEY_MIME);
        assertTrue("not an audio file", mime.startsWith("audio/"));

        MediaFormat configFormat = format;
        if (decoderName == null) {
            codec = MediaCodec.createDecoderByType(mime);
        } else {
            codec = MediaCodec.createByCodecName(decoderName);
        }

        // set DRC parameters
        if (drcParams != null) {
            configFormat.setInteger(MediaFormat.KEY_AAC_DRC_BOOST_FACTOR, drcParams.mBoost);
            configFormat.setInteger(MediaFormat.KEY_AAC_DRC_ATTENUATION_FACTOR, drcParams.mCut);
            if (drcParams.mDecoderTargetLevel != 0) {
                configFormat.setInteger(MediaFormat.KEY_AAC_DRC_TARGET_REFERENCE_LEVEL,
                        drcParams.mDecoderTargetLevel);
            }
            configFormat.setInteger(MediaFormat.KEY_AAC_DRC_HEAVY_COMPRESSION, drcParams.mHeavy);
        }
        Log.v(localTag, "configuring with " + configFormat);
        codec.configure(configFormat, null /* surface */, null /* crypto */, 0 /* flags */);

        codec.start();
        codecInputBuffers = codec.getInputBuffers();
        codecOutputBuffers = codec.getOutputBuffers();

        extractor.selectTrack(0);

        // start decoding
        final long kTimeOutUs = 5000;
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        boolean sawInputEOS = false;
        boolean sawOutputEOS = false;
        int noOutputCounter = 0;
        int samplecounter = 0;
        while (!sawOutputEOS && noOutputCounter < 50) {
            noOutputCounter++;
            if (!sawInputEOS) {
                int inputBufIndex = codec.dequeueInputBuffer(kTimeOutUs);

                if (inputBufIndex >= 0) {
                    ByteBuffer dstBuf = codecInputBuffers[inputBufIndex];

                    int sampleSize =
                        extractor.readSampleData(dstBuf, 0 /* offset */);

                    long presentationTimeUs = 0;

                    if (sampleSize < 0 && eossample > 0) {
                        fail("test is broken: never reached eos sample");
                    }
                    if (sampleSize < 0) {
                        Log.d(TAG, "saw input EOS.");
                        sawInputEOS = true;
                        sampleSize = 0;
                    } else {
                        if (samplecounter == eossample) {
                            sawInputEOS = true;
                        }
                        samplecounter++;
                        presentationTimeUs = extractor.getSampleTime();
                    }
                    codec.queueInputBuffer(
                            inputBufIndex,
                            0 /* offset */,
                            sampleSize,
                            presentationTimeUs,
                            sawInputEOS ? MediaCodec.BUFFER_FLAG_END_OF_STREAM : 0);

                    if (!sawInputEOS) {
                        extractor.advance();
                    }
                }
            }

            int res = codec.dequeueOutputBuffer(info, kTimeOutUs);

            if (res >= 0) {
                //Log.d(TAG, "got frame, size " + info.size + "/" + info.presentationTimeUs);

                if (info.size > 0) {
                    noOutputCounter = 0;
                    if (timestamps != null) {
                        timestamps.add(info.presentationTimeUs);
                    }
                }

                int outputBufIndex = res;
                ByteBuffer buf = codecOutputBuffers[outputBufIndex];

                if (decodedIdx + (info.size / 2) >= decoded.length) {
                    decoded = Arrays.copyOf(decoded, decodedIdx + (info.size / 2));
                }

                buf.position(info.offset);
                for (int i = 0; i < info.size; i += 2) {
                    decoded[decodedIdx++] = buf.getShort();
                }

                codec.releaseOutputBuffer(outputBufIndex, false /* render */);

                if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                    Log.d(TAG, "saw output EOS.");
                    sawOutputEOS = true;
                }
            } else if (res == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                codecOutputBuffers = codec.getOutputBuffers();

                Log.d(TAG, "output buffers have changed.");
            } else if (res == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                MediaFormat oformat = codec.getOutputFormat();
                audioParams.setNumChannels(oformat.getInteger(MediaFormat.KEY_CHANNEL_COUNT));
                audioParams.setSamplingRate(oformat.getInteger(MediaFormat.KEY_SAMPLE_RATE));
                Log.d(TAG, "output format has changed to " + oformat);
            } else {
                Log.d(TAG, "dequeueOutputBuffer returned " + res);
            }
        }
        if (noOutputCounter >= 50) {
            fail("decoder stopped outputing data");
        }

        codec.stop();
        codec.release();
        return decoded;
    }

}

