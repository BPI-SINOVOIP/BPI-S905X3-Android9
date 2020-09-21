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

package com.google.android.tv.partner.support;

import static com.google.common.truth.Truth.assertThat;

import com.android.tv.testing.constants.ConfigConstants;
import com.google.thirdparty.robolectric.GoogleRobolectricTestRunner;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

/** Tests for {@link TunerSetupUtils} */
@RunWith(GoogleRobolectricTestRunner.class)
@Config(sdk = ConfigConstants.SDK)
public class TunerSetupUtilsTest {

    @Test
    public void getMatchCount_allMatch() {
        List<String> channelNumbers1 = Arrays.asList("1.1", "1.2", "2.3");
        List<List<String>> parsedChannelNumbers2 =
                TunerSetupUtils.parseChannelNumbers(Arrays.asList("1.1", "1.2", "2.3"));
        assertThat(TunerSetupUtils.getMatchCount(channelNumbers1, parsedChannelNumbers2))
                .isEqualTo(3);
    }

    @Test
    public void getMatchCount_someMatch() {
        List<String> channelNumbers1 = Arrays.asList("1.1", "1.2", "2.3");
        List<List<String>> parsedChannelNumbers2 =
                TunerSetupUtils.parseChannelNumbers(Arrays.asList("1.0", "1.1", "2"));
        assertThat(TunerSetupUtils.getMatchCount(channelNumbers1, parsedChannelNumbers2))
                .isEqualTo(2);
    }

    @Test
    public void getMatchCount_noMatch() {
        List<String> channelNumbers1 = Arrays.asList("1.1", "1.2", "2.3");
        List<List<String>> parsedChannelNumbers2 =
                TunerSetupUtils.parseChannelNumbers(Arrays.asList("1.0", "1.3", "3"));
        assertThat(TunerSetupUtils.getMatchCount(channelNumbers1, parsedChannelNumbers2))
                .isEqualTo(0);
    }

    @Test
    public void parseChannelNumber_majorNumberOnly() {
        assertThat(TunerSetupUtils.parseChannelNumber("11")).containsExactly("11");
    }

    @Test
    public void parseChannelNumber_majorAndMinorNumbers() {
        assertThat(TunerSetupUtils.parseChannelNumber("2-11")).containsExactly("2", "11");
    }

    @Test
    public void parseChannelNumber_containsExtraSpaces() {
        assertThat(TunerSetupUtils.parseChannelNumber(" 2 - 11 ")).containsExactly("2", "11");
    }

    @Test
    public void parseChannelNumber_tooLong() {
        assertThat(TunerSetupUtils.parseChannelNumber("2-11-1")).isEmpty();
    }

    @Test
    public void parseChannelNumber_empty() {
        assertThat(TunerSetupUtils.parseChannelNumber("  ")).isEmpty();
    }

    @Test
    public void matchChannelNumber_match_majorAndMinorNumbers() {
        assertThat(
                        TunerSetupUtils.matchChannelNumber(
                                Arrays.asList("22", "1"), Arrays.asList("22", "1")))
                .isTrue();
    }

    @Test
    public void matchChannelNumber_match_majorNumber() {
        assertThat(
                        TunerSetupUtils.matchChannelNumber(
                                Collections.singletonList("22"), Collections.singletonList("22")))
                .isTrue();
    }

    @Test
    public void matchChannelNumber_match_differentSize() {
        assertThat(
                        TunerSetupUtils.matchChannelNumber(
                                Arrays.asList("22", "1"), Collections.singletonList("22")))
                .isTrue();
    }

    @Test
    public void matchChannelNumber_notMatch() {
        assertThat(
                        TunerSetupUtils.matchChannelNumber(
                                Arrays.asList("22", "1"), Collections.singletonList("11")))
                .isFalse();
    }
}
