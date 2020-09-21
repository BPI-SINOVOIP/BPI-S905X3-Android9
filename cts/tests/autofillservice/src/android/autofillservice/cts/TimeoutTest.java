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

package android.autofillservice.cts;

import static android.autofillservice.cts.Helper.assertFloat;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;
import static org.testng.Assert.expectThrows;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.concurrent.Callable;

@RunWith(MockitoJUnitRunner.class)
public class TimeoutTest {

    private static final String NAME = "TIME, Y U NO OUT?";
    private static final String DESC = "something";

    @Mock
    private Callable<Object> mJob;

    @Test
    public void testInvalidConstructor() {
        // Invalid name
        assertThrows(IllegalArgumentException.class, ()-> new Timeout(null, 1, 2, 2));
        assertThrows(IllegalArgumentException.class, ()-> new Timeout("", 1, 2, 2));
        // Invalid initial value
        assertThrows(IllegalArgumentException.class, ()-> new Timeout(NAME, -1, 2, 2));
        assertThrows(IllegalArgumentException.class, ()-> new Timeout(NAME, 0, 2, 2));
        // Invalid multiplier
        assertThrows(IllegalArgumentException.class, ()-> new Timeout(NAME, 1, -1, 2));
        assertThrows(IllegalArgumentException.class, ()-> new Timeout(NAME, 1, 0, 2));
        assertThrows(IllegalArgumentException.class, ()-> new Timeout(NAME, 1, 1, 2));
        // Invalid max value
        assertThrows(IllegalArgumentException.class, ()-> new Timeout(NAME, 1, 2, -1));
        assertThrows(IllegalArgumentException.class, ()-> new Timeout(NAME, 1, 2, 0));
        // Max value cannot be less than initial
        assertThrows(IllegalArgumentException.class, ()-> new Timeout(NAME, 2, 2, 1));
    }

    @Test
    public void testGetters() {
        final Timeout timeout = new Timeout(NAME, 1, 2, 5);
        assertThat(timeout.ms()).isEqualTo(1);
        assertFloat(timeout.getMultiplier(), 2);
        assertThat(timeout.getMaxValue()).isEqualTo(5);
        assertThat(timeout.getName()).isEqualTo(NAME);
    }

    @Test
    public void testIncrease() {
        final Timeout timeout = new Timeout(NAME, 1, 2, 5);
        // Pre-maximum
        assertThat(timeout.increase()).isEqualTo(1);
        assertThat(timeout.ms()).isEqualTo(2);
        assertThat(timeout.increase()).isEqualTo(2);
        assertThat(timeout.ms()).isEqualTo(4);
        // Post-maximum
        assertThat(timeout.increase()).isEqualTo(4);
        assertThat(timeout.ms()).isEqualTo(5);
        assertThat(timeout.increase()).isEqualTo(5);
        assertThat(timeout.ms()).isEqualTo(5);
    }

    @Test
    public void testRun_invalidArgs() {
        final Timeout timeout = new Timeout(NAME, 1, 2, 5);
        // Invalid description
        assertThrows(IllegalArgumentException.class, ()-> timeout.run(null, mJob));
        assertThrows(IllegalArgumentException.class, ()-> timeout.run("", mJob));
        // Invalid max attempts
        assertThrows(IllegalArgumentException.class, ()-> timeout.run(DESC, -1, mJob));
        assertThrows(IllegalArgumentException.class, ()-> timeout.run(DESC, 0, mJob));
        // Invalid job
        assertThrows(IllegalArgumentException.class, ()-> timeout.run(DESC, null));
    }

    @Test
    public void testRun_successOnFirstAttempt() throws Exception {
        final Timeout timeout = new Timeout(NAME, 100, 2, 500);
        final Object result = new Object();
        when(mJob.call()).thenReturn(result);
        assertThat(timeout.run(DESC, 1, mJob)).isSameAs(result);
    }

    @Test
    public void testRun_successOnSecondAttempt() throws Exception {
        final Timeout timeout = new Timeout(NAME, 100, 2, 500);
        final Object result = new Object();
        when(mJob.call()).thenReturn((Object) null, result);
        assertThat(timeout.run(DESC, 10, mJob)).isSameAs(result);
    }

    @Test
    public void testRun_allAttemptsFailed() throws Exception {
        final Timeout timeout = new Timeout(NAME, 100, 2, 500);
        final RetryableException e = expectThrows(RetryableException.class,
                () -> timeout.run(DESC, 10, mJob));
        assertThat(e.getMessage()).contains(DESC);
        assertThat(e.getTimeout()).isSameAs(timeout);
    }
}
