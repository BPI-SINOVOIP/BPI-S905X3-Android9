/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.tradefed.device;

import com.android.ddmlib.MultiLineReceiver;
import com.android.tradefed.log.LogUtil.CLog;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Parser for 'adb shell dumpsys package' output.
 */
class DumpsysPackageReceiver extends MultiLineReceiver {

    /** the text that marks the beginning of the hidden system packages section in output */
    private static final String HIDDEN_SYSTEM_PACKAGES_PREFIX = "Hidden system packages:";

    /** regex for marking the start of a single package's output */
    private static final Pattern PACKAGE_PATTERN = Pattern.compile("Package\\s\\[([\\w\\.]+)\\]");

    @SuppressWarnings("serial")
    static class ParseException extends IOException {
        ParseException(String msg) {
            super(msg);
        }

        ParseException(String msg, Throwable t) {
            super(msg, t);
        }
    }

    /**
     * State handling interface for parsing output. Using GoF state design pattern.
     */
    private interface ParserState {
        ParserState parse(String line) throws ParseException;
    }

    /**
     * Initial state of package parser, where its looking for start of package to parse.
     */
    private class PackagesParserState implements ParserState {

        /**
         * {@inheritDoc}
         */
        @Override
        public ParserState parse(String line) throws ParseException {
            Matcher matcher = PACKAGE_PATTERN.matcher(line);
            if (matcher.find()) {
                String name = matcher.group(1);
                return new PackageParserState(name);
            }
            return this;
        }
    }

    /**
     * Parser for a single package's data.
     * <p/>
     * Expected pattern is:
     * Package: [com.foo]
     *   key=value
     *   key2=value2
     */
    private class PackageParserState implements ParserState {

        private PackageInfo mPkgInfo;

        /**
         * @param name
         */
        public PackageParserState(String name) {
            mPkgInfo = new PackageInfo(name);
            addPackage(name, mPkgInfo);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public ParserState parse(String line) throws ParseException {
            // first look if we've moved on to another package
            Matcher matcher = PACKAGE_PATTERN.matcher(line);
            if (matcher.find()) {
                String name = matcher.group(1);
                return new PackageParserState(name);
            }
            if (line.startsWith(HIDDEN_SYSTEM_PACKAGES_PREFIX)) {
                // done parsing packages, now parse hidden packages
                return new HiddenPackagesParserState();
            }
            parseAttributes(line);

            return this;
        }

        private void parseAttributes(String line) {
            String[] prop = line.split("=");
            if (prop.length == 2) {
                mPkgInfo.addAttribute(prop[0], prop[1]);
            } else if (prop.length > 2) {
                // multiple props on one line. Split by both whitespace and =
                String[] vn = line.split(" |=");
                if (vn.length % 2 != 0) {
                    // improper format, ignore
                    return;
                }
                for (int i=0; i < vn.length; i = i + 2) {
                    mPkgInfo.addAttribute(vn[i], vn[i+1]);
                }
            }

        }
    }

    /**
     * State of package parser where its looking for start of hidden packages to parse.
     */
    private class HiddenPackagesParserState implements ParserState {

        /**
         * {@inheritDoc}
         */
        @Override
        public ParserState parse(String line) throws ParseException {
            Matcher matcher = PACKAGE_PATTERN.matcher(line);
            if (matcher.find()) {
                String name = matcher.group(1);
                return new HiddenPackageParserState(name);
            }
            return this;
        }
    }

    /**
     * Parser for a single package's data
     */
    private class HiddenPackageParserState implements ParserState {

        private PackageInfo mPkgInfo;

        /**
         * @param name
         * @throws ParseException
         */
        public HiddenPackageParserState(String name) throws ParseException {
            mPkgInfo = mPkgInfoMap.get(name);
            if (mPkgInfo == null) {
                throw new ParseException(String.format(
                        "could not find package for hidden package %s", name));
            }
            mPkgInfo.setIsUpdatedSystemApp(true);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public ParserState parse(String line) throws ParseException {
            Matcher matcher = PACKAGE_PATTERN.matcher(line);
            if (matcher.find()) {
                String name = matcher.group(1);
                return new HiddenPackageParserState(name);
            }
            return this;
        }
    }

    private Map<String, PackageInfo> mPkgInfoMap = new HashMap<String, PackageInfo>();

    private ParserState mCurrentState = new PackagesParserState();

    private boolean mCancelled = false;

    void addPackage(String name, PackageInfo pkgInfo) {
        mPkgInfoMap.put(name, pkgInfo);
    }

    /**
     * @return the parsed {@link PackageInfo}s as a map of package name to {@link PackageInfo}.
     */
    public Map<String, PackageInfo> getPackages() {
        return mPkgInfoMap;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isCancelled() {
        return mCancelled ;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void processNewLines(String[] lines) {
        try {
            for (String line : lines) {
                mCurrentState = mCurrentState.parse(line);
            }
        } catch (ParseException e) {
            CLog.e(e);
            mCancelled = true;
        }
    }
}
