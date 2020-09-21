/*
 * Copyright (C) 2008 The Android Open Source Project
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
import android.content.res.AssetFileDescriptor;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.SoundPool;
import android.platform.test.annotations.AppModeFull;
import android.test.AndroidTestCase;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicInteger;

@AppModeFull(reason = "TODO: evaluate and port to instant")
abstract class SoundPoolTest extends AndroidTestCase {

    private static final int SOUNDPOOL_STREAMS = 4;
    private static final int PRIORITY = 1;
    private static final int LOUD = 20;
    private static final int QUIET = LOUD / 2;
    private static final int SILENT = 0;
    private File mFile;
    private SoundPool mSoundPool;

    /**
     * function to return resource ID for A4 sound.
     * should be implemented by child class
     * @return resource ID
     */
    protected abstract int getSoundA();

    protected abstract int getSoundCs();

    protected abstract int getSoundE();

    protected abstract int getSoundB();

    protected abstract int getSoundGs();

    protected abstract String getFileName();

    private int[] getSounds() {
        int[] sounds = { getSoundA(),
                         getSoundCs(),
                         getSoundE(),
                         getSoundB(),
                         getSoundGs() };
        return sounds;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mFile = new File(mContext.getFilesDir(), getFileName());
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        if (mFile.exists()) {
            mFile.delete();
        }
        if (mSoundPool != null) {
            mSoundPool.release();
            mSoundPool = null;
            return;
        }
    }

    public void testLoad() throws Exception {
        int srcQuality = 100;
        mSoundPool = new SoundPool(SOUNDPOOL_STREAMS, AudioManager.STREAM_MUSIC, srcQuality);
        int sampleId1 = mSoundPool.load(mContext, getSoundA(), PRIORITY);
        waitUntilLoaded(sampleId1);
        // should return true, but returns false
        mSoundPool.unload(sampleId1);

        AssetFileDescriptor afd = mContext.getResources().openRawResourceFd(getSoundCs());
        int sampleId2;
        sampleId2 = mSoundPool.load(afd, PRIORITY);
        waitUntilLoaded(sampleId2);
        mSoundPool.unload(sampleId2);

        FileDescriptor fd = afd.getFileDescriptor();
        long offset = afd.getStartOffset();
        long length = afd.getLength();
        int sampleId3;
        sampleId3 = mSoundPool.load(fd, offset, length, PRIORITY);
        waitUntilLoaded(sampleId3);
        mSoundPool.unload(sampleId3);

        String path = mFile.getAbsolutePath();
        createSoundFile(mFile);
        int sampleId4;
        sampleId4 = mSoundPool.load(path, PRIORITY);
        waitUntilLoaded(sampleId4);
        mSoundPool.unload(sampleId4);
    }

    private void createSoundFile(File f) throws Exception {
        FileOutputStream fOutput = null;
        try {
            fOutput = new FileOutputStream(f);
            InputStream is = mContext.getResources().openRawResource(getSoundA());
            byte[] buffer = new byte[1024];
            int length = is.read(buffer);
            while (length != -1) {
                fOutput.write(buffer, 0, length);
                length = is.read(buffer);
            }
        } finally {
            if (fOutput != null) {
                fOutput.flush();
                fOutput.close();
            }
        }
    }

    public void testSoundPoolOp() throws Exception {
        int srcQuality = 100;
        mSoundPool = new SoundPool(SOUNDPOOL_STREAMS, AudioManager.STREAM_MUSIC, srcQuality);
        int sampleID = loadSampleSync(getSoundA(), PRIORITY);

        int waitMsec = 1000;
        float leftVolume = SILENT;
        float rightVolume = LOUD;
        int priority = 1;
        int loop = 0;
        float rate = 1f;
        int streamID = mSoundPool.play(sampleID, leftVolume, rightVolume, priority, loop, rate);
        assertTrue(streamID != 0);
        Thread.sleep(waitMsec);
        rate = 1.4f;
        mSoundPool.setRate(streamID, rate);
        Thread.sleep(waitMsec);
        mSoundPool.setRate(streamID, 1f);
        Thread.sleep(waitMsec);
        mSoundPool.pause(streamID);
        Thread.sleep(waitMsec);
        mSoundPool.resume(streamID);
        Thread.sleep(waitMsec);
        mSoundPool.stop(streamID);

        streamID = mSoundPool.play(sampleID, leftVolume, rightVolume, priority, loop, rate);
        assertTrue(streamID != 0);
        loop = -1;// loop forever
        mSoundPool.setLoop(streamID, loop);
        Thread.sleep(waitMsec);
        leftVolume = SILENT;
        rightVolume = SILENT;
        mSoundPool.setVolume(streamID, leftVolume, rightVolume);
        Thread.sleep(waitMsec);
        rightVolume = LOUD;
        mSoundPool.setVolume(streamID, leftVolume, rightVolume);
        priority = 0;
        mSoundPool.setPriority(streamID, priority);
        Thread.sleep(waitMsec * 10);
        mSoundPool.stop(streamID);
        mSoundPool.unload(sampleID);
    }

    public void testMultiSound() throws Exception {
        int srcQuality = 100;
        mSoundPool = new SoundPool(SOUNDPOOL_STREAMS, AudioManager.STREAM_MUSIC, srcQuality);
        int sampleID1 = loadSampleSync(getSoundA(), PRIORITY);
        int sampleID2 = loadSampleSync(getSoundCs(), PRIORITY);
        long waitMsec = 1000;
        Thread.sleep(waitMsec);

        // play sounds one at a time
        int streamID1 = mSoundPool.play(sampleID1, LOUD, QUIET, PRIORITY, -1, 1);
        assertTrue(streamID1 != 0);
        Thread.sleep(waitMsec * 4);
        mSoundPool.stop(streamID1);
        int streamID2 = mSoundPool.play(sampleID2, QUIET, LOUD, PRIORITY, -1, 1);
        assertTrue(streamID2 != 0);
        Thread.sleep(waitMsec * 4);
        mSoundPool.stop(streamID2);

        // play both at once repeating the first, but not the second
        streamID1 = mSoundPool.play(sampleID1, LOUD, QUIET, PRIORITY, 1, 1);
        streamID2 = mSoundPool.play(sampleID2, QUIET, LOUD, PRIORITY, 0, 1);
        assertTrue(streamID1 != 0);
        assertTrue(streamID2 != 0);
        Thread.sleep(4000);
        // both streams should have stopped by themselves; no way to check

        mSoundPool.release();
        mSoundPool = null;
    }

    public void testLoadMore() throws Exception {
        mSoundPool = new SoundPool(SOUNDPOOL_STREAMS, AudioManager.STREAM_MUSIC, 0);
        int[] sounds = getSounds();
        int[] soundIds = new int[sounds.length];
        int[] streamIds = new int[sounds.length];
        for (int i = 0; i < sounds.length; i++) {
            soundIds[i] = loadSampleSync(sounds[i], PRIORITY);
            System.out.println("load: " + soundIds[i]);
        }
        for (int i = 0; i < soundIds.length; i++) {
            streamIds[i] = mSoundPool.play(soundIds[i], LOUD, LOUD, PRIORITY, -1, 1);
        }
        Thread.sleep(3000);
        for (int stream : streamIds) {
            assertTrue(stream != 0);
            mSoundPool.stop(stream);
        }
        for (int sound : soundIds) {
            mSoundPool.unload(sound);
        }
        mSoundPool.release();
    }

    public void testAutoPauseResume() throws Exception {
        // The number of possible SoundPool streams simultaneously active is limited by
        // track resources. Generally this is no greater than 32, but the actual
        // amount may be less depending on concurrently running applications.
        // Here we attempt to create more streams than what is normally possible;
        // SoundPool should gracefully degrade to play those streams it can.
        //
        // Try to keep the maxStreams less than the number required to be active
        // and certainly less than 20 to be cooperative to other applications.
        final int TEST_STREAMS = 40;
        SoundPool soundPool = null;
        try {
            soundPool = new SoundPool.Builder()
                    .setAudioAttributes(new AudioAttributes.Builder().build())
                    .setMaxStreams(TEST_STREAMS)
                    .build();

            // get our sounds
            final int[] sounds = getSounds();

            // set our completion listener
            final int[] loadIds = new int[TEST_STREAMS];
            final Object done = new Object();
            final int[] loaded = new int[1]; // used as a "pointer" to an integer
            final SoundPool fpool = soundPool; // final reference in scope of try block
            soundPool.setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
                    @Override
                    public void onLoadComplete(SoundPool pool, int sampleId, int status) {
                        assertEquals(fpool, pool);
                        assertEquals(0 /* success */, status);
                        synchronized(done) {
                            loadIds[loaded[0]++] = sampleId;
                            if (loaded[0] == loadIds.length) {
                                done.notify();
                            }
                        }
                    }
                });

            // initiate loading
            final int[] soundIds = new int[TEST_STREAMS];
            for (int i = 0; i < soundIds.length; i++) {
                soundIds[i] = soundPool.load(mContext, sounds[i % sounds.length], PRIORITY);
            }

            // wait for all sounds to load,
            // it usually takes about 33 seconds and 50 seconds is used to have some headroom.
            final long LOAD_TIMEOUT_IN_MS = 50000;
            final long startTime = System.currentTimeMillis();
            synchronized(done) {
                while (loaded[0] != soundIds.length) {
                    final long waitTime =
                            LOAD_TIMEOUT_IN_MS - (System.currentTimeMillis() - startTime);
                    assertTrue(waitTime > 0);
                    done.wait(waitTime);
                }
            }

            // verify the Ids match (actually does sorting too)
            Arrays.sort(loadIds);
            Arrays.sort(soundIds);
            assertTrue(Arrays.equals(loadIds, soundIds));

            // play - should hear the following:
            // 1 second of sound
            // 1 second of silence
            // 1 second of sound.
            int[] streamIds = new int[soundIds.length];
            for (int i = 0; i < soundIds.length; i++) {
                streamIds[i] = soundPool.play(soundIds[i],
                        0.5f /* leftVolume */, 0.5f /* rightVolume */, PRIORITY,
                        -1 /* loop (infinite) */, 1.0f /* rate */);
            }
            Thread.sleep(1000 /* millis */);
            soundPool.autoPause();
            Thread.sleep(1000 /* millis */);
            soundPool.autoResume();
            Thread.sleep(1000 /* millis */);

            // clean up
            for (int stream : streamIds) {
                assertTrue(stream != 0);
                soundPool.stop(stream);
            }
            for (int sound : soundIds) {
                assertEquals(true, soundPool.unload(sound));
            }
            // check to see we're really unloaded
            for (int sound : soundIds) {
                assertEquals(false, soundPool.unload(sound));
            }
        } finally {
            if (soundPool != null) {
                soundPool.release();
                soundPool = null;
            }
        }
    }

    /**
     * Load a sample and wait until it is ready to be played.
     * @return The sample ID.
     * @throws InterruptedException
     */
    private int loadSampleSync(int sampleId, int prio) throws InterruptedException {
        int sample = mSoundPool.load(mContext, sampleId, prio);
        waitUntilLoaded(sample);
        return sample;
    }

    /**
     * Wait until the specified sample is loaded.
     * @param sampleId The sample ID.
     * @throws InterruptedException
     */
    private void waitUntilLoaded(int sampleId) throws InterruptedException {
        int stream = 0;
        while (stream == 0) {
            Thread.sleep(500);
            stream = mSoundPool.play(sampleId, SILENT, SILENT, 1, 0, 1);
        }
        mSoundPool.stop(stream);
    }
}
