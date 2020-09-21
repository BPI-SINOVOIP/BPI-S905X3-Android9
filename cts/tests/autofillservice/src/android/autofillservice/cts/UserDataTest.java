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

import static android.provider.Settings.Secure.AUTOFILL_USER_DATA_MAX_CATEGORY_COUNT;
import static android.provider.Settings.Secure.AUTOFILL_USER_DATA_MAX_FIELD_CLASSIFICATION_IDS_SIZE;
import static android.provider.Settings.Secure.AUTOFILL_USER_DATA_MAX_USER_DATA_SIZE;
import static android.provider.Settings.Secure.AUTOFILL_USER_DATA_MAX_VALUE_LENGTH;
import static android.provider.Settings.Secure.AUTOFILL_USER_DATA_MIN_VALUE_LENGTH;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import android.autofillservice.cts.common.SettingsStateChangerRule;
import android.content.Context;
import android.platform.test.annotations.AppModeFull;
import android.service.autofill.UserData;
import android.support.test.InstrumentationRegistry;

import com.google.common.base.Strings;

import org.junit.Before;
import org.junit.ClassRule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
@AppModeFull // Unit test
public class UserDataTest {

    private static final Context sContext = InstrumentationRegistry.getContext();

    @ClassRule
    public static final SettingsStateChangerRule sUserDataMaxFcSizeChanger =
            new SettingsStateChangerRule(sContext,
                    AUTOFILL_USER_DATA_MAX_FIELD_CLASSIFICATION_IDS_SIZE, "10");

    @ClassRule
    public static final SettingsStateChangerRule sUserDataMaxCategoriesSizeChanger =
            new SettingsStateChangerRule(sContext, AUTOFILL_USER_DATA_MAX_CATEGORY_COUNT, "2");

    @ClassRule
    public static final SettingsStateChangerRule sUserDataMaxUserSizeChanger =
            new SettingsStateChangerRule(sContext, AUTOFILL_USER_DATA_MAX_USER_DATA_SIZE, "4");

    @ClassRule
    public static final SettingsStateChangerRule sUserDataMinValueChanger =
            new SettingsStateChangerRule(sContext, AUTOFILL_USER_DATA_MIN_VALUE_LENGTH, "4");

    @ClassRule
    public static final SettingsStateChangerRule sUserDataMaxValueChanger =
            new SettingsStateChangerRule(sContext, AUTOFILL_USER_DATA_MAX_VALUE_LENGTH, "50");


    private final String mShortValue = Strings.repeat("k", UserData.getMinValueLength() - 1);
    private final String mLongValue = "LONG VALUE, Y U NO SHORTER"
            + Strings.repeat("?", UserData.getMaxValueLength());
    private final String mId = "4815162342";
    private final String mCategoryId = "id1";
    private final String mCategoryId2 = "id2";
    private final String mCategoryId3 = "id3";
    private final String mValue = mShortValue + "-1";
    private final String mValue2 = mShortValue + "-2";
    private final String mValue3 = mShortValue + "-3";
    private final String mValue4 = mShortValue + "-4";
    private final String mValue5 = mShortValue + "-5";

    private UserData.Builder mBuilder;

    @Before
    public void setFixtures() {
        mBuilder = new UserData.Builder(mId, mValue, mCategoryId);
    }

    @Test
    public void testBuilder_invalid() {
        assertThrows(NullPointerException.class,
                () -> new UserData.Builder(null, mValue, mCategoryId));
        assertThrows(IllegalArgumentException.class,
                () -> new UserData.Builder("", mValue, mCategoryId));
        assertThrows(NullPointerException.class,
                () -> new UserData.Builder(mId, null, mCategoryId));
        assertThrows(IllegalArgumentException.class,
                () -> new UserData.Builder(mId, "", mCategoryId));
        assertThrows(IllegalArgumentException.class,
                () -> new UserData.Builder(mId, mShortValue, mCategoryId));
        assertThrows(IllegalArgumentException.class,
                () -> new UserData.Builder(mId, mLongValue, mCategoryId));
        assertThrows(NullPointerException.class, () -> new UserData.Builder(mId, mValue, null));
        assertThrows(IllegalArgumentException.class, () -> new UserData.Builder(mId, mValue, ""));
    }

    @Test
    public void testAdd_invalid() {
        assertThrows(NullPointerException.class, () -> mBuilder.add(null, mCategoryId));
        assertThrows(IllegalArgumentException.class, () -> mBuilder.add("", mCategoryId));
        assertThrows(IllegalArgumentException.class, () -> mBuilder.add(mShortValue, mCategoryId));
        assertThrows(IllegalArgumentException.class, () -> mBuilder.add(mLongValue, mCategoryId));
        assertThrows(NullPointerException.class, () -> mBuilder.add(mValue, null));
        assertThrows(IllegalArgumentException.class, () -> mBuilder.add(mValue, ""));
    }

    @Test
    public void testAdd_duplicatedValue() {
        assertThrows(IllegalStateException.class, () -> mBuilder.add(mValue, mCategoryId));
        assertThrows(IllegalStateException.class, () -> mBuilder.add(mValue, mCategoryId2));
    }

    @Test
    public void testAdd_maximumCategoriesReached() {
        // Max is 2; one was added in the constructor
        mBuilder.add(mValue2, mCategoryId2);
        assertThrows(IllegalStateException.class, () -> mBuilder.add(mValue3, mCategoryId3));
    }

    @Test
    public void testAdd_maximumUserDataReached() {
        // Max is 4; one was added in the constructor
        mBuilder.add(mValue2, mCategoryId);
        mBuilder.add(mValue3, mCategoryId);
        mBuilder.add(mValue4, mCategoryId2);
        assertThrows(IllegalStateException.class, () -> mBuilder.add(mValue5, mCategoryId2));
    }

    @Test
    public void testSetFcAlgorithm() {
        final UserData userData = mBuilder.setFieldClassificationAlgorithm("algo_mas", null)
                .build();
        assertThat(userData.getFieldClassificationAlgorithm()).isEqualTo("algo_mas");
    }

    @Test
    public void testBuild_valid() {
        final UserData userData = mBuilder.build();
        assertThat(userData).isNotNull();
        assertThat(userData.getId()).isEqualTo(mId);
        assertThat(userData.getFieldClassificationAlgorithm()).isNull();
    }

    @Test
    public void testNoMoreInteractionsAfterBuild() {
        testBuild_valid();

        assertThrows(IllegalStateException.class, () -> mBuilder.add(mValue, mCategoryId2));
        assertThrows(IllegalStateException.class,
                () -> mBuilder.setFieldClassificationAlgorithm("algo_mas", null));
        assertThrows(IllegalStateException.class, () -> mBuilder.build());
    }
}
