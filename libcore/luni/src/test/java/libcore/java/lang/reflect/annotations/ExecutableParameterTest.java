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

package libcore.java.lang.reflect.annotations;

import junit.framework.TestCase;

import java.lang.annotation.Annotation;
import java.lang.reflect.Constructor;
import java.lang.reflect.Executable;
import java.lang.reflect.Method;
import java.lang.reflect.Parameter;

import libcore.java.lang.reflect.annotations.AnnotatedElementTestSupport.AnnotationA;
import libcore.java.lang.reflect.annotations.AnnotatedElementTestSupport.AnnotationB;
import libcore.java.lang.reflect.annotations.AnnotatedElementTestSupport.AnnotationC;
import libcore.java.lang.reflect.annotations.AnnotatedElementTestSupport.AnnotationD;
import libcore.java.lang.reflect.annotations.AnnotatedElementTestSupport.Container;
import libcore.java.lang.reflect.annotations.AnnotatedElementTestSupport.Repeated;

import static libcore.java.lang.reflect.annotations.AnnotatedElementTestSupport.EXPECT_EMPTY;
import static libcore.java.lang.reflect.annotations.AnnotatedElementTestSupport.annotationsToTypes;
import static libcore.java.lang.reflect.annotations.AnnotatedElementTestSupport.assertAnnotationsMatch;
import static libcore.java.lang.reflect.annotations.AnnotatedElementTestSupport.set;

/**
 * Tests for {@link Executable#getParameterAnnotations()} via the {@link Constructor} and
 * {@link Method} classes. See {@link  for testing of the
 * {@link java.lang.reflect.AnnotatedElement} methods.
 */
public class ExecutableParameterTest extends TestCase {
    private static class MethodClass {
        public void methodWithoutAnnotatedParameters(String parameter1, String parameter2) {}

        public void methodWithAnnotatedParameters(@AnnotationB @AnnotationD String parameter1,
                @AnnotationC @AnnotationD String parameter2) {}
    }

    public void testMethodGetParameterAnnotations() throws Exception {
        Method methodWithoutAnnotatedParameters = MethodClass.class.getMethod(
                "methodWithoutAnnotatedParameters", String.class, String.class);
        Annotation[][] noParameterAnnotations = getParameterAnnotations(
                methodWithoutAnnotatedParameters, 2);
        assertEquals(set(), annotationsToTypes(noParameterAnnotations[0]));
        assertEquals(set(), annotationsToTypes(noParameterAnnotations[1]));

        Method methodWithAnnotatedParameters = MethodClass.class.getMethod(
                "methodWithAnnotatedParameters", String.class, String.class);
        Annotation[][] parameterAnnotations = getParameterAnnotations(
                methodWithAnnotatedParameters, 2);
        assertEquals(set(AnnotationB.class, AnnotationD.class),
                annotationsToTypes(parameterAnnotations[0]));
        assertEquals(set(AnnotationC.class, AnnotationD.class),
                annotationsToTypes(parameterAnnotations[1]));
    }

    private static class AnnotatedMethodClass {
        void noAnnotation(String p0) {}

        void multipleAnnotationOddity(
                @Repeated(1) @Container({@Repeated(2), @Repeated(3)}) String p0) {}

        void multipleAnnotationExplicitSingle(@Container({@Repeated(1)}) String p0) {}

        void multipleAnnotation(@Repeated(1) @Repeated(2) String p0) {}

        void singleAnnotation(@Repeated(1) String p0) {}

        static void staticSingleAnnotation(@Repeated(1) String p0) {}

        static Method getMethodWithoutAnnotations() throws Exception {
            return AnnotatedMethodClass.class.getDeclaredMethod("noAnnotation", String.class);
        }

        static Method getMethodMultipleAnnotationOddity() throws Exception {
            return AnnotatedMethodClass.class.getDeclaredMethod(
                    "multipleAnnotationOddity", String.class);
        }

        static Method getMethodMultipleAnnotationExplicitSingle() throws Exception {
            return AnnotatedMethodClass.class.getDeclaredMethod(
                    "multipleAnnotationExplicitSingle", String.class);
        }

        static Method getMethodMultipleAnnotation() throws Exception {
            return AnnotatedMethodClass.class.getDeclaredMethod("multipleAnnotation", String.class);
        }

        static Method getMethodSingleAnnotation() throws Exception {
            return AnnotatedMethodClass.class.getDeclaredMethod("singleAnnotation", String.class);
        }

        static Method getMethodStaticSingleAnnotation() throws Exception {
            return AnnotatedMethodClass.class.getDeclaredMethod("staticSingleAnnotation",
                    String.class);
        }
    }

    private static abstract class AnnotatedMethodAbstractClass {
        abstract void abstractSingleAnnotation(@Repeated(1) String p0);

        static Method getMethodAbstractSingleAnnotation() throws Exception {
            return AnnotatedMethodAbstractClass.class.getDeclaredMethod(
                    "abstractSingleAnnotation", String.class);
        }
    }


    public void testMethodGetParameterAnnotations_repeated() throws Exception {
        assertOnlyParameterAnnotations(
                AnnotatedMethodClass.getMethodWithoutAnnotations(), EXPECT_EMPTY);
        assertOnlyParameterAnnotations(
                AnnotatedMethodClass.getMethodMultipleAnnotationOddity(),
                "@Repeated(1)", "@Container({@Repeated(2), @Repeated(3)})");
        assertOnlyParameterAnnotations(
                AnnotatedMethodClass.getMethodMultipleAnnotationExplicitSingle(),
                "@Container({@Repeated(1)})");
        assertOnlyParameterAnnotations(
                AnnotatedMethodClass.getMethodMultipleAnnotation(),
                "@Container({@Repeated(1), @Repeated(2)})");
        assertOnlyParameterAnnotations(
                AnnotatedMethodClass.getMethodSingleAnnotation(),
                "@Repeated(1)");
        assertOnlyParameterAnnotations(
                AnnotatedMethodClass.getMethodStaticSingleAnnotation(),
                "@Repeated(1)");
        assertOnlyParameterAnnotations(
                AnnotatedMethodAbstractClass.getMethodAbstractSingleAnnotation(),
                "@Repeated(1)");

    }

    private static class ConstructorClass {
        // No annotations.
        public ConstructorClass(Integer parameter1, Integer parameter2) {}

        // Annotations.
        public ConstructorClass(@AnnotationB @AnnotationD String parameter1,
                @AnnotationC @AnnotationD String parameter2) {}
    }

    public void testConstructorGetParameterAnnotations() throws Exception {
        Constructor constructorWithoutAnnotatedParameters =
                ConstructorClass.class.getDeclaredConstructor(Integer.class, Integer.class);
        Annotation[][] noParameterAnnotations = getParameterAnnotations(
                constructorWithoutAnnotatedParameters, 2);
        assertEquals(set(), annotationsToTypes(noParameterAnnotations[0]));
        assertEquals(set(), annotationsToTypes(noParameterAnnotations[1]));

        Constructor constructorWithAnnotatedParameters =
                ConstructorClass.class.getDeclaredConstructor(String.class, String.class);
        Annotation[][] parameterAnnotations = getParameterAnnotations(
                constructorWithAnnotatedParameters, 2);
        assertEquals(set(AnnotationB.class, AnnotationD.class),
                annotationsToTypes(parameterAnnotations[0]));
        assertEquals(set(AnnotationC.class, AnnotationD.class),
                annotationsToTypes(parameterAnnotations[1]));
    }

    private static class AnnotatedConstructorClass {
        public AnnotatedConstructorClass(Boolean p0) {}

        public AnnotatedConstructorClass(
                @Repeated(1) @Container({@Repeated(2), @Repeated(3)}) Long p0) {}

        public AnnotatedConstructorClass(@Container({@Repeated(1)}) Double p0) {}

        public AnnotatedConstructorClass(@Repeated(1) @Repeated(2) Integer p0) {}

        public AnnotatedConstructorClass(@Repeated(1) String p0) {}

        static Constructor<?> getConstructorWithoutAnnotations() throws Exception {
            return AnnotatedConstructorClass.class.getDeclaredConstructor(Boolean.class);
        }

        static Constructor<?> getConstructorMultipleAnnotationOddity() throws Exception {
            return AnnotatedConstructorClass.class.getDeclaredConstructor(Long.class);
        }

        static Constructor<?> getConstructorMultipleAnnotationExplicitSingle()
                throws Exception {
            return AnnotatedConstructorClass.class.getDeclaredConstructor(Double.class);
        }

        static Constructor<?> getConstructorMultipleAnnotation() throws Exception {
            return AnnotatedConstructorClass.class.getDeclaredConstructor(Integer.class);
        }

        static Constructor<?> getConstructorSingleAnnotation() throws Exception {
            return AnnotatedConstructorClass.class.getDeclaredConstructor(String.class);
        }
    }

    public void testConstructorGetParameterAnnotations_repeated() throws Exception {
        assertOnlyParameterAnnotations(
                AnnotatedConstructorClass.getConstructorWithoutAnnotations(),
                EXPECT_EMPTY);
        assertOnlyParameterAnnotations(
                AnnotatedConstructorClass.getConstructorMultipleAnnotationOddity(),
                "@Repeated(1)", "@Container({@Repeated(2), @Repeated(3)})");
        assertOnlyParameterAnnotations(
                AnnotatedConstructorClass.getConstructorMultipleAnnotationExplicitSingle(),
                "@Container({@Repeated(1)})");
        assertOnlyParameterAnnotations(
                AnnotatedConstructorClass.getConstructorMultipleAnnotation(),
                "@Container({@Repeated(1), @Repeated(2)})");
        assertOnlyParameterAnnotations(
                AnnotatedConstructorClass.getConstructorSingleAnnotation(),
                "@Repeated(1)");
    }

    /**
     * As an inner class the constructor will actually have two parameters: the first, referencing
     * the enclosing object, is inserted by the compiler.
     */
    class InnerClass {
        InnerClass(@AnnotationA String p1) {}
    }

    /** Special case testing for a compiler-generated constructor parameter. JLS 8.8.1, JLS 13.1. */
    public void testImplicitConstructorParameters_innerClass() throws Exception {
        Constructor<InnerClass> constructor =
                InnerClass.class.getDeclaredConstructor(
                        ExecutableParameterTest.class, String.class);

        // The parameter annotation code behaves as if there are two parameters.
        Annotation[][] annotations = getParameterAnnotations(constructor, 2);
        assertAnnotationsMatch(annotations[0], new String[0]);
        assertAnnotationsMatch(annotations[1], new String[] { "@AnnotationA" });
    }

    static abstract class AnonymousBaseClass {
        public AnonymousBaseClass(@AnnotationA String p1) {}
    }

    /** Special case testing for a compiler-generated constructor parameter. JLS 13.1 */
    public void testImplicitConstructorParameters_anonymousClass() throws Exception {
        /*
         * As an anonymous class the constructor will actually have two parameters: the first,
         * referencing the enclosing object, is inserted by the compiler.
         */
        AnonymousBaseClass anonymousClassInstance = new AnonymousBaseClass("p1") {};

        Constructor<? extends AnonymousBaseClass> constructor =
                anonymousClassInstance.getClass().getDeclaredConstructor(
                        ExecutableParameterTest.class, String.class);
        // The parameter annotation code behaves as if there are two parameters.
        Annotation[][] annotations = getParameterAnnotations(constructor, 2);
        assertAnnotationsMatch(annotations[0], new String[0]);
        assertAnnotationsMatch(annotations[1], new String[0]);
    }

    /**
     * A static inner / nested member class will not have synthetic parameters and should behave
     * like a top-level class.
     */
    static class StaticInnerClass {
        StaticInnerClass(@AnnotationA String p1) {}
    }

    /** Special case testing for a compiler-generated constructor parameter. */
    public void testImplicitConstructorParameters_staticInnerClass() throws Exception {
        Constructor<StaticInnerClass> constructor =
                StaticInnerClass.class.getDeclaredConstructor(String.class);
        Parameter[] parameters = constructor.getParameters();
        assertEquals(1, parameters.length);

        Annotation[][] annotations = getParameterAnnotations(constructor, 1);
        assertAnnotationsMatch(annotations[0], new String[] { "@AnnotationA" });
    }

    private static void assertOnlyParameterAnnotations(
            Executable executable, String... expectedAnnotationStrings) throws Exception {
        Annotation[][] allAnnotations = getParameterAnnotations(executable, 1);

        Annotation[] p0Annotations = allAnnotations[0];
        assertAnnotationsMatch(p0Annotations, expectedAnnotationStrings);
    }

    private static Annotation[][] getParameterAnnotations(
            Executable executable, int expectedParameterAnnotationsSize) {
        Annotation[][] allAnnotations = executable.getParameterAnnotations();
        assertEquals(expectedParameterAnnotationsSize, allAnnotations.length);
        return allAnnotations;
    }
}
