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

package vogar.target.junit;

import java.util.List;
import org.junit.runner.Runner;
import org.junit.runners.Suite;
import org.junit.runners.model.InitializationError;

/**
 * A {@link Suite} that allows the list of {@link Runner runners} to be supplied in the constructor
 * and will take a {@link String} name instead of a {@link Class}.
 */
public class ExtendedSuiteRunner extends Suite {
    private final String name;

    public ExtendedSuiteRunner(String name, List<Runner> runners)
            throws InitializationError {
        super(Dummy.class, runners);
        this.name = name;
    }

    /**
     * ParentRunner requires that the class be public.
     */
    public static class Dummy {
    }

    @Override
    protected String getName() {
        return name;
    }
}
