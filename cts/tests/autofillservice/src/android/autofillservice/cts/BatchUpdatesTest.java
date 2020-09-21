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

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;
import static org.testng.Assert.assertThrows;

import android.platform.test.annotations.AppModeFull;
import android.service.autofill.BatchUpdates;
import android.service.autofill.InternalTransformation;
import android.service.autofill.Transformation;
import android.support.test.runner.AndroidJUnit4;
import android.widget.RemoteViews;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@AppModeFull // Unit test
public class BatchUpdatesTest {

    private final BatchUpdates.Builder mBuilder = new BatchUpdates.Builder();

    @Test
    public void testAddTransformation_null() {
        assertThrows(IllegalArgumentException.class, () ->  mBuilder.transformChild(42, null));
    }

    @Test
    public void testAddTransformation_invalidClass() {
        assertThrows(IllegalArgumentException.class,
                () ->  mBuilder.transformChild(42, mock(Transformation.class)));
    }

    @Test
    public void testSetUpdateTemplate_null() {
        assertThrows(NullPointerException.class, () ->  mBuilder.updateTemplate(null));
    }

    @Test
    public void testEmptyObject() {
        assertThrows(IllegalStateException.class, () ->  mBuilder.build());
    }

    @Test
    public void testNoMoreChangesAfterBuild() {
        assertThat(mBuilder.updateTemplate(mock(RemoteViews.class)).build()).isNotNull();
        assertThrows(IllegalStateException.class,
                () ->  mBuilder.updateTemplate(mock(RemoteViews.class)));
        assertThrows(IllegalStateException.class,
                () ->  mBuilder.transformChild(42, mock(InternalTransformation.class)));
    }
}
