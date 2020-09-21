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

package vogar.android;

import junit.framework.AssertionFailedError;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

/**
 * Obtains test specific arguments from the {@link VogarArgs} annotation on the test and makes them
 * available to the test itself.
 *
 * @see VogarArgs
 */
public class VogarArgsRule implements TestRule {

    private String[] testSpecificArgs = null;

    @Override
    public Statement apply(Statement base, Description description) {
        VogarArgs vogarArgs = description.getAnnotation(VogarArgs.class);
        if (vogarArgs == null) {
            throw new AssertionFailedError("Must specify @" + VogarArgs.class.getSimpleName());
        } else {
            this.testSpecificArgs = vogarArgs.value();
        }
        return base;
    }

    public String[] getTestSpecificArgs() {
        return testSpecificArgs;
    }
}
