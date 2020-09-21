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

import com.android.loganalysis.item.IItem;

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for {@link AbstractSectionParser}
 */
public class AbstractSectionParserTest extends TestCase {
    AbstractSectionParser mParser = null;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mParser = new AbstractSectionParser() {
            @Override
            public IItem parse(List<String> lines) {
                for (String line : lines) {
                    parseLine(line);
                }
                commit();
                return null;
            }
        };
    }

    private static class FakeBlockParser implements IParser {
        private String mExpected = null;
        private int mCalls = 0;

        public FakeBlockParser(String expected) {
            mExpected = expected;
        }

        public int getCalls() {
            return mCalls;
        }

        @Override
        public IItem parse(List<String> input) {
            assertEquals(1, input.size());
            assertEquals("parseBlock() got unexpected input!", mExpected, input.get(0));
            mCalls += 1;
            return null;
        }
    }

    /**
     * Verifies that {@link AbstractSectionParser} switches between parsers as expected
     */
    public void testSwitchParsers() {
        final String lineFormat = "howdy, parser %d!";
        final String linePattern = "I spy %d candles";
        final int nParsers = 4;
        FakeBlockParser[] parsers = new FakeBlockParser[nParsers];
        final List<String> lines = new ArrayList<String>(2*nParsers);

        for (int i = 0; i < nParsers; ++i) {
            String line = String.format(lineFormat, i);
            FakeBlockParser parser = new FakeBlockParser(line);
            mParser.addSectionParser(parser, String.format(linePattern, i));
            parsers[i] = parser;

            // add the parser trigger
            lines.add(String.format(linePattern, i));
            // and then add the line that the parser is expecting
            lines.add(String.format(lineFormat, i));
        }

        mParser.parse(lines);

        // Verify that all the parsers were run
        for (int i = 0; i < nParsers; ++i) {
            assertEquals(String.format("Parser %d has wrong call count!", i), 1,
                    parsers[i].getCalls());
        }
    }
}

