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

import android.media.AudioPresentation;
import android.util.Log;

import com.android.compatibility.common.util.CtsAndroidTestCase;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class AudioPresentationTest extends CtsAndroidTestCase {
    private String TAG = "AudioPresentationTest";
    private static final String REPORT_LOG_NAME = "CtsMediaTestCases";

    public void testGetters() throws Exception {
        final int PRESENTATION_ID = 42;
        final int PROGRAM_ID = 43;
        final Map<Locale, String> LABELS = generateLabels();
        final Locale LOCALE = Locale.US;
        final int MASTERING_INDICATION = AudioPresentation.MASTERED_FOR_STEREO;
        final boolean HAS_AUDIO_DESCRIPTION = false;
        final boolean HAS_SPOKEN_SUBTITLES = true;
        final boolean HAS_DIALOGUE_ENHANCEMENT = true;

        AudioPresentation presentation = new AudioPresentation(
                PRESENTATION_ID,
                PROGRAM_ID,
                labelsLocaleToString(LABELS),
                LOCALE.toString(),
                MASTERING_INDICATION,
                HAS_AUDIO_DESCRIPTION,
                HAS_SPOKEN_SUBTITLES,
                HAS_DIALOGUE_ENHANCEMENT);
        assertEquals(PRESENTATION_ID, presentation.getPresentationId());
        assertEquals(PROGRAM_ID, presentation.getProgramId());
        assertEquals(LABELS.toString().toLowerCase(),
                presentation.getLabels().toString().toLowerCase());
        assertEquals(LOCALE.toString().toLowerCase(),
                presentation.getLocale().toString().toLowerCase());
        assertEquals(MASTERING_INDICATION, presentation.getMasteringIndication());
        assertEquals(HAS_AUDIO_DESCRIPTION, presentation.hasAudioDescription());
        assertEquals(HAS_SPOKEN_SUBTITLES, presentation.hasSpokenSubtitles());
        assertEquals(HAS_DIALOGUE_ENHANCEMENT, presentation.hasDialogueEnhancement());
    }

    private static Map<Locale, String> generateLabels() {
        Map<Locale, String> result = new HashMap<Locale, String>();
        result.put(Locale.US, Locale.US.getDisplayLanguage());
        result.put(Locale.FRENCH, Locale.FRENCH.getDisplayLanguage());
        result.put(Locale.GERMAN, Locale.GERMAN.getDisplayLanguage());
        return result;
    }

    private static Map<String, String> labelsLocaleToString(Map<Locale, String> localesMap) {
        Map<String, String> result = new HashMap<String, String>();
        for (Map.Entry<Locale, String> entry : localesMap.entrySet()) {
            result.put(entry.getKey().toString(), entry.getValue());
        }
        return result;
    }
}
