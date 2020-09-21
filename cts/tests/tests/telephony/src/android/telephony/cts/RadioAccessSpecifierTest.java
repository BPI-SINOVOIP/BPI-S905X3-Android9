/*
 * Copyright (C) 2018 The Android Open Source Project
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
package android.telephony.cts;

import static com.google.common.truth.Truth.assertThat;

import android.os.Parcel;
import android.telephony.RadioAccessSpecifier;
import android.test.AndroidTestCase;

public class RadioAccessSpecifierTest extends AndroidTestCase {
    public void testConstructorAndGetters() {
        // Constructor and getters.
        int ran = 1;
        int[] bands = {1, 2, 3, 4};
        int[] channels = {5, 6, 7};
        RadioAccessSpecifier radioAccessSpecifier = new RadioAccessSpecifier(ran, bands, channels);
        assertThat(radioAccessSpecifier.describeContents()).isEqualTo(0);
        assertThat(radioAccessSpecifier.getRadioAccessNetwork()).isEqualTo(ran);
        assertThat(radioAccessSpecifier.getBands()).isEqualTo(bands);
        assertThat(radioAccessSpecifier.getChannels()).isEqualTo(channels);

        // Comparision method.
        RadioAccessSpecifier toCompare1 = new RadioAccessSpecifier(ran, bands, channels);
        RadioAccessSpecifier toCompare2 = new RadioAccessSpecifier(ran, new int[] {1, 2, 3, 4},
                new int[] {5, 6, 7});
        RadioAccessSpecifier toCompare3 = new RadioAccessSpecifier(ran+1, bands, channels);
        assertThat(radioAccessSpecifier).isEqualTo(toCompare1);
        assertThat(radioAccessSpecifier).isEqualTo(toCompare2);
        assertThat(radioAccessSpecifier).isNotEqualTo(toCompare3);

        // Parcel read and write.
        Parcel stateParcel = Parcel.obtain();
        radioAccessSpecifier.writeToParcel(stateParcel, 0);
        stateParcel.setDataPosition(0);
        toCompare1 = RadioAccessSpecifier.CREATOR.createFromParcel(stateParcel);
        assertThat(radioAccessSpecifier).isEqualTo(toCompare1);

        // Other methods.
        assertThat(radioAccessSpecifier.hashCode()).isGreaterThan(0);
        assertThat(radioAccessSpecifier.toString()).isNotNull();
    }
}
