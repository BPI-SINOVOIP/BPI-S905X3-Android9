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

import static android.service.autofill.FillResponse.FLAG_DISABLE_ACTIVITY_ONLY;
import static android.service.autofill.FillResponse.FLAG_TRACK_CONTEXT_COMMITED;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import android.content.IntentSender;
import android.os.Bundle;
import android.platform.test.annotations.AppModeFull;
import android.service.autofill.Dataset;
import android.service.autofill.FillResponse;
import android.service.autofill.SaveInfo;
import android.service.autofill.UserData;
import android.view.autofill.AutofillId;
import android.view.autofill.AutofillValue;
import android.widget.RemoteViews;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
@AppModeFull // Unit test
public class FillResponseTest {

    private final AutofillId mAutofillId = new AutofillId(42);
    private final FillResponse.Builder mBuilder = new FillResponse.Builder();
    private final AutofillId[] mIds = new AutofillId[] { mAutofillId };
    private final SaveInfo mSaveInfo = new SaveInfo.Builder(0, mIds).build();
    private final Bundle mClientState = new Bundle();
    private final Dataset mDataset = new Dataset.Builder()
            .setValue(mAutofillId, AutofillValue.forText("forty-two"))
            .build();
    private final long mDisableDuration = 666;
    @Mock private RemoteViews mPresentation;
    @Mock private RemoteViews mHeader;
    @Mock private RemoteViews mFooter;
    @Mock private IntentSender mIntentSender;

    @Test
    public void testBuilder_setAuthentication_invalid() {
        // null ids
        assertThrows(IllegalArgumentException.class,
                () -> mBuilder.setAuthentication(null, mIntentSender, mPresentation));
        // empty ids
        assertThrows(IllegalArgumentException.class,
                () -> mBuilder.setAuthentication(new AutofillId[] {}, mIntentSender,
                        mPresentation));
        // ids with null value
        assertThrows(IllegalArgumentException.class,
                () -> mBuilder.setAuthentication(new AutofillId[] {null}, mIntentSender,
                        mPresentation));
        // null intent sender
        assertThrows(IllegalArgumentException.class,
                () -> mBuilder.setAuthentication(mIds, null, mPresentation));
        // null presentation
        assertThrows(IllegalArgumentException.class,
                () -> mBuilder.setAuthentication(mIds, mIntentSender, null));
    }

    @Test
    public void testBuilder_setAuthentication_valid() {
        new FillResponse.Builder().setAuthentication(mIds, null, null);
        new FillResponse.Builder().setAuthentication(mIds, mIntentSender, mPresentation);
    }

    @Test
    public void testBuilder_setAuthentication_illegalState() {
        assertThrows(IllegalStateException.class,
                () -> new FillResponse.Builder().setHeader(mHeader).setAuthentication(mIds,
                        mIntentSender, mPresentation));
        assertThrows(IllegalStateException.class,
                () -> new FillResponse.Builder().setFooter(mFooter).setAuthentication(mIds,
                        mIntentSender, mPresentation));
    }

    @Test
    public void testBuilder_setHeaderOrFooterInvalid() {
        assertThrows(NullPointerException.class, () -> new FillResponse.Builder().setHeader(null));
        assertThrows(NullPointerException.class, () -> new FillResponse.Builder().setFooter(null));
    }

    @Test
    public void testBuilder_setHeaderOrFooterAfterAuthentication() {
        FillResponse.Builder builder =
                new FillResponse.Builder().setAuthentication(mIds, mIntentSender, mPresentation);
        assertThrows(IllegalStateException.class, () -> builder.setHeader(mHeader));
        assertThrows(IllegalStateException.class, () -> builder.setHeader(mFooter));
    }

    @Test
    public void testBuilder_setFlag_invalid() {
        assertThrows(IllegalArgumentException.class, () -> mBuilder.setFlags(-1));
    }

    @Test
    public void testBuilder_setFlag_valid() {
        mBuilder.setFlags(0);
        mBuilder.setFlags(FLAG_TRACK_CONTEXT_COMMITED);
        mBuilder.setFlags(FLAG_DISABLE_ACTIVITY_ONLY);
    }

    @Test
    public void testBuilder_disableAutofill_invalid() {
        assertThrows(IllegalArgumentException.class, () -> mBuilder.disableAutofill(0));
        assertThrows(IllegalArgumentException.class, () -> mBuilder.disableAutofill(-1));
    }

    @Test
    public void testBuilder_disableAutofill_valid() {
        mBuilder.disableAutofill(mDisableDuration);
        mBuilder.disableAutofill(Long.MAX_VALUE);
    }

    @Test
    public void testBuilder_disableAutofill_mustBeTheOnlyMethodCalled() {
        // No method can be called after disableAutofill()
        mBuilder.disableAutofill(mDisableDuration);
        assertThrows(IllegalStateException.class, () -> mBuilder.setSaveInfo(mSaveInfo));
        assertThrows(IllegalStateException.class, () -> mBuilder.addDataset(mDataset));
        assertThrows(IllegalStateException.class,
                () -> mBuilder.setAuthentication(mIds, mIntentSender, mPresentation));
        assertThrows(IllegalStateException.class,
                () -> mBuilder.setFieldClassificationIds(mAutofillId));
        assertThrows(IllegalStateException.class,
                () -> mBuilder.setClientState(mClientState));

        // And vice-versa...
        final FillResponse.Builder builder1 = new FillResponse.Builder().setSaveInfo(mSaveInfo);
        assertThrows(IllegalStateException.class, () -> builder1.disableAutofill(mDisableDuration));
        final FillResponse.Builder builder2 = new FillResponse.Builder().addDataset(mDataset);
        assertThrows(IllegalStateException.class, () -> builder2.disableAutofill(mDisableDuration));
        final FillResponse.Builder builder3 =
                new FillResponse.Builder().setAuthentication(mIds, mIntentSender, mPresentation);
        assertThrows(IllegalStateException.class, () -> builder3.disableAutofill(mDisableDuration));
        final FillResponse.Builder builder4 =
                new FillResponse.Builder().setFieldClassificationIds(mAutofillId);
        assertThrows(IllegalStateException.class, () -> builder4.disableAutofill(mDisableDuration));
        final FillResponse.Builder builder5 =
                new FillResponse.Builder().setClientState(mClientState);
        assertThrows(IllegalStateException.class, () -> builder5.disableAutofill(mDisableDuration));
    }

    @Test
    public void testBuilder_setFieldClassificationIds_invalid() {
        assertThrows(NullPointerException.class,
                () -> mBuilder.setFieldClassificationIds((AutofillId) null));
        assertThrows(NullPointerException.class,
                () -> mBuilder.setFieldClassificationIds((AutofillId[]) null));
        final AutofillId[] oneTooMany =
                new AutofillId[UserData.getMaxFieldClassificationIdsSize() + 1];
        for (int i = 0; i < oneTooMany.length; i++) {
            oneTooMany[i] = new AutofillId(i);
        }
        assertThrows(IllegalArgumentException.class,
                () -> mBuilder.setFieldClassificationIds(oneTooMany));
    }

    @Test
    public void testBuilder_setFieldClassificationIds_valid() {
        mBuilder.setFieldClassificationIds(mAutofillId);
    }

    @Test
    public void testBuilder_setFieldClassificationIds_setsFlag() {
        mBuilder.setFieldClassificationIds(mAutofillId);
        assertThat(mBuilder.build().getFlags()).isEqualTo(FLAG_TRACK_CONTEXT_COMMITED);
    }

    @Test
    public void testBuilder_setFieldClassificationIds_addsFlag() {
        mBuilder.setFlags(FLAG_DISABLE_ACTIVITY_ONLY).setFieldClassificationIds(mAutofillId);
        assertThat(mBuilder.build().getFlags())
                .isEqualTo(FLAG_TRACK_CONTEXT_COMMITED | FLAG_DISABLE_ACTIVITY_ONLY);
    }

    @Test
    public void testBuild_invalid() {
        assertThrows(IllegalStateException.class, () -> mBuilder.build());
    }

    @Test
    public void testBuild_valid() {
        // authentication only
        assertThat(new FillResponse.Builder().setAuthentication(mIds, mIntentSender, mPresentation)
                .build()).isNotNull();
        // save info only
        assertThat(new FillResponse.Builder().setSaveInfo(mSaveInfo).build()).isNotNull();
        // dataset only
        assertThat(new FillResponse.Builder().addDataset(mDataset).build()).isNotNull();
        // disable autofill only
        assertThat(new FillResponse.Builder().disableAutofill(mDisableDuration).build())
                .isNotNull();
        // fill detection only
        assertThat(new FillResponse.Builder().setFieldClassificationIds(mAutofillId).build())
                .isNotNull();
        // client state only
        assertThat(new FillResponse.Builder().setClientState(mClientState).build())
                .isNotNull();
    }

    @Test
    public void testBuilder_build_headerOrFooterWithoutDatasets() {
        assertThrows(IllegalStateException.class,
                () -> new FillResponse.Builder().setHeader(mHeader).build());
        assertThrows(IllegalStateException.class,
                () -> new FillResponse.Builder().setFooter(mFooter).build());
    }

    @Test
    public void testNoMoreInteractionsAfterBuild() {
        assertThat(mBuilder.setAuthentication(mIds, mIntentSender, mPresentation).build())
                .isNotNull();

        assertThrows(IllegalStateException.class, () -> mBuilder.build());
        assertThrows(IllegalStateException.class,
                () -> mBuilder.setAuthentication(mIds, mIntentSender, mPresentation).build());
        assertThrows(IllegalStateException.class, () -> mBuilder.setIgnoredIds(mIds));
        assertThrows(IllegalStateException.class, () -> mBuilder.addDataset(null));
        assertThrows(IllegalStateException.class, () -> mBuilder.setSaveInfo(mSaveInfo));
        assertThrows(IllegalStateException.class, () -> mBuilder.setClientState(mClientState));
        assertThrows(IllegalStateException.class, () -> mBuilder.setFlags(0));
        assertThrows(IllegalStateException.class,
                () -> mBuilder.setFieldClassificationIds(mAutofillId));
        assertThrows(IllegalStateException.class, () -> mBuilder.setHeader(mHeader));
        assertThrows(IllegalStateException.class, () -> mBuilder.setFooter(mFooter));
    }
}
