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
import com.android.loganalysis.util.RegexTrie;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * A {@link IParser} that splits an input file into discrete sections and passes each section to an
 * {@link IParser} to parse.
 * <p>
 * Before parsing input, {@link IParser}s can be added with
 * {@link #addSectionParser(IParser, String)}. The default parser is {@link NoopParser} but this can
 * be overwritten by calling {@link #setParser(IParser)} before parsing the input.
 * </p>
 */
public abstract class AbstractSectionParser implements IParser {
    private RegexTrie<IParser> mSectionTrie = new RegexTrie<IParser>();
    private IParser mCurrentParser = new NoopParser();
    private List<String> mParseBlock = new LinkedList<String>();
    private Map<IParser, IItem> mSections = new HashMap<IParser, IItem>();

    /**
     * A method to add a given section parser to the set of potential parsers to use.
     *
     * @param parser The {@link IParser} to add
     * @param pattern The regular expression to trigger this parser
    */
    protected void addSectionParser(IParser parser, String pattern) {
        if (parser == null) {
            throw new NullPointerException("Parser is null");
        }
        if (pattern == null) {
            throw new NullPointerException("Pattern is null");
        }
        mSectionTrie.put(parser, pattern);
    }

    /**
     * Parse a line of input, either adding the input to the current block or switching parsers and
     * running the current parser.
     *
     * @param line The line to parse
     */
    protected void parseLine(String line) {
        IParser nextParser = mSectionTrie.retrieve(line);

        if (nextParser == null) {
            // no match, so buffer this for the current parser, if there is one
            if (mCurrentParser != null) {
                mParseBlock.add(line);
            } else {
                // CLog.w("Line outside of parsed section: %s", line);
            }
        } else {
            runCurrentParser();
            mCurrentParser = nextParser;
        }
    }

    /**
     * Signal that the input has finished and run the last parser.
     */
    protected void commit() {
        runCurrentParser();
    }

    /**
     * Gets the {@link IItem} for a given section.
     *
     * @param parser The {@link IParser} type for the section.
     * @return The {@link IItem}.
     */
    protected IItem getSection(IParser parser) {
        return mSections.get(parser);
    }

    /**
     * Set the {@link IParser}. Used to set the initial parser.
     *
     * @param parser The {@link IParser} to set.
     */
    protected void setParser(IParser parser) {
        mCurrentParser = parser;
    }

    /**
     * Callback for when parsers are switched.
     */
    protected void onSwitchParser() {
    }

    /**
     * Run the current parser and add the {@link IItem} to the sections map.
     */
    private void runCurrentParser() {
        if (mCurrentParser != null) {
            IItem item = mCurrentParser.parse(mParseBlock);
            if (item != null && !(mCurrentParser instanceof NoopParser)) {
                mSections.put(mCurrentParser, item);
                // CLog.v("Just ran the %s parser", mCurrentParser.getClass().getSimpleName());
            }
        }

        mParseBlock.clear();
        onSwitchParser();
    }
}

