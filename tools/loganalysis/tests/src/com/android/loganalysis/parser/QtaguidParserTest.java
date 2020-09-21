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

import com.android.loganalysis.item.QtaguidItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

public class QtaguidParserTest extends TestCase {

    public void testSingleLine() {
        List<String> input = Arrays.asList("12 wlan0 0x0 10009 0 111661 353 258252 484 111661 353 0 0 0 0 258252 484 0 0 0 0");

        QtaguidItem item = new QtaguidParser().parse(input);

        assertEquals(1, item.getUids().size());
        assertEquals(111661, item.getRxBytes(10009));
        assertEquals(258252, item.getTxBytes(10009));
    }

    public void testMalformedLine() {
        List<String> input = Arrays.asList("a b c d", "a b c d e f g h i j k l");

        QtaguidItem item = new QtaguidParser().parse(input);

        assertEquals(0, item.getUids().size());
    }

    public void testMultipleLines() {
        List <String> input = Arrays.asList(
                "IDX IFACE ACCT_TAG_HEX UID_TAG_INT CNT_SET RX_BYTES RX_PACKETS TX_BYTES TX_PACKETS " +
                "rx_tcp_bytes rx_tcp_packets rx_udp_bytes rx_udp_packets rx_other_bytes rx_other_packets " +
                "tx_tcp_bytes tx_tcp_packets tx_udp_bytes tx_udp_packets tx_other_bytes tx_other_packets",
                "2 wlan0 0x0 0 0 669013 7534 272120 2851 161253 1606 203916 1228 303844 4700 123336 998 108412 1268 40372 585",
                "3 wlan0 0x0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
                "4 wlan0 0x0 1000 0 104010 860 135166 2090 2304 23 101706 837 0 0 3774 36 85344 843 46048 1211",
                "5 wlan0 0x0 1000 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
                "6 wlan0 0x0 1020 0 68666 162 110566 260 0 0 68666 162 0 0 0 0 110566 260 0 0",
                "7 wlan0 0x0 1020 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
                "8 wlan0 0x0 10000 0 826063 2441 486365 2402 725175 2202 100888 239 0 0 427377 2261 58988 141 0 0",
                "9 wlan0 0x0 10000 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
                "10 wlan0 0x0 10007 0 0 0 640 10 0 0 0 0 0 0 640 10 0 0 0 0",
                "11 wlan0 0x0 10007 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
                "12 wlan0 0x0 10009 0 17773800 18040 6588861 16079 17773800 18040 0 0 0 0 6588861 16079 0 0 0 0",
                "13 wlan0 0x0 10009 1 6392090 5092 109907 1235 6392090 5092 0 0 0 0 109907 1235 0 0 0 0",
                "14 wlan0 0x0 10010 0 33397 41 5902 59 33397 41 0 0 0 0 5902 59 0 0 0 0",
                "15 wlan0 0x0 10010 1 14949782 13336 1099914 12201 14949782 13336 0 0 0 0 1099914 12201 0 0 0 0",
                "16 wlan0 0x0 10014 0 54459314 43660 1000730 9780 54459314 43660 0 0 0 0 1000730 9780 0 0 0 0",
                "17 wlan0 0x0 10014 1 5411545 4459 416719 4154 5411545 4459 0 0 0 0 416719 4154 0 0 0 0",
                "18 wlan0 0x0 10024 0 98775055 80471 3929490 45504 98775055 80471 0 0 0 0 3929490 45504 0 0 0 0",
                "19 wlan0 0x0 10024 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        QtaguidItem item = new QtaguidParser().parse(input);
        assertEquals(9, item.getUids().size());
        assertEquals(24165890, item.getRxBytes(10009));
        assertEquals(6698768, item.getTxBytes(10009));
    }
}
