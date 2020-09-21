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
import com.android.loganalysis.item.InterruptItem.InterruptCategory;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A {@link IParser} to parse wake up interrupts
 */
public class InterruptParser implements IParser {

    /**
     * Matches: Wakeup reason 289:bcmsdh_sdmmc:200:qcom,smd-rpm:240:msmgpio:
     * 20m 5s 194ms (1485 times) realtime
     */
    private static final Pattern Interrupt = Pattern.compile(
            "^\\s*Wakeup reason (.*): (?:\\d+h )?(?:\\d+m )?(?:\\d+s )(?:\\d+ms )" +
            "\\((\\d+) times\\) realtime");

    private InterruptItem mItem = new InterruptItem();

    /**
     * {@inheritDoc}
     *
     * @return The {@link InterruptItem}.
     */
    @Override
    public InterruptItem parse(List<String> lines) {
        for (String line : lines) {
            Matcher m = Interrupt.matcher(line);
            if(m.matches()) {
                final String interruptName = m.group(1);
                final int interruptCount = Integer.parseInt(m.group(2));
                mItem.addInterrupt(interruptName, interruptCount,
                        getInterruptCategory(interruptName));
            } else {
                // Done with interrupts
                break;
            }
        }
        return mItem;
    }

    /**
     * Get the {@link InterruptItem}.
     * <p>
     * Exposed for unit testing.
     * </p>
     */
    InterruptItem getItem() {
        return mItem;
    }

    private InterruptCategory getInterruptCategory(String interruptName) {
        if (interruptName.contains("bcmsdh_sdmmc") || interruptName.contains("msm_pcie_wake")) {
            return InterruptCategory.WIFI_INTERRUPT;
        } else if (interruptName.contains("smd-modem") ||
                interruptName.contains("smsm-modem")) {
            return InterruptCategory.MODEM_INTERRUPT;
        } else if (interruptName.contains("smd-adsp")) {
            return InterruptCategory.ADSP_INTERRUPT;
        } else if (interruptName.contains("max77686-irq") ||
                interruptName.contains("cpcap-irq") ||
                interruptName.contains("TWL6030-PIH")) {
            return InterruptCategory.ALARM_INTERRUPT;
        }

        return InterruptCategory.UNKNOWN_INTERRUPT;
    }

}
