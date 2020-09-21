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
package vogar.target.junit;

import java.util.concurrent.atomic.AtomicReference;
import javax.annotation.Nullable;
import vogar.monitor.TargetMonitor;
import vogar.target.RunnerFactory;
import vogar.target.TargetRunner;
import vogar.target.TestEnvironment;

/**
 * Creates a Runner for JUnit 3 or JUnit 4 tests.
 */
public class JUnitRunnerFactory implements RunnerFactory {

    @Override @Nullable
    public TargetRunner newRunner(TargetMonitor monitor, String qualification,
            Class<?> klass, AtomicReference<String> skipPastReference,
            TestEnvironment testEnvironment, int timeoutSeconds, String[] args) {
        if (supports(klass)) {
            return new JUnitTargetRunner(monitor, skipPastReference, testEnvironment,
                    timeoutSeconds, klass, qualification, args);
        } else {
            return null;
        }
    }

    private boolean supports(Class<?> klass) {
        return Junit3.isJunit3Test(klass) || Junit4.isJunit4Test(klass);
    }
}