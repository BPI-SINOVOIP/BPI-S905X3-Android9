/*
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

package android.media.cts;

import android.media.cts.R;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.audiofx.AudioEffect;
import android.media.audiofx.DynamicsProcessing;
import android.media.audiofx.DynamicsProcessing.BandBase;
import android.media.audiofx.DynamicsProcessing.BandStage;
import android.media.audiofx.DynamicsProcessing.Channel;
import android.media.audiofx.DynamicsProcessing.Eq;
import android.media.audiofx.DynamicsProcessing.EqBand;
import android.media.audiofx.DynamicsProcessing.Limiter;
import android.media.audiofx.DynamicsProcessing.Mbc;
import android.media.audiofx.DynamicsProcessing.MbcBand;
import android.platform.test.annotations.AppModeFull;
import android.test.AndroidTestCase;
import android.util.Log;

public class DynamicsProcessingTest extends PostProcTestBase {

    private static final String TAG = "DynamicsProcessingTest";
    private DynamicsProcessing mDP;

    private static final int MIN_CHANNEL_COUNT = 1;
    private static final float EPSILON = 0.00001f;
    private static final int DEFAULT_VARIANT =
            DynamicsProcessing.VARIANT_FAVOR_FREQUENCY_RESOLUTION;
    private static final boolean DEFAULT_PREEQ_IN_USE = true;
    private static final int DEFAULT_PREEQ_BAND_COUNT = 2;
    private static final boolean DEFAULT_MBC_IN_USE = true;
    private static final int DEFAULT_MBC_BAND_COUNT = 2;
    private static final boolean DEFAULT_POSTEQ_IN_USE = true;
    private static final int DEFAULT_POSTEQ_BAND_COUNT = 2;
    private static final boolean DEFAULT_LIMITER_IN_USE = true;
    private static final float DEFAULT_FRAME_DURATION = 9.5f;
    private static final float DEFAULT_INPUT_GAIN = -12.5f;

    private static final int TEST_CHANNEL_COUNT = 2;
    private static final float TEST_GAIN1 = 12.1f;
    private static final float TEST_GAIN2 = -2.8f;
    private static final int TEST_CHANNEL_INDEX = 0;
    private static final int TEST_BAND_INDEX = 0;

    // -----------------------------------------------------------------
    // DynamicsProcessing tests:
    // ----------------------------------

    // -----------------------------------------------------------------
    // 0 - constructors
    // ----------------------------------

    // Test case 0.0: test constructor and release
    @AppModeFull(reason = "Fails for instant but not enough to block the release")
    public void test0_0ConstructorAndRelease() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        try {
            AudioManager am = (AudioManager) getContext().getSystemService(Context.AUDIO_SERVICE);
            assertNotNull("null AudioManager", am);
            createDynamicsProcessing(AudioManager.AUDIO_SESSION_ID_GENERATE);
            releaseDynamicsProcessing();

            final int session = am.generateAudioSessionId();
            assertTrue("cannot generate new session", session != AudioManager.ERROR);
            createDynamicsProcessing(session);
        } finally {
            releaseDynamicsProcessing();
        }
    }

    public void test0_1ConstructorWithConfigAndRelease() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        try {
            createDefaultEffect();
        } finally {
            releaseDynamicsProcessing();
        }
    }

    // -----------------------------------------------------------------
    // 1 - create with parameters
    // ----------------------------------

    public void test1_0ParametersEngine() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        try {
            createDefaultEffect();

            // Check Parameters:
            DynamicsProcessing.Config engineConfig = mDP.getConfig();
            final float preferredFrameDuration = engineConfig.getPreferredFrameDuration();
            assertEquals("preferredFrameDuration is different", DEFAULT_FRAME_DURATION,
                    preferredFrameDuration, EPSILON);

            final int preEqBandCount = engineConfig.getPreEqBandCount();
            assertEquals("preEqBandCount is different", DEFAULT_PREEQ_BAND_COUNT, preEqBandCount);

            final int mbcBandCount = engineConfig.getMbcBandCount();
            assertEquals("mbcBandCount is different", DEFAULT_MBC_BAND_COUNT, mbcBandCount);

            final int postEqBandCount = engineConfig.getPostEqBandCount();
            assertEquals("postEqBandCount is different", DEFAULT_POSTEQ_BAND_COUNT,
                    postEqBandCount);
        } finally {
            releaseDynamicsProcessing();
        }
    }

    public void test1_1ParametersChannel() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        try {
            createDefaultEffect();

            // Check Parameters:
            final int channelCount = mDP.getChannelCount();
            assertTrue("unexpected channel count", channelCount >= MIN_CHANNEL_COUNT);

            Channel channel = mDP.getChannelByChannelIndex(TEST_CHANNEL_INDEX);

            final float inputGain = channel.getInputGain();
            assertEquals("inputGain is different", DEFAULT_INPUT_GAIN, inputGain, EPSILON);
        } finally {
            releaseDynamicsProcessing();
        }
    }

    public void test1_2ParametersPreEq() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        try {
            createDefaultEffect();

            DynamicsProcessing.Eq eq = mDP.getPreEqByChannelIndex(TEST_CHANNEL_INDEX);

            final boolean inUse = eq.isInUse();
            assertEquals("inUse is different", DEFAULT_PREEQ_IN_USE, inUse);

            final int bandCount = eq.getBandCount();
            assertEquals("band count is different", DEFAULT_PREEQ_BAND_COUNT, bandCount);
            releaseDynamicsProcessing();
        } finally {
            releaseDynamicsProcessing();
        }
    }

    public void test1_3ParametersMbc() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        try {
            createDefaultEffect();

            DynamicsProcessing.Mbc mbc = mDP.getMbcByChannelIndex(TEST_CHANNEL_INDEX);

            final boolean inUse = mbc.isInUse();
            assertEquals("inUse is different", DEFAULT_MBC_IN_USE, inUse);

            final int bandCount = mbc.getBandCount();
            assertEquals("band count is different", DEFAULT_MBC_BAND_COUNT, bandCount);
        } finally {
            releaseDynamicsProcessing();
        }
    }

    public void test1_4ParametersPostEq() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        try {
            createDefaultEffect();

            DynamicsProcessing.Eq eq = mDP.getPostEqByChannelIndex(TEST_CHANNEL_INDEX);

            boolean inUse = eq.isInUse();
            assertEquals("inUse is different", DEFAULT_POSTEQ_IN_USE, inUse);

            int bandCount = eq.getBandCount();
            assertEquals("band count is different", DEFAULT_POSTEQ_BAND_COUNT, bandCount);
        } finally {
            releaseDynamicsProcessing();
        }
    }

    public void test1_5ParametersLimiter() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        try {
            createDefaultEffect();

            DynamicsProcessing.Limiter limiter = mDP.getLimiterByChannelIndex(TEST_CHANNEL_INDEX);

            final boolean inUse = limiter.isInUse();
            assertEquals("inUse is different", DEFAULT_LIMITER_IN_USE, inUse);
        } finally {
            releaseDynamicsProcessing();
        }
    }

    public void test1_6Channel_perStage() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        try {
            createDefaultEffect();

            Channel channel = mDP.getChannelByChannelIndex(TEST_CHANNEL_INDEX);

            // Per Stage
            mDP.setInputGainAllChannelsTo(TEST_GAIN1);

            Eq preEq = mDP.getPreEqByChannelIndex(TEST_CHANNEL_INDEX);
            EqBand preEqBand = preEq.getBand(TEST_BAND_INDEX);
            preEqBand.setGain(TEST_GAIN1);
            preEq.setBand(TEST_BAND_INDEX, preEqBand);
            mDP.setPreEqAllChannelsTo(preEq);

            Mbc mbc = mDP.getMbcByChannelIndex(TEST_CHANNEL_INDEX);
            MbcBand mbcBand = mbc.getBand(TEST_BAND_INDEX);
            mbcBand.setPreGain(TEST_GAIN1);
            mbc.setBand(TEST_BAND_INDEX, mbcBand);
            mDP.setMbcAllChannelsTo(mbc);

            Eq postEq = mDP.getPostEqByChannelIndex(TEST_CHANNEL_INDEX);
            EqBand postEqBand = postEq.getBand(TEST_BAND_INDEX);
            postEqBand.setGain(TEST_GAIN1);
            postEq.setBand(TEST_BAND_INDEX, postEqBand);
            mDP.setPostEqAllChannelsTo(postEq);

            Limiter limiter = mDP.getLimiterByChannelIndex(TEST_CHANNEL_INDEX);
            limiter.setPostGain(TEST_GAIN1);
            mDP.setLimiterAllChannelsTo(limiter);

            int channelCount = mDP.getChannelCount();
            for (int i = 0; i < channelCount; i++) {
                Channel channelTest = mDP.getChannelByChannelIndex(i);
                assertEquals("inputGain is different in channel " + i, TEST_GAIN1,
                        channelTest.getInputGain(), EPSILON);

                Eq preEqTest = new Eq(mDP.getPreEqByChannelIndex(i));
                EqBand preEqBandTest = preEqTest.getBand(TEST_BAND_INDEX);
                assertEquals("preEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN1, preEqBandTest.getGain(), EPSILON);

                Mbc mbcTest = new Mbc(mDP.getMbcByChannelIndex(i));
                MbcBand mbcBandTest = mbcTest.getBand(TEST_BAND_INDEX);
                assertEquals("mbcBand preGain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN1, mbcBandTest.getPreGain(), EPSILON);

                Eq postEqTest = new Eq(mDP.getPostEqByChannelIndex(i));
                EqBand postEqBandTest = postEqTest.getBand(TEST_BAND_INDEX);
                assertEquals("postEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN1, postEqBandTest.getGain(), EPSILON);

                Limiter limiterTest = new Limiter(mDP.getLimiterByChannelIndex(i));
                assertEquals("limiter gain is different in channel " + i,
                        TEST_GAIN1, limiterTest.getPostGain(), EPSILON);

                // change by Stage
                mDP.setInputGainbyChannel(i, TEST_GAIN2);
                assertEquals("inputGain is different in channel " + i, TEST_GAIN2,
                        mDP.getInputGainByChannelIndex(i), EPSILON);

                preEqBandTest.setGain(TEST_GAIN2);
                preEqTest.setBand(TEST_BAND_INDEX, preEqBandTest);
                mDP.setPreEqByChannelIndex(i, preEqTest);
                assertEquals("preEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN2,
                        mDP.getPreEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(), EPSILON);

                mbcBandTest.setPreGain(TEST_GAIN2);
                mbcTest.setBand(TEST_BAND_INDEX, mbcBandTest);
                mDP.setMbcByChannelIndex(i, mbcTest);
                assertEquals("mbcBand preGain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN2,
                        mDP.getMbcBandByChannelIndex(i, TEST_BAND_INDEX).getPreGain(), EPSILON);

                postEqBandTest.setGain(TEST_GAIN2);
                postEqTest.setBand(TEST_BAND_INDEX, postEqBandTest);
                mDP.setPostEqByChannelIndex(i, postEqTest);
                assertEquals("postEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN2,
                        mDP.getPostEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(), EPSILON);

                limiterTest.setPostGain(TEST_GAIN2);
                mDP.setLimiterByChannelIndex(i, limiterTest);
                assertEquals("limiter gain is different in channel " + i,
                        TEST_GAIN2, mDP.getLimiterByChannelIndex(i).getPostGain(), EPSILON);
            }
        } finally {
            releaseDynamicsProcessing();
        }

    }

    public void test1_7Channel_perBand() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        try {
            createDefaultEffect();

            Channel channel = mDP.getChannelByChannelIndex(TEST_CHANNEL_INDEX);

            // Per Band
            EqBand preEqBand = mDP.getPreEqBandByChannelIndex(TEST_CHANNEL_INDEX, TEST_BAND_INDEX);
            preEqBand.setGain(TEST_GAIN1);
            mDP.setPreEqBandAllChannelsTo(TEST_BAND_INDEX, preEqBand);

            MbcBand mbcBand = mDP.getMbcBandByChannelIndex(TEST_CHANNEL_INDEX, TEST_BAND_INDEX);
            mbcBand.setPreGain(TEST_GAIN1);
            mDP.setMbcBandAllChannelsTo(TEST_BAND_INDEX, mbcBand);

            EqBand postEqBand = mDP.getPostEqBandByChannelIndex(TEST_CHANNEL_INDEX,
                    TEST_BAND_INDEX);
            postEqBand.setGain(TEST_GAIN1);
            mDP.setPostEqBandAllChannelsTo(TEST_BAND_INDEX, postEqBand);

            int channelCount = mDP.getChannelCount();

            for (int i = 0; i < channelCount; i++) {

                EqBand preEqBandTest = new EqBand(mDP.getPreEqBandByChannelIndex(i,
                        TEST_BAND_INDEX));
                assertEquals("preEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN1, preEqBandTest.getGain(), EPSILON);

                MbcBand mbcBandTest = new MbcBand(mDP.getMbcBandByChannelIndex(i, TEST_BAND_INDEX));
                assertEquals("mbcBand preGain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN1, mbcBandTest.getPreGain(), EPSILON);

                EqBand postEqBandTest = new EqBand(mDP.getPostEqBandByChannelIndex(i,
                        TEST_BAND_INDEX));
                assertEquals("postEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN1, postEqBandTest.getGain(), EPSILON);

                // change per Band
                preEqBandTest.setGain(TEST_GAIN2);
                mDP.setPreEqBandByChannelIndex(i, TEST_BAND_INDEX, preEqBandTest);
                assertEquals("preEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN2,
                        mDP.getPreEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(), EPSILON);

                mbcBandTest.setPreGain(TEST_GAIN2);
                mDP.setMbcBandByChannelIndex(i, TEST_BAND_INDEX, mbcBandTest);
                assertEquals("mbcBand preGain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN2,
                        mDP.getMbcBandByChannelIndex(i, TEST_BAND_INDEX).getPreGain(),
                        EPSILON);

                postEqBandTest.setGain(TEST_GAIN2);
                mDP.setPostEqBandByChannelIndex(i, TEST_BAND_INDEX, postEqBandTest);
                assertEquals("postEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN2,
                        mDP.getPostEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                        EPSILON);
            }

        } finally {
            releaseDynamicsProcessing();
        }
    }

    public void test1_8Channel_setAllChannelsTo() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        try {
            createDefaultEffect();

            Channel channel = mDP.getChannelByChannelIndex(TEST_CHANNEL_INDEX);
            // get Stages, apply all channels
            Eq preEq = new Eq(mDP.getPreEqByChannelIndex(TEST_CHANNEL_INDEX));
            EqBand preEqBand = new EqBand(preEq.getBand(TEST_BAND_INDEX));
            preEqBand.setGain(TEST_GAIN1);
            preEq.setBand(TEST_BAND_INDEX, preEqBand);
            channel.setPreEq(preEq);

            Mbc mbc = new Mbc(mDP.getMbcByChannelIndex(TEST_CHANNEL_INDEX));
            MbcBand mbcBand = new MbcBand(mbc.getBand(TEST_BAND_INDEX));
            mbcBand.setPreGain(TEST_GAIN1);
            mbc.setBand(TEST_BAND_INDEX, mbcBand);
            channel.setMbc(mbc);

            Eq postEq = new Eq(mDP.getPostEqByChannelIndex(TEST_CHANNEL_INDEX));
            EqBand postEqBand = new EqBand(postEq.getBand(TEST_BAND_INDEX));
            postEqBand.setGain(TEST_GAIN1);
            postEq.setBand(TEST_BAND_INDEX, postEqBand);
            channel.setPostEq(postEq);

            Limiter limiter = new Limiter(mDP.getLimiterByChannelIndex(TEST_CHANNEL_INDEX));
            limiter.setPostGain(TEST_GAIN1);
            channel.setLimiter(limiter);

            mDP.setAllChannelsTo(channel);

            int channelCount = mDP.getChannelCount();
            for (int i = 0; i < channelCount; i++) {
                assertEquals("preEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN1,
                        mDP.getPreEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(), EPSILON);

                assertEquals("mbcBand preGain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN1,
                        mDP.getMbcBandByChannelIndex(i, TEST_BAND_INDEX).getPreGain(), EPSILON);

                assertEquals("postEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, TEST_GAIN1,
                        mDP.getPostEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(), EPSILON);

                assertEquals("limiter gain is different in channel " + i,
                        TEST_GAIN1, mDP.getLimiterByChannelIndex(i).getPostGain(), EPSILON);
            }

        } finally {
            releaseDynamicsProcessing();
        }
    }

    public void test1_9Channel_setChannelTo() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        try {
            createDefaultEffect();

            Channel channel = mDP.getChannelByChannelIndex(TEST_CHANNEL_INDEX);

            Eq preEq = new Eq(mDP.getPreEqByChannelIndex(TEST_CHANNEL_INDEX));
            EqBand preEqBand = new EqBand(preEq.getBand(TEST_BAND_INDEX));

            Mbc mbc = new Mbc(mDP.getMbcByChannelIndex(TEST_CHANNEL_INDEX));
            MbcBand mbcBand = new MbcBand(mbc.getBand(TEST_BAND_INDEX));

            Eq postEq = new Eq(mDP.getPostEqByChannelIndex(TEST_CHANNEL_INDEX));
            EqBand postEqBand = new EqBand(postEq.getBand(TEST_BAND_INDEX));

            Limiter limiter = new Limiter(mDP.getLimiterByChannelIndex(TEST_CHANNEL_INDEX));

            // get Stages, apply per channel
            int channelCount = mDP.getChannelCount();
            for (int i = 0; i < channelCount; i++) {
                float gain = i % 2 == 0 ? TEST_GAIN1 : TEST_GAIN2;

                preEqBand.setGain(gain);
                preEq.setBand(TEST_BAND_INDEX, preEqBand);
                channel.setPreEq(preEq);

                mbcBand.setPreGain(gain);
                mbc.setBand(TEST_BAND_INDEX, mbcBand);
                channel.setMbc(mbc);

                postEqBand.setGain(gain);
                postEq.setBand(TEST_BAND_INDEX, postEqBand);
                channel.setPostEq(postEq);

                limiter.setPostGain(gain);
                channel.setLimiter(limiter);

                mDP.setChannelTo(i, channel);
            }

            for (int i = 0; i < channelCount; i++) {
                float expectedGain = i % 2 == 0 ? TEST_GAIN1 : TEST_GAIN2;
                assertEquals("preEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, expectedGain,
                        mDP.getPreEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                        EPSILON);

                assertEquals("mbcBand preGain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, expectedGain,
                        mDP.getMbcBandByChannelIndex(i, TEST_BAND_INDEX).getPreGain(),
                        EPSILON);

                assertEquals("postEQBand gain is different in channel " + i + " band " +
                        TEST_BAND_INDEX, expectedGain,
                        mDP.getPostEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                        EPSILON);

                assertEquals("limiter gain is different in channel " + i,
                        expectedGain, mDP.getLimiterByChannelIndex(i).getPostGain(), EPSILON);
            }

        } finally {
            releaseDynamicsProcessing();
        }
    }

    // -----------------------------------------------------------------
    // 2 - config builder tests
    // ----------------------------------

    public void test2_0ConfigBasic() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        DynamicsProcessing.Config config = getBuilderWithValues().build();

        assertEquals("getVariant is different", DEFAULT_VARIANT,
                config.getVariant());
        assertEquals("isPreEqInUse is different", DEFAULT_PREEQ_IN_USE,
                config.isPreEqInUse());
        assertEquals("getPreEqBandCount is different", DEFAULT_PREEQ_BAND_COUNT,
                config.getPreEqBandCount());
        assertEquals("isMbcInUse is different", DEFAULT_MBC_IN_USE,
                config.isMbcInUse());
        assertEquals("getMbcBandCount is different", DEFAULT_MBC_BAND_COUNT,
                config.getMbcBandCount());
        assertEquals("isPostEqInUse is different", DEFAULT_POSTEQ_IN_USE,
                config.isPostEqInUse());
        assertEquals("getPostEqBandCount is different", DEFAULT_POSTEQ_BAND_COUNT,
                config.getPostEqBandCount());
        assertEquals("isLimiterInUse is different", DEFAULT_LIMITER_IN_USE,
                config.isLimiterInUse());
        assertEquals("getPreferredFrameDuration is different", DEFAULT_FRAME_DURATION,
                config.getPreferredFrameDuration());
    }

    public void test2_1ConfigChannel() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        DynamicsProcessing.Config config = getBuilderWithValues(TEST_CHANNEL_COUNT).build();

        // per channel
        Channel channel = config.getChannelByChannelIndex(TEST_CHANNEL_INDEX);
        // change some parameters
        channel.setInputGain(TEST_GAIN1);

        // get stages from channel
        Eq preEq = channel.getPreEq();
        EqBand preEqBand = preEq.getBand(TEST_BAND_INDEX);
        preEqBand.setGain(TEST_GAIN1);
        channel.setPreEqBand(TEST_BAND_INDEX, preEqBand);

        Mbc mbc = channel.getMbc();
        MbcBand mbcBand = mbc.getBand(TEST_BAND_INDEX);
        mbcBand.setPreGain(TEST_GAIN1);
        channel.setMbcBand(TEST_BAND_INDEX, mbcBand);

        Eq postEq = channel.getPostEq();
        EqBand postEqBand = postEq.getBand(TEST_BAND_INDEX);
        postEqBand.setGain(TEST_GAIN1);
        channel.setPostEqBand(TEST_BAND_INDEX, postEqBand);

        Limiter limiter = channel.getLimiter();
        limiter.setPostGain(TEST_GAIN1);
        channel.setLimiter(limiter);

        config.setAllChannelsTo(channel);
        for (int i = 0; i < TEST_CHANNEL_COUNT; i++) {
            Channel channelTest = new Channel(config.getChannelByChannelIndex(i));
            assertEquals("inputGain is different in channel " + i, TEST_GAIN1,
                    channelTest.getInputGain(), EPSILON);

            EqBand preEqBandTest = new EqBand(channelTest.getPreEqBand(TEST_BAND_INDEX));
            assertEquals("preEQBand gain is different in channel " + i + " band " + TEST_BAND_INDEX,
                    TEST_GAIN1, preEqBandTest.getGain(), EPSILON);

            MbcBand mbcBandTest = new MbcBand(channelTest.getMbcBand(TEST_BAND_INDEX));
            assertEquals("mbcBand preGain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN1, mbcBandTest.getPreGain(), EPSILON);

            EqBand postEqBandTest = new EqBand(channelTest.getPostEqBand(TEST_BAND_INDEX));
            assertEquals("postEQBand gain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN1, postEqBandTest.getGain(), EPSILON);

            Limiter limiterTest = new Limiter(channelTest.getLimiter());
            assertEquals("limiter gain is different in channel " + i,
                    TEST_GAIN1, limiterTest.getPostGain(), EPSILON);

            /// changes per channelIndex
            channelTest.setInputGain(TEST_GAIN2);
            preEqBandTest.setGain(TEST_GAIN2);
            channelTest.setPreEqBand(TEST_BAND_INDEX, preEqBandTest);
            mbcBandTest.setPreGain(TEST_GAIN2);
            channelTest.setMbcBand(TEST_BAND_INDEX, mbcBandTest);
            postEqBandTest.setGain(TEST_GAIN2);
            channelTest.setPostEqBand(TEST_BAND_INDEX, postEqBandTest);
            limiterTest.setPostGain(TEST_GAIN2);
            channelTest.setLimiter(limiterTest);
            config.setChannelTo(i, channelTest);

            assertEquals("inputGain is different in channel " + i, TEST_GAIN2,
                    config.getInputGainByChannelIndex(i), EPSILON);

            // get by module
            Eq preEqTest = config.getPreEqByChannelIndex(i);
            assertEquals("preEQBand gain is different in channel " + i + " band " + TEST_BAND_INDEX,
                    TEST_GAIN2, preEqTest.getBand(TEST_BAND_INDEX).getGain(), EPSILON);

            Mbc mbcTest = config.getMbcByChannelIndex(i);
            assertEquals("mbcBand preGain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN2, mbcTest.getBand(TEST_BAND_INDEX).getPreGain(),
                    EPSILON);

            Eq postEqTest = config.getPostEqByChannelIndex(i);
            assertEquals("postEQBand gain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN2, postEqTest.getBand(TEST_BAND_INDEX).getGain(),
                    EPSILON);

            limiterTest = config.getLimiterByChannelIndex(i);
            assertEquals("limiter gain is different in channel " + i,
                    TEST_GAIN2, limiterTest.getPostGain(), EPSILON);
        }
    }

    public void test2_2ConfigChannel_perStage() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        DynamicsProcessing.Config config = getBuilderWithValues(TEST_CHANNEL_COUNT).build();

        // Per Stage
        config.setInputGainAllChannelsTo(TEST_GAIN1);

        Eq preEq = config.getPreEqByChannelIndex(TEST_CHANNEL_INDEX);
        EqBand preEqBand = preEq.getBand(TEST_BAND_INDEX);
        preEqBand.setGain(TEST_GAIN1);
        preEq.setBand(TEST_BAND_INDEX, preEqBand);
        config.setPreEqAllChannelsTo(preEq);

        Mbc mbc = config.getMbcByChannelIndex(TEST_CHANNEL_INDEX);
        MbcBand mbcBand = mbc.getBand(TEST_BAND_INDEX);
        mbcBand.setPreGain(TEST_GAIN1);
        mbc.setBand(TEST_BAND_INDEX, mbcBand);
        config.setMbcAllChannelsTo(mbc);

        Eq postEq = config.getPostEqByChannelIndex(TEST_CHANNEL_INDEX);
        EqBand postEqBand = postEq.getBand(TEST_BAND_INDEX);
        postEqBand.setGain(TEST_GAIN1);
        postEq.setBand(TEST_BAND_INDEX, postEqBand);
        config.setPostEqAllChannelsTo(postEq);

        Limiter limiter = config.getLimiterByChannelIndex(TEST_CHANNEL_INDEX);
        limiter.setPostGain(TEST_GAIN1);
        config.setLimiterAllChannelsTo(limiter);

        for (int i = 0; i < TEST_CHANNEL_COUNT; i++) {
            Channel channelTest = config.getChannelByChannelIndex(i);
            assertEquals("inputGain is different in channel " + i, TEST_GAIN1,
                    channelTest.getInputGain(), EPSILON);

            Eq preEqTest = new Eq(config.getPreEqByChannelIndex(i));
            EqBand preEqBandTest = preEqTest.getBand(TEST_BAND_INDEX);
            assertEquals("preEQBand gain is different in channel " + i + " band " + TEST_BAND_INDEX,
                    TEST_GAIN1, preEqBandTest.getGain(), EPSILON);

            Mbc mbcTest = new Mbc(config.getMbcByChannelIndex(i));
            MbcBand mbcBandTest = mbcTest.getBand(TEST_BAND_INDEX);
            assertEquals("mbcBand preGain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN1, mbcBandTest.getPreGain(), EPSILON);

            Eq postEqTest = new Eq(config.getPostEqByChannelIndex(i));
            EqBand postEqBandTest = postEqTest.getBand(TEST_BAND_INDEX);
            assertEquals("postEQBand gain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN1, postEqBandTest.getGain(), EPSILON);

            Limiter limiterTest = new Limiter(config.getLimiterByChannelIndex(i));
            assertEquals("limiter gain is different in channel " + i,
                    TEST_GAIN1, limiterTest.getPostGain(), EPSILON);

            // change by Stage
            config.setInputGainByChannelIndex(i, TEST_GAIN2);
            assertEquals("inputGain is different in channel " + i, TEST_GAIN2,
                    config.getInputGainByChannelIndex(i), EPSILON);

            preEqBandTest.setGain(TEST_GAIN2);
            preEqTest.setBand(TEST_BAND_INDEX, preEqBandTest);
            config.setPreEqByChannelIndex(i, preEqTest);
            assertEquals("preEQBand gain is different in channel " + i + " band " + TEST_BAND_INDEX,
                    TEST_GAIN2, config.getPreEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                    EPSILON);

            mbcBandTest.setPreGain(TEST_GAIN2);
            mbcTest.setBand(TEST_BAND_INDEX, mbcBandTest);
            config.setMbcByChannelIndex(i, mbcTest);
            assertEquals("mbcBand preGain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN2,
                    config.getMbcBandByChannelIndex(i, TEST_BAND_INDEX).getPreGain(), EPSILON);

            postEqBandTest.setGain(TEST_GAIN2);
            postEqTest.setBand(TEST_BAND_INDEX, postEqBandTest);
            config.setPostEqByChannelIndex(i, postEqTest);
            assertEquals("postEQBand gain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN2,
                    config.getPostEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(), EPSILON);

            limiterTest.setPostGain(TEST_GAIN2);
            config.setLimiterByChannelIndex(i, limiterTest);
            assertEquals("limiter gain is different in channel " + i,
                    TEST_GAIN2, config.getLimiterByChannelIndex(i).getPostGain(), EPSILON);
        }
    }

    public void test2_3ConfigChannel_perBand() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        DynamicsProcessing.Config config = getBuilderWithValues(TEST_CHANNEL_COUNT).build();

        // Per Band
        EqBand preEqBand = config.getPreEqBandByChannelIndex(TEST_CHANNEL_INDEX, TEST_BAND_INDEX);
        preEqBand.setGain(TEST_GAIN1);
        config.setPreEqBandAllChannelsTo(TEST_BAND_INDEX, preEqBand);

        MbcBand mbcBand = config.getMbcBandByChannelIndex(TEST_CHANNEL_INDEX, TEST_BAND_INDEX);
        mbcBand.setPreGain(TEST_GAIN1);
        config.setMbcBandAllChannelsTo(TEST_BAND_INDEX, mbcBand);

        EqBand postEqBand = config.getPostEqBandByChannelIndex(TEST_CHANNEL_INDEX, TEST_BAND_INDEX);
        postEqBand.setGain(TEST_GAIN1);
        config.setPostEqBandAllChannelsTo(TEST_BAND_INDEX, postEqBand);

        for (int i = 0; i < TEST_CHANNEL_COUNT; i++) {

            EqBand preEqBandTest = new EqBand(config.getPreEqBandByChannelIndex(i,
                    TEST_BAND_INDEX));
            assertEquals("preEQBand gain is different in channel " + i + " band " + TEST_BAND_INDEX,
                    TEST_GAIN1, preEqBandTest.getGain(), EPSILON);

            MbcBand mbcBandTest = new MbcBand(config.getMbcBandByChannelIndex(i, TEST_BAND_INDEX));
            assertEquals("mbcBand preGain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN1, mbcBandTest.getPreGain(), EPSILON);

            EqBand postEqBandTest = new EqBand(config.getPostEqBandByChannelIndex(i,
                    TEST_BAND_INDEX));
            assertEquals("postEQBand gain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN1, postEqBandTest.getGain(), EPSILON);

            // change per Band
            preEqBandTest.setGain(TEST_GAIN2);
            config.setPreEqBandByChannelIndex(i, TEST_BAND_INDEX, preEqBandTest);
            assertEquals("preEQBand gain is different in channel " + i + " band " + TEST_BAND_INDEX,
                    TEST_GAIN2, config.getPreEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                    EPSILON);

            mbcBandTest.setPreGain(TEST_GAIN2);
            config.setMbcBandByChannelIndex(i, TEST_BAND_INDEX, mbcBandTest);
            assertEquals("mbcBand preGain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN2,
                    config.getMbcBandByChannelIndex(i, TEST_BAND_INDEX).getPreGain(), EPSILON);

            postEqBandTest.setGain(TEST_GAIN2);
            config.setPostEqBandByChannelIndex(i, TEST_BAND_INDEX, postEqBandTest);
            assertEquals("postEQBand gain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN2,
                    config.getPostEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(), EPSILON);
        }
    }

    public void test2_4Channel_perStage() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        DynamicsProcessing.Config config = getBuilderWithValues(MIN_CHANNEL_COUNT).build();

        Channel channel = new Channel(config.getChannelByChannelIndex(TEST_CHANNEL_INDEX));

        channel.setInputGain(TEST_GAIN1);
        assertEquals("channel gain is different", TEST_GAIN1, channel.getInputGain(), EPSILON);

        // set by stage
        Eq preEq = new Eq(channel.getPreEq());
        EqBand preEqBand = new EqBand(preEq.getBand(TEST_BAND_INDEX));
        preEqBand.setGain(TEST_GAIN1);
        preEq.setBand(TEST_BAND_INDEX, preEqBand);
        channel.setPreEq(preEq);
        assertEquals("preEQBand gain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN1, channel.getPreEq().getBand(TEST_BAND_INDEX).getGain(), EPSILON);
        preEqBand.setGain(TEST_GAIN2);
        preEq.setBand(TEST_BAND_INDEX, preEqBand);
        channel.setPreEq(preEq);
        assertEquals("preEQBand gain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN2, channel.getPreEq().getBand(TEST_BAND_INDEX).getGain(), EPSILON);

        Mbc mbc = new Mbc(channel.getMbc());
        MbcBand mbcBand = new MbcBand(mbc.getBand(TEST_BAND_INDEX));
        mbcBand.setPreGain(TEST_GAIN1);
        mbc.setBand(TEST_BAND_INDEX, mbcBand);
        channel.setMbc(mbc);
        assertEquals("mbcBand preGain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN1, channel.getMbc().getBand(TEST_BAND_INDEX).getPreGain(), EPSILON);
        mbcBand.setPreGain(TEST_GAIN2);
        mbc.setBand(TEST_BAND_INDEX, mbcBand);
        channel.setMbc(mbc);
        assertEquals("mbcBand preGain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN2, channel.getMbc().getBand(TEST_BAND_INDEX).getPreGain(), EPSILON);

        Eq postEq = new Eq(channel.getPostEq());
        EqBand postEqBand = new EqBand(postEq.getBand(TEST_BAND_INDEX));
        postEqBand.setGain(TEST_GAIN1);
        postEq.setBand(TEST_BAND_INDEX, postEqBand);
        channel.setPostEq(postEq);
        assertEquals("postEqBand gain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN1, channel.getPostEq().getBand(TEST_BAND_INDEX).getGain(), EPSILON);
        postEqBand.setGain(TEST_GAIN2);
        postEq.setBand(TEST_BAND_INDEX, postEqBand);
        channel.setPostEq(postEq);
        assertEquals("postEQBand gain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN2, channel.getPostEq().getBand(TEST_BAND_INDEX).getGain(), EPSILON);

        Limiter limiter = new Limiter(channel.getLimiter());
        limiter.setPostGain(TEST_GAIN1);
        channel.setLimiter(limiter);
        assertEquals("limiter gain is different",
                TEST_GAIN1, channel.getLimiter().getPostGain(), EPSILON);
        limiter.setPostGain(TEST_GAIN2);
        channel.setLimiter(limiter);
        assertEquals("limiter gain is different",
                TEST_GAIN2, channel.getLimiter().getPostGain(), EPSILON);

    }

    public void test2_5Channel_perBand() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        DynamicsProcessing.Config config = getBuilderWithValues(MIN_CHANNEL_COUNT).build();

        Channel channel = new Channel(config.getChannelByChannelIndex(TEST_CHANNEL_INDEX));

        channel.setInputGain(TEST_GAIN1);
        assertEquals("channel gain is different", TEST_GAIN1, channel.getInputGain(), EPSILON);

        // set by band
        EqBand preEqBand = new EqBand(channel.getPreEqBand(TEST_BAND_INDEX));
        preEqBand.setGain(TEST_GAIN1);
        channel.setPreEqBand(TEST_BAND_INDEX, preEqBand);
        assertEquals("preEQBand gain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN1, channel.getPreEqBand(TEST_BAND_INDEX).getGain(), EPSILON);
        preEqBand.setGain(TEST_GAIN2);
        channel.setPreEqBand(TEST_BAND_INDEX, preEqBand);
        assertEquals("preEQBand gain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN2, channel.getPreEqBand(TEST_BAND_INDEX).getGain(), EPSILON);

        MbcBand mbcBand = new MbcBand(channel.getMbcBand(TEST_BAND_INDEX));
        mbcBand.setPreGain(TEST_GAIN1);
        channel.setMbcBand(TEST_BAND_INDEX, mbcBand);
        assertEquals("mbcBand preGain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN1, channel.getMbcBand(TEST_BAND_INDEX).getPreGain(), EPSILON);
        mbcBand.setPreGain(TEST_GAIN2);
        channel.setMbcBand(TEST_BAND_INDEX, mbcBand);
        assertEquals("mbcBand preGain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN2, channel.getMbcBand(TEST_BAND_INDEX).getPreGain(), EPSILON);

        EqBand postEqBand = new EqBand(channel.getPostEqBand(TEST_BAND_INDEX));
        postEqBand.setGain(TEST_GAIN1);
        channel.setPostEqBand(TEST_BAND_INDEX, postEqBand);
        assertEquals("postEqBand gain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN1, channel.getPostEqBand(TEST_BAND_INDEX).getGain(), EPSILON);
        postEqBand.setGain(TEST_GAIN2);
        channel.setPostEqBand(TEST_BAND_INDEX, postEqBand);
        assertEquals("postEqBand gain is different in band " + TEST_BAND_INDEX,
                TEST_GAIN2, channel.getPostEqBand(TEST_BAND_INDEX).getGain(), EPSILON);
    }

    public void test2_6Eq() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        final boolean inUse = true;
        final boolean enabled = true;
        final int bandCount = 3;

        Eq eq = new Eq(inUse, enabled, bandCount);
        assertEquals("eq inUse is different", inUse, eq.isInUse());
        assertEquals("eq enabled is different", enabled, eq.isEnabled());
        assertEquals("eq bandCount is different", bandCount, eq.getBandCount());

        // changes
        eq.setEnabled(!enabled);
        assertEquals("eq enabled is different", !enabled, eq.isEnabled());

        // bands
        for (int i = 0; i < bandCount; i++) {
            final float frequency = (i + 1) * 100.3f;
            final float gain = (i + 1) * 10.1f;
            EqBand eqBand = new EqBand(eq.getBand(i));

            eqBand.setEnabled(enabled);
            eqBand.setCutoffFrequency(frequency);
            eqBand.setGain(gain);

            eq.setBand(i, eqBand);

            // compare
            assertEquals("eq enabled is different in band " + i, enabled,
                    eq.getBand(i).isEnabled());
            assertEquals("eq cutoffFrequency is different in band " + i,
                    frequency, eq.getBand(i).getCutoffFrequency(), EPSILON);
            assertEquals("eq eqBand gain is different in band " + i,
                    gain, eq.getBand(i).getGain(), EPSILON);

            // EqBand constructor
            EqBand eqBand2 = new EqBand(enabled, frequency, gain);

            assertEquals("eq enabled is different in band " + i, enabled,
                    eqBand2.isEnabled());
            assertEquals("eq cutoffFrequency is different in band " + i,
                    frequency, eqBand2.getCutoffFrequency(), EPSILON);
            assertEquals("eq eqBand gain is different in band " + i,
                    gain, eqBand2.getGain(), EPSILON);
        }
    }

    public void test2_7Mbc() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        final boolean inUse = true;
        final boolean enabled = true;
        final int bandCount = 3;

        Mbc mbc = new Mbc(inUse, enabled, bandCount);
        assertEquals("mbc inUse is different", inUse, mbc.isInUse());
        assertEquals("mbc enabled is different", enabled, mbc.isEnabled());
        assertEquals("mbc bandCount is different", bandCount, mbc.getBandCount());

        // changes
        mbc.setEnabled(!enabled);
        assertEquals("enabled is different", !enabled, mbc.isEnabled());

        // bands
        for (int i = 0; i < bandCount; i++) {
            int index = i + 1;
            final float frequency = index * 100.3f;
            final float attackTime = index * 3.2f;
            final float releaseTime = 2 * attackTime;
            final float ratio = index * 1.2f;
            final float threshold = index * (-12.8f);
            final float kneeWidth = index * 0.3f;
            final float noiseGateThreshold = index * (-20.1f);
            final float expanderRatio = index * 1.1f;
            final float preGain = index * 10.1f;
            final float postGain = index * (-0.2f);
            MbcBand mbcBand = new MbcBand(mbc.getBand(i));

            mbcBand.setEnabled(enabled);
            mbcBand.setCutoffFrequency(frequency);
            mbcBand.setAttackTime(attackTime);
            mbcBand.setReleaseTime(releaseTime);
            mbcBand.setRatio(ratio);
            mbcBand.setThreshold(threshold);
            mbcBand.setKneeWidth(kneeWidth);
            mbcBand.setNoiseGateThreshold(noiseGateThreshold);
            mbcBand.setExpanderRatio(expanderRatio);
            mbcBand.setPreGain(preGain);
            mbcBand.setPostGain(postGain);

            mbc.setBand(i, mbcBand);

            // compare
            assertEquals("mbc enabled is different", enabled, mbc.getBand(i).isEnabled());
            assertEquals("mbc cutoffFrequency is different in band " + i,
                    frequency, mbc.getBand(i).getCutoffFrequency(), EPSILON);
            assertEquals("mbc attackTime is different in band " + i,
                    attackTime, mbc.getBand(i).getAttackTime(), EPSILON);
            assertEquals("mbc releaseTime is different in band " + i,
                    releaseTime, mbc.getBand(i).getReleaseTime(), EPSILON);
            assertEquals("mbc ratio is different in band " + i,
                    ratio, mbc.getBand(i).getRatio(), EPSILON);
            assertEquals("mbc threshold is different in band " + i,
                    threshold, mbc.getBand(i).getThreshold(), EPSILON);
            assertEquals("mbc kneeWidth is different in band " + i,
                    kneeWidth, mbc.getBand(i).getKneeWidth(), EPSILON);
            assertEquals("mbc noiseGateThreshold is different in band " + i,
                    noiseGateThreshold, mbc.getBand(i).getNoiseGateThreshold(), EPSILON);
            assertEquals("mbc expanderRatio is different in band " + i,
                    expanderRatio, mbc.getBand(i).getExpanderRatio(), EPSILON);
            assertEquals("mbc preGain is different in band " + i,
                    preGain, mbc.getBand(i).getPreGain(), EPSILON);
            assertEquals("mbc postGain is different in band " + i,
                    postGain, mbc.getBand(i).getPostGain(), EPSILON);

            // MbcBand constructor
            MbcBand mbcBand2 = new MbcBand(enabled, frequency, attackTime, releaseTime, ratio,
                    threshold, kneeWidth, noiseGateThreshold, expanderRatio, preGain, postGain);

            assertEquals("mbc enabled is different", enabled, mbcBand2.isEnabled());
            assertEquals("mbc cutoffFrequency is different in band " + i,
                    frequency, mbcBand2.getCutoffFrequency(), EPSILON);
            assertEquals("mbc attackTime is different in band " + i,
                    attackTime, mbcBand2.getAttackTime(), EPSILON);
            assertEquals("mbc releaseTime is different in band " + i,
                    releaseTime, mbcBand2.getReleaseTime(), EPSILON);
            assertEquals("mbc ratio is different in band " + i,
                    ratio, mbcBand2.getRatio(), EPSILON);
            assertEquals("mbc threshold is different in band " + i,
                    threshold, mbcBand2.getThreshold(), EPSILON);
            assertEquals("mbc kneeWidth is different in band " + i,
                    kneeWidth, mbcBand2.getKneeWidth(), EPSILON);
            assertEquals("mbc noiseGateThreshold is different in band " + i,
                    noiseGateThreshold, mbcBand2.getNoiseGateThreshold(), EPSILON);
            assertEquals("mbc expanderRatio is different in band " + i,
                    expanderRatio, mbcBand2.getExpanderRatio(), EPSILON);
            assertEquals("mbc preGain is different in band " + i,
                    preGain, mbcBand2.getPreGain(), EPSILON);
            assertEquals("mbc postGain is different in band " + i,
                    postGain, mbcBand2.getPostGain(), EPSILON);
        }
    }

    public void test2_8Limiter() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        final boolean inUse = true;
        final boolean enabled = true;
        final int linkGroup = 4;
        final float attackTime = 3.2f;
        final float releaseTime = 2 * attackTime;
        final float ratio = 1.2f;
        final float threshold = (-12.8f);
        final float postGain = (-0.2f);

        Limiter limiter = new Limiter(inUse, enabled, linkGroup, attackTime, releaseTime, ratio,
                threshold, postGain);
        assertEquals("limiter inUse is different", inUse, limiter.isInUse());
        assertEquals("limiter enabled is different", enabled, limiter.isEnabled());
        assertEquals("limiter linkGroup is different", linkGroup, limiter.getLinkGroup());

        // defaults
        assertEquals("limiter attackTime is different",
                attackTime, limiter.getAttackTime(), EPSILON);
        assertEquals("limiter releaseTime is different",
                releaseTime, limiter.getReleaseTime(), EPSILON);
        assertEquals("limiter ratio is different",
                ratio, limiter.getRatio(), EPSILON);
        assertEquals("limiter threshold is different",
                threshold, limiter.getThreshold(), EPSILON);
        assertEquals("limiter postGain is different",
                postGain, limiter.getPostGain(), EPSILON);

        // changes
        final boolean newEnabled = !enabled;
        final int newLinkGroup = 7;
        final float newAttackTime = attackTime + 10;
        final float newReleaseTime = releaseTime + 10;
        final float newRatio = ratio + 2f;
        final float newThreshold = threshold - 20f;
        final float newPostGain = postGain + 3f;

        limiter.setEnabled(newEnabled);
        limiter.setLinkGroup(newLinkGroup);
        limiter.setAttackTime(newAttackTime);
        limiter.setReleaseTime(newReleaseTime);
        limiter.setRatio(newRatio);
        limiter.setThreshold(newThreshold);
        limiter.setPostGain(newPostGain);

        assertEquals("limiter enabled is different", newEnabled, limiter.isEnabled());
        assertEquals("limiter linkGroup is different", newLinkGroup, limiter.getLinkGroup());
        assertEquals("limiter attackTime is different",
                newAttackTime, limiter.getAttackTime(), EPSILON);
        assertEquals("limiter releaseTime is different",
                newReleaseTime, limiter.getReleaseTime(), EPSILON);
        assertEquals("limiter ratio is different",
                newRatio, limiter.getRatio(), EPSILON);
        assertEquals("limiter threshold is different",
                newThreshold, limiter.getThreshold(), EPSILON);
        assertEquals("limiter postGain is different",
                newPostGain, limiter.getPostGain(), EPSILON);
    }

    public void test2_9BandStage() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        final boolean inUse = true;
        final boolean enabled = true;
        final int bandCount = 3;

        BandStage bandStage = new BandStage(inUse, enabled, bandCount);
        assertEquals("bandStage inUse is different", inUse, bandStage.isInUse());
        assertEquals("bandStage enabled is different", enabled, bandStage.isEnabled());
        assertEquals("bandStage bandCount is different", bandCount, bandStage.getBandCount());

        // change
        bandStage.setEnabled(!enabled);
        assertEquals("bandStage enabled is different", !enabled, bandStage.isEnabled());
    }

    public void test2_10Stage() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        final boolean inUse = true;
        final boolean enabled = true;
        final int bandCount = 3;

        DynamicsProcessing.Stage stage = new DynamicsProcessing.Stage(inUse, enabled);
        assertEquals("stage inUse is different", inUse, stage.isInUse());
        assertEquals("stage enabled is different", enabled, stage.isEnabled());

        // change
        stage.setEnabled(!enabled);
        assertEquals("stage enabled is different", !enabled, stage.isEnabled());
    }

    public void test2_11BandBase() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        final boolean enabled = true;
        final float frequency = 100.3f;

        BandBase bandBase = new BandBase(enabled, frequency);

        assertEquals("bandBase enabled is different", enabled, bandBase.isEnabled());
        assertEquals("bandBase cutoffFrequency is different",
                frequency, bandBase.getCutoffFrequency(), EPSILON);

        // change
        final float newFrequency = frequency + 10f;
        bandBase.setEnabled(!enabled);
        bandBase.setCutoffFrequency(newFrequency);
        assertEquals("bandBase enabled is different", !enabled, bandBase.isEnabled());
        assertEquals("bandBase cutoffFrequency is different",
                newFrequency, bandBase.getCutoffFrequency(), EPSILON);
    }

    public void test2_12Channel() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        final float inputGain = 3.4f;
        final boolean preEqInUse = true;
        final int preEqBandCount = 3;
        final boolean mbcInUse = true;
        final int mbcBandCount = 4;
        final boolean postEqInUse = true;
        final int postEqBandCount = 5;
        final boolean limiterInUse = true;

        Channel channel = new Channel(inputGain, preEqInUse, preEqBandCount, mbcInUse,
                mbcBandCount, postEqInUse, postEqBandCount, limiterInUse);

        assertEquals("channel inputGain is different", inputGain,
                channel.getInputGain(), EPSILON);
        assertEquals("channel preEqInUse is different", preEqInUse, channel.getPreEq().isInUse());
        assertEquals("channel preEqBandCount is different", preEqBandCount,
                channel.getPreEq().getBandCount());
        assertEquals("channel mbcInUse is different", mbcInUse, channel.getMbc().isInUse());
        assertEquals("channel mbcBandCount is different", mbcBandCount,
                channel.getMbc().getBandCount());
        assertEquals("channel postEqInUse is different", postEqInUse,
                channel.getPostEq().isInUse());
        assertEquals("channel postEqBandCount is different", postEqBandCount,
                channel.getPostEq().getBandCount());
        assertEquals("channel limiterInUse is different", limiterInUse,
                channel.getLimiter().isInUse());
    }
    // -----------------------------------------------------------------
    // 3 - Builder
    // ----------------------------------

    public void test3_0Builder_stagesAllChannels() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        DynamicsProcessing.Config config = getBuilderWithValues(MIN_CHANNEL_COUNT).build();
        DynamicsProcessing.Config.Builder builder = getBuilderWithValues(TEST_CHANNEL_COUNT);

        // get Stages, apply all channels
        Eq preEq = new Eq(config.getPreEqByChannelIndex(TEST_CHANNEL_INDEX));
        EqBand preEqBand = new EqBand(preEq.getBand(TEST_BAND_INDEX));
        preEqBand.setGain(TEST_GAIN1);
        preEq.setBand(TEST_BAND_INDEX, preEqBand);
        builder.setPreEqAllChannelsTo(preEq);

        Mbc mbc = new Mbc(config.getMbcByChannelIndex(TEST_CHANNEL_INDEX));
        MbcBand mbcBand = new MbcBand(mbc.getBand(TEST_BAND_INDEX));
        mbcBand.setPreGain(TEST_GAIN1);
        mbc.setBand(TEST_BAND_INDEX, mbcBand);
        builder.setMbcAllChannelsTo(mbc);

        Eq postEq = new Eq(config.getPostEqByChannelIndex(TEST_CHANNEL_INDEX));
        EqBand postEqBand = new EqBand(postEq.getBand(TEST_BAND_INDEX));
        postEqBand.setGain(TEST_GAIN1);
        postEq.setBand(TEST_BAND_INDEX, postEqBand);
        builder.setPostEqAllChannelsTo(postEq);

        Limiter limiter = new Limiter(config.getLimiterByChannelIndex(TEST_CHANNEL_INDEX));
        limiter.setPostGain(TEST_GAIN1);
        builder.setLimiterAllChannelsTo(limiter);

        // build and compare
        DynamicsProcessing.Config newConfig = builder.build();
        for (int i = 0; i < TEST_CHANNEL_COUNT; i++) {
            assertEquals("preEQBand gain is different in channel " + i + " band " + TEST_BAND_INDEX,
                    TEST_GAIN1, newConfig.getPreEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                    EPSILON);

            assertEquals("mbcBand preGain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN1,
                    newConfig.getMbcBandByChannelIndex(i, TEST_BAND_INDEX).getPreGain(),
                    EPSILON);

            assertEquals("postEQBand gain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN1,
                    newConfig.getPostEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                    EPSILON);

            assertEquals("limiter gain is different in channel " + i,
                    TEST_GAIN1, newConfig.getLimiterByChannelIndex(i).getPostGain(), EPSILON);
        }
    }

    public void test3_1Builder_stagesByChannelIndex() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        DynamicsProcessing.Config config = getBuilderWithValues(MIN_CHANNEL_COUNT).build();
        DynamicsProcessing.Config.Builder builder = getBuilderWithValues(TEST_CHANNEL_COUNT);

        Eq preEq = new Eq(config.getPreEqByChannelIndex(TEST_CHANNEL_INDEX));
        EqBand preEqBand = new EqBand(preEq.getBand(TEST_BAND_INDEX));

        Mbc mbc = new Mbc(config.getMbcByChannelIndex(TEST_CHANNEL_INDEX));
        MbcBand mbcBand = new MbcBand(mbc.getBand(TEST_BAND_INDEX));

        Eq postEq = new Eq(config.getPostEqByChannelIndex(TEST_CHANNEL_INDEX));
        EqBand postEqBand = new EqBand(postEq.getBand(TEST_BAND_INDEX));

        Limiter limiter = new Limiter(config.getLimiterByChannelIndex(TEST_CHANNEL_INDEX));

        // get Stages, apply per channel
        for (int i = 0; i < TEST_CHANNEL_COUNT; i++) {
            float gain = i % 2 == 0 ? TEST_GAIN1 : TEST_GAIN2;

            builder.setInputGainByChannelIndex(i, gain);

            preEqBand.setGain(gain);
            preEq.setBand(TEST_BAND_INDEX, preEqBand);
            builder.setPreEqByChannelIndex(i, preEq);

            mbcBand.setPreGain(gain);
            mbc.setBand(TEST_BAND_INDEX, mbcBand);
            builder.setMbcByChannelIndex(i, mbc);

            postEqBand.setGain(gain);
            postEq.setBand(TEST_BAND_INDEX, postEqBand);
            builder.setPostEqByChannelIndex(i, postEq);

            limiter.setPostGain(gain);
            builder.setLimiterByChannelIndex(i, limiter);
        }
        // build and compare
        DynamicsProcessing.Config newConfig = builder.build();
        for (int i = 0; i < TEST_CHANNEL_COUNT; i++) {
            float expectedGain = i % 2 == 0 ? TEST_GAIN1 : TEST_GAIN2;

            assertEquals("inputGain is different in channel " + i,
                    expectedGain,
                    newConfig.getInputGainByChannelIndex(i),
                    EPSILON);

            assertEquals("preEQBand gain is different in channel " + i + " band " + TEST_BAND_INDEX,
                    expectedGain,
                    newConfig.getPreEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                    EPSILON);

            assertEquals("mbcBand preGain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, expectedGain,
                    newConfig.getMbcBandByChannelIndex(i, TEST_BAND_INDEX).getPreGain(),
                    EPSILON);

            assertEquals("postEQBand gain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, expectedGain,
                    newConfig.getPostEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                    EPSILON);

            assertEquals("limiter gain is different in channel " + i,
                    expectedGain, newConfig.getLimiterByChannelIndex(i).getPostGain(), EPSILON);
        }
    }

    public void test3_2Builder_setAllChannelsTo() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        DynamicsProcessing.Config config = getBuilderWithValues(MIN_CHANNEL_COUNT).build();
        DynamicsProcessing.Config.Builder builder = getBuilderWithValues(TEST_CHANNEL_COUNT);

        Channel channel = new Channel(config.getChannelByChannelIndex(TEST_CHANNEL_INDEX));

        // get Stages, apply all channels
        Eq preEq = new Eq(config.getPreEqByChannelIndex(TEST_CHANNEL_INDEX));
        EqBand preEqBand = new EqBand(preEq.getBand(TEST_BAND_INDEX));
        preEqBand.setGain(TEST_GAIN1);
        preEq.setBand(TEST_BAND_INDEX, preEqBand);
        channel.setPreEq(preEq);

        Mbc mbc = new Mbc(config.getMbcByChannelIndex(TEST_CHANNEL_INDEX));
        MbcBand mbcBand = new MbcBand(mbc.getBand(TEST_BAND_INDEX));
        mbcBand.setPreGain(TEST_GAIN1);
        mbc.setBand(TEST_BAND_INDEX, mbcBand);
        channel.setMbc(mbc);

        Eq postEq = new Eq(config.getPostEqByChannelIndex(TEST_CHANNEL_INDEX));
        EqBand postEqBand = new EqBand(postEq.getBand(TEST_BAND_INDEX));
        postEqBand.setGain(TEST_GAIN1);
        postEq.setBand(TEST_BAND_INDEX, postEqBand);
        channel.setPostEq(postEq);

        Limiter limiter = new Limiter(config.getLimiterByChannelIndex(TEST_CHANNEL_INDEX));
        limiter.setPostGain(TEST_GAIN1);
        channel.setLimiter(limiter);

        builder.setAllChannelsTo(channel);
        // build and compare
        DynamicsProcessing.Config newConfig = builder.build();
        for (int i = 0; i < TEST_CHANNEL_COUNT; i++) {
            assertEquals("preEQBand gain is different in channel " + i + " band " + TEST_BAND_INDEX,
                    TEST_GAIN1, newConfig.getPreEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                    EPSILON);

            assertEquals("mbcBand preGain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN1,
                    newConfig.getMbcBandByChannelIndex(i, TEST_BAND_INDEX).getPreGain(),
                    EPSILON);

            assertEquals("postEQBand gain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, TEST_GAIN1,
                    newConfig.getPostEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                    EPSILON);

            assertEquals("limiter gain is different in channel " + i,
                    TEST_GAIN1, newConfig.getLimiterByChannelIndex(i).getPostGain(), EPSILON);
        }
    }

    public void test3_3Builder_setChannelTo() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }

        DynamicsProcessing.Config config = getBuilderWithValues(MIN_CHANNEL_COUNT).build();
        DynamicsProcessing.Config.Builder builder = getBuilderWithValues(TEST_CHANNEL_COUNT);

        Channel channel = new Channel(config.getChannelByChannelIndex(TEST_CHANNEL_INDEX));

        Eq preEq = new Eq(config.getPreEqByChannelIndex(TEST_CHANNEL_INDEX));
        EqBand preEqBand = new EqBand(preEq.getBand(TEST_BAND_INDEX));

        Mbc mbc = new Mbc(config.getMbcByChannelIndex(TEST_CHANNEL_INDEX));
        MbcBand mbcBand = new MbcBand(mbc.getBand(TEST_BAND_INDEX));

        Eq postEq = new Eq(config.getPostEqByChannelIndex(TEST_CHANNEL_INDEX));
        EqBand postEqBand = new EqBand(postEq.getBand(TEST_BAND_INDEX));

        Limiter limiter = new Limiter(config.getLimiterByChannelIndex(TEST_CHANNEL_INDEX));

        // get Stages, apply per channel
        for (int i = 0; i < TEST_CHANNEL_COUNT; i++) {
            float gain = i % 2 == 0 ? TEST_GAIN1 : TEST_GAIN2;

            preEqBand.setGain(gain);
            preEq.setBand(TEST_BAND_INDEX, preEqBand);
            channel.setPreEq(preEq);

            mbcBand.setPreGain(gain);
            mbc.setBand(TEST_BAND_INDEX, mbcBand);
            channel.setMbc(mbc);

            postEqBand.setGain(gain);
            postEq.setBand(TEST_BAND_INDEX, postEqBand);
            channel.setPostEq(postEq);

            limiter.setPostGain(gain);
            channel.setLimiter(limiter);

            builder.setChannelTo(i, channel);
        }
        // build and compare
        DynamicsProcessing.Config newConfig = builder.build();
        for (int i = 0; i < TEST_CHANNEL_COUNT; i++) {
            float expectedGain = i % 2 == 0 ? TEST_GAIN1 : TEST_GAIN2;
            assertEquals("preEQBand gain is different in channel " + i + " band " + TEST_BAND_INDEX,
                    expectedGain,
                    newConfig.getPreEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                    EPSILON);

            assertEquals("mbcBand preGain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, expectedGain,
                    newConfig.getMbcBandByChannelIndex(i, TEST_BAND_INDEX).getPreGain(),
                    EPSILON);

            assertEquals("postEQBand gain is different in channel " + i + " band " +
                    TEST_BAND_INDEX, expectedGain,
                    newConfig.getPostEqBandByChannelIndex(i, TEST_BAND_INDEX).getGain(),
                    EPSILON);

            assertEquals("limiter gain is different in channel " + i,
                    expectedGain, newConfig.getLimiterByChannelIndex(i).getPostGain(), EPSILON);
        }
    }
    // -----------------------------------------------------------------
    // private methods
    // ----------------------------------

    private void createDynamicsProcessing(int session) {
        createDynamicsProcessingWithConfig(session, null);
    }

    private void createDynamicsProcessingWithConfig(int session, DynamicsProcessing.Config config) {
        releaseDynamicsProcessing();
        try {
            mDP = (config == null ? new DynamicsProcessing(session)
                    : new DynamicsProcessing(0 /* priority */, session, config));
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "createDynamicsProcessingWithConfig() DynamicsProcessing not found"
                    + "exception: ", e);
        } catch (UnsupportedOperationException e) {
            Log.e(TAG, "createDynamicsProcessingWithConfig() Effect library not loaded exception: ",
                    e);
        }
        assertNotNull("could not create DynamicsProcessing", mDP);
    }

    private void releaseDynamicsProcessing() {
        if (mDP != null) {
            mDP.release();
            mDP = null;
        }
    }

    private void createDefaultEffect() {
        DynamicsProcessing.Config config = getBuilderWithValues().build();
        assertNotNull("null config", config);

        AudioManager am = (AudioManager) getContext().getSystemService(Context.AUDIO_SERVICE);
        assertNotNull("null AudioManager", am);

        int session = am.generateAudioSessionId();
        assertTrue("cannot generate new session", session != AudioManager.ERROR);

        createDynamicsProcessingWithConfig(session, config);
    }

    private DynamicsProcessing.Config.Builder getBuilder(int channelCount) {
        // simple config
        DynamicsProcessing.Config.Builder builder = new DynamicsProcessing.Config.Builder(
                DEFAULT_VARIANT /* variant */,
                channelCount/* channels */,
                DEFAULT_PREEQ_IN_USE /* enable preEQ */,
                DEFAULT_PREEQ_BAND_COUNT /* preEq bands */,
                DEFAULT_MBC_IN_USE /* enable mbc */,
                DEFAULT_MBC_BAND_COUNT /* mbc bands */,
                DEFAULT_POSTEQ_IN_USE /* enable postEq */,
                DEFAULT_POSTEQ_BAND_COUNT /* postEq bands */,
                DEFAULT_LIMITER_IN_USE /* enable limiter */);

        return builder;
    }

    private DynamicsProcessing.Config.Builder getBuilderWithValues(int channelCount) {
        // simple config
        DynamicsProcessing.Config.Builder builder = getBuilder(channelCount);

        // Set Defaults
        builder.setPreferredFrameDuration(DEFAULT_FRAME_DURATION);
        builder.setInputGainAllChannelsTo(DEFAULT_INPUT_GAIN);
        return builder;
    }

    private DynamicsProcessing.Config.Builder getBuilderWithValues() {
        return getBuilderWithValues(MIN_CHANNEL_COUNT);
    }

}
