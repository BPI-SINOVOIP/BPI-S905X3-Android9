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

package vogar.target.junit4;

import java.util.Locale;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

/**
 * Verify that the locale is reset to Locale.US before/after each test is run.
 *
 * <p>This ensures that the {@link vogar.target.TestEnvironment} class is used correctly.
 */
public class ChangeDefaultLocaleTest {

    @Test
    public void testDefault_Locale_CANADA() {
        assertEquals(Locale.US, Locale.getDefault());
        Locale.setDefault(Locale.CANADA);
    }

    @Test
    public void testDefault_Locale_CHINA() {
        assertEquals(Locale.US, Locale.getDefault());
        Locale.setDefault(Locale.CHINA);
    }
}
