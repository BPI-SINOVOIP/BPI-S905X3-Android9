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

import static org.mockito.Mockito.mock;
import static org.testng.Assert.assertThrows;

import android.platform.test.annotations.AppModeFull;
import android.service.autofill.BatchUpdates;
import android.service.autofill.CustomDescription;
import android.service.autofill.InternalValidator;
import android.service.autofill.Transformation;
import android.service.autofill.Validator;
import android.support.test.runner.AndroidJUnit4;
import android.widget.RemoteViews;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@AppModeFull // Unit test
public class CustomDescriptionUnitTest {

    private final CustomDescription.Builder mBuilder =
            new CustomDescription.Builder(mock(RemoteViews.class));
    private final BatchUpdates mValidUpdate =
            new BatchUpdates.Builder().updateTemplate(mock(RemoteViews.class)).build();
    private final Validator mValidCondition = mock(InternalValidator.class);

    @Test
    public void testNullConstructor() {
        assertThrows(NullPointerException.class, () ->  new CustomDescription.Builder(null));
    }

    @Test
    public void testAddChild_null() {
        assertThrows(IllegalArgumentException.class, () ->  mBuilder.addChild(42, null));
    }

    @Test
    public void testAddChild_invalidTransformation() {
        assertThrows(IllegalArgumentException.class,
                () ->  mBuilder.addChild(42, mock(Transformation.class)));
    }

    @Test
    public void testBatchUpdate_nullCondition() {
        assertThrows(IllegalArgumentException.class,
                () ->  mBuilder.batchUpdate(null, mValidUpdate));
    }

    @Test
    public void testBatchUpdate_invalidCondition() {
        assertThrows(IllegalArgumentException.class,
                () ->  mBuilder.batchUpdate(mock(Validator.class), mValidUpdate));
    }

    @Test
    public void testBatchUpdate_nullUpdates() {
        assertThrows(NullPointerException.class,
                () ->  mBuilder.batchUpdate(mValidCondition, null));
    }
}
