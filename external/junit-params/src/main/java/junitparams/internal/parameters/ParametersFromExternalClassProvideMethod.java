package junitparams.internal.parameters;

import junitparams.Parameters;
import org.junit.runners.model.FrameworkMethod;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

class ParametersFromExternalClassProvideMethod implements ParametrizationStrategy {
    private final ParamsFromMethodCommon paramsFromMethodCommon;
    private Parameters annotation;

    ParametersFromExternalClassProvideMethod(FrameworkMethod frameworkMethod) {
        this.paramsFromMethodCommon = new ParamsFromMethodCommon(frameworkMethod);
        annotation = frameworkMethod.getAnnotation(Parameters.class);
    }

    @Override
    public Object[] getParameters() {
        Class<?> sourceClass = annotation.source();
        return fillResultWithAllParamProviderMethods(sourceClass);
    }

    @Override
    public boolean isApplicable() {
        return annotation != null
                && !annotation.source().isAssignableFrom(Void.class)
                && annotation.method().isEmpty();
    }

    private Object[] fillResultWithAllParamProviderMethods(Class<?> sourceClass) {
        if (sourceClass.isEnum()) {
            return sourceClass.getEnumConstants();
        }

        List<Object> result = getParamsFromSourceHierarchy(sourceClass);
        if (result.isEmpty())
            throw new RuntimeException(
                    "No methods starting with provide or they return no result in the parameters source class: "
                            + sourceClass.getName());

        return result.toArray();
    }

    private List<Object> getParamsFromSourceHierarchy(Class<?> sourceClass) {
        List<Object> result = new ArrayList<Object>();
        while (sourceClass.getSuperclass() != null) {
            result.addAll(gatherParamsFromAllMethodsFrom(sourceClass));
            sourceClass = sourceClass.getSuperclass();
        }

        return result;
    }

    private List<Object> gatherParamsFromAllMethodsFrom(Class<?> sourceClass) {
        List<Object> result = new ArrayList<Object>();
        Method[] methods = sourceClass.getDeclaredMethods();
        for (Method prividerMethod : methods) {
            if (prividerMethod.getName().startsWith("provide")) {
                if (!Modifier.isStatic(prividerMethod.getModifiers())) {
                    throw new RuntimeException("Parameters source method " +
                            prividerMethod.getName() +
                            " is not declared as static. Change it to a static method.");
                }
                try {
                    result.addAll(
                            Arrays.asList(paramsFromMethodCommon.getDataFromMethod(prividerMethod)));
                } catch (Exception e) {
                    throw new RuntimeException("Cannot invoke parameters source method: " + prividerMethod.getName(),
                            e);
                }
            }
        }
        return result;
    }
}