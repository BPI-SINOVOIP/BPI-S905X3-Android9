package junitparams.internal;

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.Description;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.TestClass;

import junitparams.internal.annotation.FrameworkMethodAnnotations;
import junitparams.internal.parameters.ParametersReader;
import junitparams.naming.MacroSubstitutionNamingStrategy;
import junitparams.naming.TestCaseNamingStrategy;

/**
 * A wrapper for a test method
 *
 * @author Pawel Lipinski
 */
public class TestMethod {
    private FrameworkMethod frameworkMethod;
    private FrameworkMethodAnnotations frameworkMethodAnnotations;
    private Class<?> testClass;
    private ParametersReader parametersReader;
    private Object[] cachedParameters;
    private TestCaseNamingStrategy namingStrategy;
    private DescribableFrameworkMethod describableFrameworkMethod;

    public TestMethod(FrameworkMethod method, TestClass testClass) {
        this.frameworkMethod = method;
        this.testClass = testClass.getJavaClass();
        frameworkMethodAnnotations = new FrameworkMethodAnnotations(method);
        parametersReader = new ParametersReader(testClass(), frameworkMethod);

        namingStrategy = new MacroSubstitutionNamingStrategy(this);
    }

    public String name() {
        return frameworkMethod.getName();
    }

    public static List<FrameworkMethod> listFrom(TestClass testClass) {
        List<FrameworkMethod> methods = new ArrayList<FrameworkMethod>();

        for (FrameworkMethod frameworkMethod : testClass.getAnnotatedMethods(Test.class)) {
            TestMethod testMethod = new TestMethod(frameworkMethod, testClass);
            methods.add(testMethod.describableFrameworkMethod());
        }

        return methods;
    }

    @Override
    public int hashCode() {
        return frameworkMethod.hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        return (obj instanceof TestMethod)
                && hasTheSameNameAsFrameworkMethod((TestMethod) obj)
                && hasTheSameParameterTypesAsFrameworkMethod((TestMethod) obj);
    }

    private boolean hasTheSameNameAsFrameworkMethod(TestMethod testMethod) {
        return frameworkMethod.getName().equals(testMethod.frameworkMethod.getName());
    }

    private boolean hasTheSameParameterTypesAsFrameworkMethod(TestMethod testMethod) {
        Class<?>[] frameworkMethodParameterTypes = frameworkMethod.getMethod().getParameterTypes();
        Class<?>[] testMethodParameterTypes = testMethod.frameworkMethod.getMethod().getParameterTypes();
        return Arrays.equals(frameworkMethodParameterTypes, testMethodParameterTypes);
    }

    private Class<?> testClass() {
        return testClass;
    }

    private boolean isIgnored() {
        return hasIgnoredAnnotation() || hasNoParameters();
    }

    private boolean hasIgnoredAnnotation() {
        return frameworkMethodAnnotations.hasAnnotation(Ignore.class);
    }

    private boolean hasNoParameters() {
       return isParameterised() && parametersSets().length == 0;
    }

    private boolean isNotIgnored() {
        return !isIgnored();
    }

    public <T extends Annotation> T getAnnotation(Class<T> annotationType) {
        return frameworkMethodAnnotations.getAnnotation(annotationType);
    }

    private Description getDescription(Object[] params, int i) {
        Object paramSet = params[i];
        String name = namingStrategy.getTestCaseName(i, paramSet);
        String uniqueMethodId = Utils.uniqueMethodId(i, paramSet, name());

        return Description.createTestDescription(testClass().getName(), name, uniqueMethodId);
    }

    DescribableFrameworkMethod describableFrameworkMethod() {
        if (describableFrameworkMethod == null) {
            Description baseDescription = Description.createTestDescription(
                    testClass, name(), frameworkMethodAnnotations.allAnnotations());
            Method method = frameworkMethod.getMethod();
            try {
                describableFrameworkMethod =
                        createDescribableFrameworkMethod(method, baseDescription);
            } catch (IllegalStateException e) {
                // Defer error until running.
                describableFrameworkMethod =
                        new DeferredErrorFrameworkMethod(method, baseDescription, e);
            }
        }

        return describableFrameworkMethod;
    }

    private DescribableFrameworkMethod createDescribableFrameworkMethod(Method method, Description baseDescription) {
        if (isParameterised()) {
            if (isNotIgnored()) {
                Object[] parametersSets = parametersSets();
                List<InstanceFrameworkMethod> methods
                        = new ArrayList<InstanceFrameworkMethod>();
                for (int i = 0; i < parametersSets.length; i++) {
                    Object parametersSet = parametersSets[i];
                    Description description = getDescription(parametersSets, i);
                    methods.add(new InstanceFrameworkMethod(
                            method, baseDescription.childlessCopy(),
                            description, parametersSet));
                }

                return new ParameterisedFrameworkMethod(method, baseDescription, methods);
            }

            warnIfNoParamsGiven();
        } else {
            verifyMethodCanBeRunByStandardRunner(frameworkMethod);
        }

        // The method to use if it was ignored or was parameterized but had no parameters.
        return new NonParameterisedFrameworkMethod(method, baseDescription, isIgnored());
    }

    private void verifyMethodCanBeRunByStandardRunner(FrameworkMethod method) {
        List<Throwable> errors = new ArrayList<Throwable>();
        method.validatePublicVoidNoArg(false, errors);
        if (!errors.isEmpty()) {
            throw new RuntimeException(errors.get(0));
        }
    }

    public Object[] parametersSets() {
        if (cachedParameters == null) {
            cachedParameters = parametersReader.read();
        }
        return cachedParameters;
    }

    private void warnIfNoParamsGiven() {
        if (isNotIgnored() && isParameterised() && parametersSets().length == 0)
            System.err.println("Method " + name() + " gets empty list of parameters, so it's being ignored!");
    }

    public FrameworkMethod frameworkMethod() {
        return frameworkMethod;
    }

    boolean isParameterised() {
        return frameworkMethodAnnotations.isParametrised();
    }
}
