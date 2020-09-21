/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.loganalysis.item.QtaguidItem;

import java.util.List;

/**
 * An {@link IParser} to handle the output of {@code xt_qtaguid}.
 */
public class QtaguidParser implements IParser {

    /**
     * Parses the output of "cat /proc/net/xt_qtaguid/stats".
     * This method only parses total received bytes and total sent bytes per user.
     *
     * xt_qtaguid contains network usage per uid in simple space separated format.
     * The first row of the output is description, and actual data follow. Example:
     *   IDX IFACE ACCT_TAG_HEX UID_TAG_INT CNT_SET RX_BYTES RX_PACKETS TX_BYTES ... (omitted)
     *   2 wlan0 0x0 0 0 669013 7534 272120 ...
     *   3 wlan0 0x0 0 1 0 0 0 ...
     *   4 wlan0 0x0 1000 0 104010 860 135166 ...
     *   ...
     */
    @Override
    public QtaguidItem parse(List<String> lines) {
        QtaguidItem item = new QtaguidItem();
        for (String line : lines) {
            String[] columns = line.split(" ", -1);
            if (columns.length < 8 || columns[0].equals("IDX")) {
                continue;
            }

            try {
                int uid = Integer.parseInt(columns[3]);
                int rxBytes = Integer.parseInt(columns[5]);
                int txBytes = Integer.parseInt(columns[7]);

                if (item.contains(uid)) {
                    item.updateRow(uid, rxBytes, txBytes);
                } else {
                    item.addRow(uid, rxBytes, txBytes);
                }
            } catch (NumberFormatException e) {
                // ignore
            }
        }

        return item;
    }
}
