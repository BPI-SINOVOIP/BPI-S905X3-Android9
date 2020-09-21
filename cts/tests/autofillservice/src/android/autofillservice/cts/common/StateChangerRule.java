/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.autofillservice.cts.common;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.common.base.Preconditions;

import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import java.util.Objects;

/**
 * JUnit rule used to prepare a state before the test is run.
 *
 * <p>It stores the current state before the test, changes it (if necessary), then restores it after
 * the test (if necessary).
 */
public class StateChangerRule<T> implements TestRule {

    private final StateManager<T> mStateManager;
    private final T mValue;

    /**
     * Default constructor.
     *
     * @param stateManager abstraction used to mange the state.
     * @param value value to be set before the test is run.
     */
    public StateChangerRule(@NonNull StateManager<T> stateManager, @Nullable T value) {
        mStateManager = Preconditions.checkNotNull(stateManager);
        mValue = value;
    }

    @Override
    public Statement apply(Statement base, Description description) {
        return new Statement() {

            @Override
            public void evaluate() throws Throwable {
                final T previousValue = mStateManager.get();
                if (!Objects.equals(previousValue, mValue)) {
                    mStateManager.set(mValue);
                }
                try {
                    base.evaluate();
                } finally {
                    final T currentValue = mStateManager.get();
                    if (!Objects.equals(currentValue, previousValue)) {
                        mStateManager.set(previousValue);
                        // TODO: if set() thowns a RuntimeException, JUnit will silently catch it
                        // and re-run the test case; it should fail instead.
                    }
                }
            }
        };
    }
}
