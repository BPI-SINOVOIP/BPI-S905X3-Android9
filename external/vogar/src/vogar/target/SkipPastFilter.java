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

package vogar.target;

import java.util.concurrent.atomic.AtomicReference;
import org.junit.runner.Description;
import org.junit.runner.manipulation.Filter;
import vogar.target.junit.JUnitUtils;

/**
 * A {@link Filter} that skips past (discards) all tests up to and including one that failed
 * previously causing the process to exit.
 *
 * <p>If {@link #skipPastReference} has a null value then this lets all tests run. Otherwise, this
 * filters out all tests up to and including the one whose name matches the value in the reference
 * at which point the reference is set to null, so all following tests are kept.
 */
public class SkipPastFilter extends Filter {

    private final AtomicReference<String> skipPastReference;

    public SkipPastFilter(AtomicReference<String> skipPastReference) {
        this.skipPastReference = skipPastReference;
    }

    @Override
    public boolean shouldRun(Description description) {
        String skipPast = skipPastReference.get();
        if (description.isTest() && skipPast != null) {
            String name = JUnitUtils.getTestName(description);
            if (skipPast.equals(name)) {
                skipPastReference.set(null);
            }
            return false;
        }

        return true;
    }

    @Override
    public String describe() {
        return "SkipPastFilter";
    }
}
