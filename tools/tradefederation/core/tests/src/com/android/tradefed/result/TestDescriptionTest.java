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
package com.android.tradefed.result;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.lang.annotation.Annotation;

/** Unit tests for {@link TestDescription}. */
@RunWith(JUnit4.class)
public class TestDescriptionTest {
    private TestDescription mDescription;

    /** Annotation used for testing. */
    public static class TestAnnotation implements Annotation {
        @Override
        public Class<? extends Annotation> annotationType() {
            return TestAnnotation.class;
        }
    }

    /** Annotation used for testing. */
    public static class TestAnnotation2 implements Annotation {
        @Override
        public Class<? extends Annotation> annotationType() {
            return TestAnnotation2.class;
        }
    }

    /**
     * Create a {@link TestDescription} with annotations and ensure it's properly holding the data.
     */
    @Test
    public void testCreateDescription() {
        mDescription =
                new TestDescription(
                        "className", "testName", new TestAnnotation(), new TestAnnotation2());
        assertEquals("className", mDescription.getClassName());
        assertEquals("testName", mDescription.getTestName());
        assertEquals("testName", mDescription.getTestNameWithoutParams());
        assertEquals(2, mDescription.getAnnotations().size());
        assertNotNull(mDescription.getAnnotation(TestAnnotation.class));
        assertNotNull(mDescription.getAnnotation(TestAnnotation2.class));
    }

    /** Create a {@link TestDescription} without annotations and ensure it is properly reflected. */
    @Test
    public void testCreateDescription_noAnnotation() {
        mDescription = new TestDescription("className", "testName");
        assertEquals("className", mDescription.getClassName());
        assertEquals("testName", mDescription.getTestName());
        assertEquals(0, mDescription.getAnnotations().size());
        assertNull(mDescription.getAnnotation(TestAnnotation.class));
    }

    /** Create a {@link TestDescription} with a parameterized method. */
    @Test
    public void testCreateDescription_parameterized() {
        mDescription = new TestDescription("className", "testName[0]");
        assertEquals("className", mDescription.getClassName());
        assertEquals("testName[0]", mDescription.getTestName());
        assertEquals("testName", mDescription.getTestNameWithoutParams());

        mDescription = new TestDescription("className", "testName[key=value]");
        assertEquals("testName[key=value]", mDescription.getTestName());
        assertEquals("testName", mDescription.getTestNameWithoutParams());

        mDescription = new TestDescription("className", "testName[key=\"quoted value\"]");
        assertEquals("testName[key=\"quoted value\"]", mDescription.getTestName());
        assertEquals("testName", mDescription.getTestNameWithoutParams());

        mDescription = new TestDescription("className", "testName[key:22]");
        assertEquals("testName[key:22]", mDescription.getTestName());
        assertEquals("testName", mDescription.getTestNameWithoutParams());
    }
}
