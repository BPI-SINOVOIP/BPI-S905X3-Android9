/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.loganalysis.item.BugreportItem;

import junit.framework.TestCase;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

/**
 * Functional tests for {@link BugreportParser}
 */
public class BugreportParserFuncTest extends TestCase {
    // FIXME: Make bugreport file configurable.
    private static final String BUGREPORT_PATH = "/tmp/bugreport.txt";

    /**
     * A test that is intended to force Brillopad to parse a bugreport. The purpose of this is to
     * assist a developer in checking why a given bugreport file might not be parsed correctly by
     * Brillopad.
     */
    public void testParse() {
        BufferedReader bugreportReader = null;
        try {
            bugreportReader = new BufferedReader(new FileReader(BUGREPORT_PATH));
        } catch (FileNotFoundException e) {
            fail(String.format("File not found at %s", BUGREPORT_PATH));
        }
        BugreportItem bugreport = null;
        try {
            long start = System.currentTimeMillis();
            bugreport = new BugreportParser().parse(bugreportReader);
            long stop = System.currentTimeMillis();
            System.out.println(String.format("Bugreport took %d ms to parse.", stop - start));
        } catch (IOException e) {
            fail(String.format("IOException: %s", e.toString()));
        } finally {
            if (bugreportReader != null) {
                try {
                    bugreportReader.close();
                } catch (IOException e) {
                    // Ignore
                }
            }
        }

        assertNotNull(bugreport);
        assertNotNull(bugreport.getTime());

        assertNotNull(bugreport.getSystemProps());
        assertTrue(bugreport.getSystemProps().size() > 0);

        assertNotNull(bugreport.getMemInfo());
        assertTrue(bugreport.getMemInfo().size() > 0);

        assertNotNull(bugreport.getProcrank());
        assertTrue(bugreport.getProcrank().getPids().size() > 0);

        assertNotNull(bugreport.getSystemLog());
        assertNotNull(bugreport.getSystemLog().getStartTime());
        assertNotNull(bugreport.getSystemLog().getStopTime());

        assertNotNull(bugreport.getLastKmsg());

        System.out.println(String.format("Stats for bugreport:\n" +
                "  Time: %s\n" +
                "  System Properties: %d items\n" +
                "  Mem info: %d items\n" +
                "  Procrank: %d items\n" +
                "  System Log:\n" +
                "    Start time: %s\n" +
                "    Stop time: %s\n" +
                "    %d ANR(s), %d Java Crash(es), %d Native Crash(es)\n" +
                "    %d Kernel Reset(s), %d Kernel Error(s)",
                bugreport.getTime(),
                bugreport.getSystemProps().size(),
                bugreport.getMemInfo().size(),
                bugreport.getProcrank().getPids().size(),
                bugreport.getSystemLog().getStartTime().toString(),
                bugreport.getSystemLog().getStopTime().toString(),
                bugreport.getSystemLog().getAnrs().size(),
                bugreport.getSystemLog().getJavaCrashes().size(),
                bugreport.getSystemLog().getNativeCrashes().size(),
                bugreport.getLastKmsg().getMiscEvents(KernelLogParser.KERNEL_RESET).size(),
                bugreport.getLastKmsg().getMiscEvents(KernelLogParser.KERNEL_ERROR).size()));
    }
}

