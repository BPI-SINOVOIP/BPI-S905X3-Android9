package junitparams.internal;

import java.lang.annotation.Annotation;
import java.lang.reflect.Array;
import java.math.BigDecimal;

import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.Statement;

import junitparams.converters.ConversionFailedException;
import junitparams.converters.ConvertParam;
import junitparams.converters.ParamAnnotation;
import junitparams.converters.ParamConverter;

/**
 * JUnit invoker for parameterised test methods
 *
 * @author Pawel Lipinski
 */
public class InvokeParameterisedMethod extends Statement {

    private final Object[] params;
    private final FrameworkMethod testMethod;
    private final Object testClass;

    public InvokeParameterisedMethod(FrameworkMethod testMethod, Object testClass, Object params) {
        this.testMethod = testMethod;
        this.testClass = testClass;
        try {
            if (params instanceof String)
                this.params = castParamsFromString((String) params);
            else {
                this.params = castParamsFromObjects(params);
            }
        } catch (ConversionFailedException e) {
            throw new RuntimeException(e);
        }
    }

    private Object[] castParamsFromString(String params) throws ConversionFailedException {
        Object[] columns = null;
        try {
            columns = Utils.splitAtCommaOrPipe(params);
            columns = castParamsUsingConverters(columns);
        } catch (RuntimeException e) {
            new IllegalArgumentException("Cannot parse parameters. Did you use ',' or '|' as column separator? "
                    + params, e).printStackTrace();
        }

        return columns;
    }

    private Object[] castParamsFromObjects(Object params) throws ConversionFailedException {
        Object[] paramset = Utils.safelyCastParamsToArray(params);

        try {
            return castParamsUsingConverters(paramset);
        } catch (ConversionFailedException e) {
            throw e;
        } catch (Exception e) {
            Class<?>[] typesOfParameters = createArrayOfTypesOf(paramset);
            Object resultParam = createObjectOfExpectedTypeBasedOnParams(paramset, typesOfParameters);
            return new Object[]{resultParam};
        }
    }

    private Object createObjectOfExpectedTypeBasedOnParams(Object[] paramset, Class<?>[] typesOfParameters) {
        Object resultParam;

        try {
            if (testMethod.getMethod().getParameterTypes()[0].isArray()) {
                resultParam = Array.newInstance(typesOfParameters[0], paramset.length);
                for (int i = 0; i < paramset.length; i++) {
                    ((Object[]) resultParam)[i] = paramset[i];
                }
            } else {
                resultParam = testMethod.getMethod().getParameterTypes()[0].getConstructor(typesOfParameters).newInstance(paramset);
            }
        } catch (Exception e) {
            throw new IllegalStateException("While trying to create object of class " + testMethod.getMethod().getParameterTypes()[0]
                    + " could not find constructor with arguments matching (type-wise) the ones given in parameters.", e);
        }
        return resultParam;
    }

    private Class<?>[] createArrayOfTypesOf(Object[] paramset) {
        Class<?>[] parametersBasedOnValues = new Class<?>[paramset.length];
        for (int i = 0; i < paramset.length; i++) {
            parametersBasedOnValues[i] = paramset[i].getClass();
        }
        return parametersBasedOnValues;
    }

    private Object[] castParamsUsingConverters(Object[] columns) throws ConversionFailedException {
        Class<?>[] expectedParameterTypes = testMethod.getMethod().getParameterTypes();

        if (testMethodParamsHasVarargs(columns, expectedParameterTypes)) {
            columns = columnsWithVarargs(columns, expectedParameterTypes);
        }

        Annotation[][] parameterAnnotations = testMethod.getMethod().getParameterAnnotations();
        verifySameSizeOfArrays(columns, expectedParameterTypes);
        columns = castAllParametersToProperTypes(columns, expectedParameterTypes, parameterAnnotations);
        return columns;
    }

    private Object[] columnsWithVarargs(Object[] columns, Class<?>[] expectedParameterTypes) {
        Object[] allParameters = standardParameters(columns, expectedParameterTypes);
        allParameters[allParameters.length - 1] = varargsParameters(columns, expectedParameterTypes);
        return allParameters;
    }

    private Object[] varargsParameters(Object[] columns, Class<?>[] expectedParameterTypes) {
        Class<?> varArgType = expectedParameterTypes[expectedParameterTypes.length - 1].getComponentType();
        Object[] varArgsParameters = (Object[]) Array.newInstance(varArgType, columns.length - expectedParameterTypes.length + 1);
        for (int i = 0; i < varArgsParameters.length; i++) {
            varArgsParameters[i] = columns[i + expectedParameterTypes.length - 1];
        }
        return varArgsParameters;
    }

    private Object[] standardParameters(Object[] columns, Class<?>[] expectedParameterTypes) {
        Object[] standardParameters = new Object[expectedParameterTypes.length];
        for (int i = 0; i < standardParameters.length - 1; i++) {
            standardParameters[i] = columns[i];
        }
        return standardParameters;
    }

    private boolean testMethodParamsHasVarargs(Object[] columns, Class<?>[] expectedParameterTypes) {
        int last = expectedParameterTypes.length - 1;
        if (columns[last] == null) {
            return false;
        }
        return expectedParameterTypes.length <= columns.length
                && expectedParameterTypes[last].isArray()
                && expectedParameterTypes[last].getComponentType().equals(columns[last].getClass());
    }

    private Object[] castAllParametersToProperTypes(Object[] columns, Class<?>[] expectedParameterTypes,
                                                    Annotation[][] parameterAnnotations) throws ConversionFailedException {
        Object[] result = new Object[columns.length];

        for (int i = 0; i < columns.length; i++) {
            if (parameterAnnotations[i].length == 0)
                result[i] = castParameterDirectly(columns[i], expectedParameterTypes[i]);
            else
                result[i] = castParameterUsingConverter(columns[i], parameterAnnotations[i]);
        }

        return result;
    }

    private Object castParameterUsingConverter(Object param, Annotation[] annotations) throws ConversionFailedException {
        for (Annotation annotation : annotations) {
            if (ParamAnnotation.matches(annotation)) {
                return ParamAnnotation.convert(annotation, param);
            }
            if (annotation.annotationType().isAssignableFrom(ConvertParam.class)) {
                Class<? extends ParamConverter<?>> converterClass = ((ConvertParam) annotation).value();
                String options = ((ConvertParam) annotation).options();
                try {
                    return converterClass.newInstance().convert(param, options);
                } catch (ConversionFailedException e) {
                    throw e;
                } catch (Exception e) {
                    throw new RuntimeException("Your ParamConverter class must have a public no-arg constructor!", e);
                }
            }
        }
        return param;
    }

    @SuppressWarnings("unchecked")
    private Object castParameterDirectly(Object object, Class clazz) {
        if (object == null || clazz.isInstance(object) || (!(object instanceof String) && clazz.isPrimitive()))
            return object;
        if (clazz.isEnum())
            return (Enum.valueOf(clazz, (String) object));
        if (clazz.isAssignableFrom(String.class))
            return object.toString();
        if (clazz.isAssignableFrom(Class.class))
            try {
                return Class.forName((String) object);
            } catch (ClassNotFoundException e) {
                throw new IllegalArgumentException("Parameter class (" + object + ") not found", e);
            }
        if (clazz.isAssignableFrom(Integer.TYPE) || clazz.isAssignableFrom(Integer.class))
            return Integer.parseInt((String) object);
        if (clazz.isAssignableFrom(Short.TYPE) || clazz.isAssignableFrom(Short.class))
            return Short.parseShort((String) object);
        if (clazz.isAssignableFrom(Long.TYPE) || clazz.isAssignableFrom(Long.class))
            return Long.parseLong((String) object);
        if (clazz.isAssignableFrom(Float.TYPE) || clazz.isAssignableFrom(Float.class))
            return Float.parseFloat((String) object);
        if (clazz.isAssignableFrom(Double.TYPE) || clazz.isAssignableFrom(Double.class))
            return Double.parseDouble((String) object);
        if (clazz.isAssignableFrom(Boolean.TYPE) || clazz.isAssignableFrom(Boolean.class))
            return Boolean.parseBoolean((String) object);
        if (clazz.isAssignableFrom(Character.TYPE) || clazz.isAssignableFrom(Character.class))
            return object.toString().charAt(0);
        if (clazz.isAssignableFrom(Byte.TYPE) || clazz.isAssignableFrom(Byte.class))
            return Byte.parseByte((String) object);
        if (clazz.isAssignableFrom(BigDecimal.class))
            return new BigDecimal((String) object);
        throw new IllegalArgumentException("Parameter type (" + clazz.getName() + ") cannot be handled!" +
                " Only primitive types, BigDecimals and Strings can be used.");
    }

    private void verifySameSizeOfArrays(Object[] columns, Class<?>[] parameterTypes) {
        if (parameterTypes.length != columns.length)
            throw new IllegalArgumentException(
                    "Number of parameters inside @Parameters annotation doesn't match the number of test method parameters.\nThere are "
                            + columns.length + " parameters in annotation, while there's " + parameterTypes.length + " parameters in the "
                            + testMethod.getName() + " method.");
    }

    @Override
    public void evaluate() throws Throwable {
        testMethod.invokeExplosively(testClass, params == null ? new Object[]{params} : params);
    }

}
