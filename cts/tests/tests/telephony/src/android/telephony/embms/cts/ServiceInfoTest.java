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
 * limitations under the License
 */

package android.telephony.embms.cts;

import android.telephony.mbms.StreamingServiceInfo;
import android.test.InstrumentationTestCase;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class ServiceInfoTest extends InstrumentationTestCase {
    private static final String ID = "StreamingServiceId";
    private static final Map<Locale, String> LOCALE_DICT = new HashMap<Locale, String>() {{
        put(Locale.US, "Entertainment Source 1");
        put(Locale.CANADA, "Entertainment Source 1, eh?");
    }};
    private static final List<Locale> LOCALES = new ArrayList<Locale>() {{
        add(Locale.CANADA);
        add(Locale.US);
    }};
    private static final String NAME = "class1";
    private static final Date BEGIN_DATE = new Date(2017, 8, 21, 18, 20, 29);
    private static final Date END_DATE = new Date(2017, 8, 21, 18, 23, 9);
    private static final StreamingServiceInfo STREAMING_SERVICE_INFO =
        new StreamingServiceInfo(LOCALE_DICT, NAME, LOCALES, ID, BEGIN_DATE, END_DATE);

    public void testDataAccess() {
        assertEquals(LOCALES.size(), STREAMING_SERVICE_INFO.getLocales().size());
        for (int i = 0; i < LOCALES.size(); i++) {
            assertTrue(STREAMING_SERVICE_INFO.getLocales().contains(LOCALES.get(i)));
            assertTrue(LOCALES.contains(STREAMING_SERVICE_INFO.getLocales().get(i)));
        }
        assertEquals(LOCALE_DICT.size(), STREAMING_SERVICE_INFO.getNamedContentLocales().size());
        for (Locale l : STREAMING_SERVICE_INFO.getNamedContentLocales()) {
            assertTrue(LOCALE_DICT.containsKey(l));
            assertEquals(LOCALE_DICT.get(l), STREAMING_SERVICE_INFO.getNameForLocale(l).toString());
        }

        assertEquals(BEGIN_DATE, STREAMING_SERVICE_INFO.getSessionStartTime());
        assertEquals(END_DATE, STREAMING_SERVICE_INFO.getSessionEndTime());
        assertEquals(NAME, STREAMING_SERVICE_INFO.getServiceClassName());
    }
}
