package junitparams.internal.parameters;

import junitparams.Parameters;
import org.junit.runners.model.FrameworkMethod;

class ParametersFromExternalClassMethod implements ParametrizationStrategy {
    private ParamsFromMethodCommon paramsFromMethodCommon;
    private Parameters annotation;

    ParametersFromExternalClassMethod(FrameworkMethod frameworkMethod) {
        this.paramsFromMethodCommon = new ParamsFromMethodCommon(frameworkMethod);
        annotation = frameworkMethod.getAnnotation(Parameters.class);
    }

    @Override
    public Object[] getParameters() {
        Class<?> sourceClass = annotation.source();
        return paramsFromMethodCommon.paramsFromMethod(sourceClass);
    }

    @Override
    public boolean isApplicable() {
        return annotation != null
                && !annotation.source().isAssignableFrom(Void.class)
                && !annotation.method().isEmpty();
    }
}