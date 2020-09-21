/*
 * Copyright (C) 2009 The Android Open Source Project
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

import android.media.AudioManager;
import android.media.ToneGenerator;
import android.platform.test.annotations.AppModeFull;
import android.test.AndroidTestCase;

@AppModeFull(reason = "TODO: evaluate and port to instant")
public class ToneGeneratorTest extends AndroidTestCase {

    public void testSyncGenerate() throws Exception {
        ToneGenerator toneGen = new ToneGenerator(AudioManager.STREAM_RING,
                                                  ToneGenerator.MAX_VOLUME);

        toneGen.startTone(ToneGenerator.TONE_PROP_BEEP2);
        final long DELAYED = 1000;
        Thread.sleep(DELAYED);
        toneGen.stopTone();
        toneGen.release();
    }

}
