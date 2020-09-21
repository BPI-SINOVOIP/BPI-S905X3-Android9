/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.loganalysis.parser;



import com.android.loganalysis.item.InterruptItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link InterruptParser}
 */
public class InterruptParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testInterruptParser() {
        List<String> inputBlock = Arrays.asList(
               " Wakeup reason 200:qcom,smd-rpm:222:fc4: 11m 49s 332ms (0 times) realtime",
               " Wakeup reason 200:qcom,smd-rpm: 48s 45ms (0 times) realtime",
               " Wakeup reason 2:qcom,smd-rpm:2:f0.qm,mm:22:fc4mi: 3s 417ms (0 times) realtime",
               " Wakeup reason 188:qcom,smd-adsp:200:qcom,smd-rpm: 1s 656ms (0 times) realtime",
               " Wakeup reason 58:qcom,smsm-modem:2:qcom,smd-rpm: 6m 16s 1ms (5 times) realtime",
               " Wakeup reason 57:qcom,smd-modem:200:qcom,smd-rpm: 40s 995ms (0 times) realtime",
               " Wakeup reason unknown: 8s 455ms (0 times) realtime",
               " Wakeup reason 9:bcmsdh_sdmmc:2:qcomd-rpm:240:mso: 8m 5s 9ms (0 times) realtime");

        InterruptItem interrupt = new InterruptParser().parse(inputBlock);

        assertEquals(1, interrupt.getInterrupts(
                InterruptItem.InterruptCategory.WIFI_INTERRUPT).size());

        assertEquals("9:bcmsdh_sdmmc:2:qcomd-rpm:240:mso", interrupt.getInterrupts(
                InterruptItem.InterruptCategory.WIFI_INTERRUPT).get(0).getName());

        assertEquals(2, interrupt.getInterrupts(
                InterruptItem.InterruptCategory.MODEM_INTERRUPT).size());

        assertEquals(5, interrupt.getInterrupts(InterruptItem.InterruptCategory.MODEM_INTERRUPT).
                get(0).getInterruptCount());

    }
}

