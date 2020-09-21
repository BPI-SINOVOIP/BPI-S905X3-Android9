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

package com.android.tv.testing;

import android.support.annotation.Nullable;
import com.android.tv.data.ChannelNumber;
import com.google.common.truth.ComparableSubject;
import com.google.common.truth.FailureMetadata;
import com.google.common.truth.Subject;
import com.google.common.truth.Truth;

/** Propositions for {@link ChannelNumber} subjects. */
public final class ChannelNumberSubject
        extends ComparableSubject<ChannelNumberSubject, ChannelNumber> {
    private static final Subject.Factory<ChannelNumberSubject, ChannelNumber> FACTORY =
            ChannelNumberSubject::new;

    public static Subject.Factory<ChannelNumberSubject, ChannelNumber> channelNumbers() {
        return FACTORY;
    }

    public static ChannelNumberSubject assertThat(@Nullable ChannelNumber actual) {
        return Truth.assertAbout(channelNumbers()).that(actual);
    }

    public ChannelNumberSubject(FailureMetadata failureMetadata, @Nullable ChannelNumber subject) {
        super(failureMetadata, subject);
    }

    public void displaysAs(int major) {
        if (!getSubject().majorNumber.equals(Integer.toString(major))
                || getSubject().hasDelimiter) {
            fail("displaysAs", major);
        }
    }

    public void displaysAs(int major, int minor) {
        if (!getSubject().majorNumber.equals(Integer.toString(major))
                || !getSubject().minorNumber.equals(Integer.toString(minor))
                || !getSubject().hasDelimiter) {
            fail("displaysAs", major + "-" + minor);
        }
    }

    public void isEmpty() {
        if (!getSubject().majorNumber.isEmpty()
                || !getSubject().minorNumber.isEmpty()
                || getSubject().hasDelimiter) {
            fail("isEmpty");
        }
    }
}
