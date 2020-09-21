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
 * limitations under the License
 */

package android.inputmethodservice.cts.devicetest;

import static android.inputmethodservice.cts.DeviceEvent.isFrom;
import static android.inputmethodservice.cts.DeviceEvent.isNewerThan;
import static android.inputmethodservice.cts.DeviceEvent.isType;
import static android.inputmethodservice.cts.common.DeviceEventConstants.DeviceEventType
        .ON_BIND_INPUT;
import static android.inputmethodservice.cts.common.DeviceEventConstants.DeviceEventType.ON_CREATE;
import static android.inputmethodservice.cts.common.DeviceEventConstants.DeviceEventType.ON_DESTROY;
import static android.inputmethodservice.cts.common.DeviceEventConstants.DeviceEventType.ON_START_INPUT;

import static android.inputmethodservice.cts.common.DeviceEventConstants.DeviceEventType
        .ON_UNBIND_INPUT;
import static android.inputmethodservice.cts.common.ImeCommandConstants.ACTION_IME_COMMAND;
import static android.inputmethodservice.cts.common.ImeCommandConstants
        .COMMAND_SWITCH_INPUT_METHOD_WITH_SUBTYPE;
import static android.inputmethodservice.cts.common.ImeCommandConstants.COMMAND_SWITCH_INPUT_METHOD;
import static android.inputmethodservice.cts.common.ImeCommandConstants
        .COMMAND_SWITCH_TO_PREVIOUS_INPUT;
import static android.inputmethodservice.cts.common.ImeCommandConstants
        .COMMAND_SWITCH_TO_NEXT_INPUT;
import static android.inputmethodservice.cts.common.ImeCommandConstants.EXTRA_ARG_STRING1;
import static android.inputmethodservice.cts.common.ImeCommandConstants.EXTRA_COMMAND;
import static android.inputmethodservice.cts.devicetest.BusyWaitUtils.pollingCheck;
import static android.inputmethodservice.cts.devicetest.MoreCollectors.startingFrom;

import android.inputmethodservice.cts.DeviceEvent;
import android.inputmethodservice.cts.common.DeviceEventConstants.DeviceEventType;
import android.inputmethodservice.cts.common.EditTextAppConstants;
import android.inputmethodservice.cts.common.Ime1Constants;
import android.inputmethodservice.cts.common.Ime2Constants;
import android.inputmethodservice.cts.common.test.DeviceTestConstants;
import android.inputmethodservice.cts.common.test.ShellCommandUtils;
import android.inputmethodservice.cts.devicetest.SequenceMatcher.MatchResult;
import android.os.SystemClock;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.concurrent.TimeUnit;
import java.util.function.IntFunction;
import java.util.function.Predicate;
import java.util.stream.Collector;

@RunWith(AndroidJUnit4.class)
public class InputMethodServiceDeviceTest {

    private static final long TIMEOUT = TimeUnit.SECONDS.toMillis(5);

    /** Test to check CtsInputMethod1 receives onCreate and onStartInput. */
    @Test
    public void testCreateIme1() throws Throwable {
        final TestHelper helper = new TestHelper(getClass(), DeviceTestConstants.TEST_CREATE_IME1);

        final long startActivityTime = SystemClock.uptimeMillis();
        helper.launchActivity(EditTextAppConstants.PACKAGE, EditTextAppConstants.CLASS,
                EditTextAppConstants.URI);

        pollingCheck(() -> helper.queryAllEvents()
                        .collect(startingFrom(helper.isStartOfTest()))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_CREATE))),
                TIMEOUT, "CtsInputMethod1.onCreate is called");
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(startActivityTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_START_INPUT))),
                TIMEOUT, "CtsInputMethod1.onStartInput is called");
    }

    /** Test to check IME is switched from CtsInputMethod1 to CtsInputMethod2. */
    @Test
    public void testSwitchIme1ToIme2() throws Throwable {
        final TestHelper helper = new TestHelper(
                getClass(), DeviceTestConstants.TEST_SWITCH_IME1_TO_IME2);

        final long startActivityTime = SystemClock.uptimeMillis();
        helper.launchActivity(EditTextAppConstants.PACKAGE, EditTextAppConstants.CLASS,
                EditTextAppConstants.URI);

        pollingCheck(() -> helper.queryAllEvents()
                        .collect(startingFrom(helper.isStartOfTest()))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_CREATE))),
                TIMEOUT, "CtsInputMethod1.onCreate is called");
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(startActivityTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_START_INPUT))),
                TIMEOUT, "CtsInputMethod1.onStartInput is called");

        helper.findUiObject(EditTextAppConstants.EDIT_TEXT_RES_NAME).click();

        // Switch IME from CtsInputMethod1 to CtsInputMethod2.
        final long switchImeTime = SystemClock.uptimeMillis();
        helper.shell(ShellCommandUtils.broadcastIntent(
                ACTION_IME_COMMAND, Ime1Constants.PACKAGE,
                "-e", EXTRA_COMMAND, COMMAND_SWITCH_INPUT_METHOD,
                "-e", EXTRA_ARG_STRING1, Ime2Constants.IME_ID));

        pollingCheck(() -> helper.shell(ShellCommandUtils.getCurrentIme())
                        .equals(Ime2Constants.IME_ID),
                TIMEOUT, "CtsInputMethod2 is current IME");
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(switchImeTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_DESTROY))),
                TIMEOUT, "CtsInputMethod1.onDestroy is called");
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(switchImeTime))
                        .filter(isFrom(Ime2Constants.CLASS))
                        .collect(sequenceOfTypes(ON_CREATE, ON_BIND_INPUT, ON_START_INPUT))
                        .matched(),
                TIMEOUT,
                "CtsInputMethod2.onCreate, onBindInput, and onStartInput are called"
                        + " in sequence");
    }

    @Test
    public void testSwitchInputMethod() throws Throwable {
        final TestHelper helper = new TestHelper(
                getClass(), DeviceTestConstants.TEST_SWITCH_INPUTMETHOD);
        final long startActivityTime = SystemClock.uptimeMillis();
        helper.launchActivity(EditTextAppConstants.PACKAGE, EditTextAppConstants.CLASS,
                EditTextAppConstants.URI);
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(startActivityTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_START_INPUT))),
                TIMEOUT, "CtsInputMethod1.onStartInput is called");
        helper.findUiObject(EditTextAppConstants.EDIT_TEXT_RES_NAME).click();

        final long setImeTime = SystemClock.uptimeMillis();
        // call setInputMethodAndSubtype(IME2, null)
        helper.shell(ShellCommandUtils.broadcastIntent(
                ACTION_IME_COMMAND, Ime1Constants.PACKAGE,
                "-e", EXTRA_COMMAND, COMMAND_SWITCH_INPUT_METHOD_WITH_SUBTYPE,
                "-e", EXTRA_ARG_STRING1, Ime2Constants.IME_ID));
        pollingCheck(() -> helper.shell(ShellCommandUtils.getCurrentIme())
                        .equals(Ime2Constants.IME_ID),
                TIMEOUT, "CtsInputMethod2 is current IME");
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(setImeTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_DESTROY))),
                TIMEOUT, "CtsInputMethod1.onDestroy is called");
    }

    @Test
    public void testSwitchToNextInputMethod() throws Throwable {
        final TestHelper helper = new TestHelper(
                getClass(), DeviceTestConstants.TEST_SWITCH_NEXT_INPUT);
        final long startActivityTime = SystemClock.uptimeMillis();
        helper.launchActivity(EditTextAppConstants.PACKAGE, EditTextAppConstants.CLASS,
                EditTextAppConstants.URI);
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(startActivityTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_START_INPUT))),
                TIMEOUT, "CtsInputMethod1.onStartInput is called");
        helper.findUiObject(EditTextAppConstants.EDIT_TEXT_RES_NAME).click();

        pollingCheck(() -> helper.shell(ShellCommandUtils.getCurrentIme())
                        .equals(Ime1Constants.IME_ID),
                TIMEOUT, "CtsInputMethod1 is current IME");
        helper.shell(ShellCommandUtils.broadcastIntent(
                ACTION_IME_COMMAND, Ime1Constants.PACKAGE,
                "-e", EXTRA_COMMAND, COMMAND_SWITCH_TO_NEXT_INPUT));
        pollingCheck(() -> !helper.shell(ShellCommandUtils.getCurrentIme())
                        .equals(Ime1Constants.IME_ID),
                TIMEOUT, "CtsInputMethod1 shouldn't be current IME");
    }

    @Test
    public void switchToPreviousInputMethod() throws Throwable {
        final TestHelper helper = new TestHelper(
                getClass(), DeviceTestConstants.TEST_SWITCH_PREVIOUS_INPUT);
        final long startActivityTime = SystemClock.uptimeMillis();
        helper.launchActivity(EditTextAppConstants.PACKAGE, EditTextAppConstants.CLASS,
                EditTextAppConstants.URI);
        helper.findUiObject(EditTextAppConstants.EDIT_TEXT_RES_NAME).click();

        final String initialIme = helper.shell(ShellCommandUtils.getCurrentIme());
        helper.shell(ShellCommandUtils.setCurrentIme(Ime2Constants.IME_ID));
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(startActivityTime))
                        .anyMatch(isFrom(Ime2Constants.CLASS).and(isType(ON_START_INPUT))),
                TIMEOUT, "CtsInputMethod2.onStartInput is called");
        helper.shell(ShellCommandUtils.broadcastIntent(
                ACTION_IME_COMMAND, Ime2Constants.PACKAGE,
                "-e", EXTRA_COMMAND, COMMAND_SWITCH_TO_PREVIOUS_INPUT));
        pollingCheck(() -> helper.shell(ShellCommandUtils.getCurrentIme())
                        .equals(initialIme),
                TIMEOUT, initialIme + " is current IME");
    }

    @Test
    public void testInputUnbindsOnImeStopped() throws Throwable {
        final TestHelper helper = new TestHelper(
                getClass(), DeviceTestConstants.TEST_INPUT_UNBINDS_ON_IME_STOPPED);
        final long startActivityTime = SystemClock.uptimeMillis();
        helper.launchActivity(EditTextAppConstants.PACKAGE, EditTextAppConstants.CLASS,
                EditTextAppConstants.URI);
        helper.findUiObject(EditTextAppConstants.EDIT_TEXT_RES_NAME).click();

        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(startActivityTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_START_INPUT))),
                TIMEOUT, "CtsInputMethod1.onStartInput is called");
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(startActivityTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_BIND_INPUT))),
                TIMEOUT, "CtsInputMethod1.onBindInput is called");

        final long imeForceStopTime = SystemClock.uptimeMillis();
        helper.shell(ShellCommandUtils.uninstallPackage(Ime1Constants.PACKAGE));

        helper.shell(ShellCommandUtils.setCurrentIme(Ime2Constants.IME_ID));
        helper.findUiObject(EditTextAppConstants.EDIT_TEXT_RES_NAME).click();
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(imeForceStopTime))
                        .anyMatch(isFrom(Ime2Constants.CLASS).and(isType(ON_START_INPUT))),
                TIMEOUT, "CtsInputMethod2.onStartInput is called");
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(imeForceStopTime))
                        .anyMatch(isFrom(Ime2Constants.CLASS).and(isType(ON_BIND_INPUT))),
                TIMEOUT, "CtsInputMethod2.onBindInput is called");
    }

    @Test
    public void testInputUnbindsOnAppStopped() throws Throwable {
        final TestHelper helper = new TestHelper(
                getClass(), DeviceTestConstants.TEST_INPUT_UNBINDS_ON_APP_STOPPED);
        final long startActivityTime = SystemClock.uptimeMillis();
        helper.launchActivity(EditTextAppConstants.PACKAGE, EditTextAppConstants.CLASS,
                EditTextAppConstants.URI);
        helper.findUiObject(EditTextAppConstants.EDIT_TEXT_RES_NAME).click();

        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(startActivityTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_START_INPUT))),
                TIMEOUT, "CtsInputMethod1.onStartInput is called");
        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(startActivityTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_BIND_INPUT))),
                TIMEOUT, "CtsInputMethod1.onBindInput is called");

        helper.shell(ShellCommandUtils.uninstallPackage(EditTextAppConstants.PACKAGE));

        pollingCheck(() -> helper.queryAllEvents()
                        .filter(isNewerThan(startActivityTime))
                        .anyMatch(isFrom(Ime1Constants.CLASS).and(isType(ON_UNBIND_INPUT))),
                TIMEOUT, "CtsInputMethod1.onUnBindInput is called");
    }

    /**
     * Build stream collector of {@link DeviceEvent} collecting sequence that elements have
     * specified types.
     *
     * @param types {@link DeviceEventType}s that elements of sequence should have.
     * @return {@link java.util.stream.Collector} that corrects the sequence.
     */
    private static Collector<DeviceEvent, ?, MatchResult<DeviceEvent>> sequenceOfTypes(
            final DeviceEventType... types) {
        final IntFunction<Predicate<DeviceEvent>[]> arraySupplier = Predicate[]::new;
        return SequenceMatcher.of(Arrays.stream(types)
                .map(DeviceEvent::isType)
                .toArray(arraySupplier));
    }
}
