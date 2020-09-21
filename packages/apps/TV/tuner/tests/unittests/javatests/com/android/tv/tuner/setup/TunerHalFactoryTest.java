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

package com.android.tv.tuner.setup;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;

import android.os.AsyncTask;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import com.android.tv.tuner.TunerHal;
import com.android.tv.tuner.setup.BaseTunerSetupActivity.TunerHalFactory;
import java.util.concurrent.Executor;
import org.junit.Test;
import org.junit.runner.RunWith;

/** Tests for {@link TunerHalFactory}. */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class TunerHalFactoryTest {
    private final FakeExecutor mFakeExecutor = new FakeExecutor();

    private static class TestTunerHalFactory extends TunerHalFactory {
        private TestTunerHalFactory(Executor executor) {
            super(null, executor);
        }

        @Override
        protected TunerHal createInstance() {
            return new com.android.tv.tuner.FakeTunerHal() {};
        }
    }

    private static class FakeExecutor implements Executor {
        Runnable mCommand;

        @Override
        public synchronized void execute(final Runnable command) {
            mCommand = command;
        }

        private synchronized void executeActually() {
            mCommand.run();
        }
    }

    @Test
    public void test_asyncGet() {
        TunerHalFactory tunerHalFactory = new TestTunerHalFactory(mFakeExecutor);
        assertNull(tunerHalFactory.mTunerHal);
        tunerHalFactory.generate();
        assertNull(tunerHalFactory.mTunerHal);
        mFakeExecutor.executeActually();
        TunerHal tunerHal = tunerHalFactory.getOrCreate();
        assertNotNull(tunerHal);
        assertSame(tunerHal, tunerHalFactory.getOrCreate());
        tunerHalFactory.clear();
    }

    @Test
    public void test_syncGet() {
        TunerHalFactory tunerHalFactory = new TestTunerHalFactory(AsyncTask.SERIAL_EXECUTOR);
        assertNull(tunerHalFactory.mTunerHal);
        tunerHalFactory.generate();
        assertNotNull(tunerHalFactory.getOrCreate());
    }

    @Test
    public void test_syncGetWithoutGenerate() {
        TunerHalFactory tunerHalFactory = new TestTunerHalFactory(mFakeExecutor);
        assertNull(tunerHalFactory.mTunerHal);
        assertNotNull(tunerHalFactory.getOrCreate());
    }
}
