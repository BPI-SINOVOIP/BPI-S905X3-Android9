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

package com.android.tradefed.util;

import com.android.ddmlib.IShellOutputReceiver;
import com.android.ddmlib.MultiLineReceiver;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.Set;

/**
 * A {@link IShellOutputReceiver} that parses the output of a 'pm list instrumentation' query
 */
public class ListInstrumentationParser extends MultiLineReceiver {

    // Each line of output from `pm list instrumentation` has the following format:
    // instrumentation:com.example.test/android.test.InstrumentationTestRunner (target=com.example)
    private static final Pattern LIST_INSTR_PATTERN =
            Pattern.compile("instrumentation:(.+)/(.+) \\(target=(.+)\\)");

    // A set of shardable instrumentation runner names. A limitation of the `pm list
    // instrumentation` output is that Tradefed will be unable to tell whether or not an
    // Instrumentation is shardable. A workaround is to have a list of shardable instrumentation
    // runners, and check if a target uses that particular runner, although this means that any
    // subclasses or other custom runner classes won't be acknowledged as shardable.
    public static final Set<String> SHARDABLE_RUNNERS = new HashSet<>(Arrays.asList(
                "android.support.test.runner.AndroidJUnitRunner"));

    private List<InstrumentationTarget> mInstrumentationTargets = new ArrayList<>();

    public static class InstrumentationTarget implements Comparable<InstrumentationTarget> {
        public final String packageName;
        public final String runnerName;
        public final String targetName;

        public InstrumentationTarget(String packageName, String runnerName, String targetName) {
            this.packageName = packageName;
            this.runnerName = runnerName;
            this.targetName = targetName;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean equals(Object object) {
            if (object instanceof InstrumentationTarget) {
                InstrumentationTarget that = (InstrumentationTarget)object;
                return Objects.equals(this.packageName, that.packageName)
                        && Objects.equals(this.runnerName, that.runnerName)
                        && Objects.equals(this.targetName, that.targetName);
            }
            return false;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public int hashCode() {
            return Objects.hash(packageName, runnerName, targetName);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public int compareTo(InstrumentationTarget o) {
            if (!this.packageName.equals(o.packageName)) {
                return this.packageName.compareTo(o.packageName);
            } else if (!this.runnerName.equals(o.runnerName)) {
                return this.runnerName.compareTo(o.runnerName);
            }
            return this.targetName.compareTo(o.targetName);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public String toString() {
            return String.format("instrumentation:%s/%s (target=%s)",
                    packageName, runnerName, targetName);
        }

        /**
         * Returns <tt>true</tt> if this instrumentation target is shardable.
         */
        public boolean isShardable() {
            return SHARDABLE_RUNNERS.contains(runnerName);
        }
    }

    public List<InstrumentationTarget> getInstrumentationTargets() {
        Collections.sort(mInstrumentationTargets);
        return mInstrumentationTargets;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isCancelled() {
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void processNewLines(String[] lines) {
        for (String line : lines) {
            Matcher m = LIST_INSTR_PATTERN.matcher(line);
            if (m.find()) {
                mInstrumentationTargets.add(
                        new InstrumentationTarget(m.group(1), m.group(2), m.group(3)));
            }
        }
    }
}
