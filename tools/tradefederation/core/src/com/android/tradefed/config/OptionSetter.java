/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.tradefed.config;

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.MultiMap;
import com.android.tradefed.util.TimeVal;
import com.android.tradefed.util.keystore.IKeyStoreClient;

import com.google.common.base.Objects;

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Pattern;

/**
 * Populates {@link Option} fields.
 * <p/>
 * Setting of numeric fields such byte, short, int, long, float, and double fields is supported.
 * This includes both unboxed and boxed versions (e.g. int vs Integer). If there is a problem
 * setting the argument to match the desired type, a {@link ConfigurationException} is thrown.
 * <p/>
 * File option fields are supported by simply wrapping the string argument in a File object without
 * testing for the existence of the file.
 * <p/>
 * Parameterized Collection fields such as List&lt;File&gt; and Set&lt;String&gt; are supported as
 * long as the parameter type is otherwise supported by the option setter. The collection field
 * should be initialized with an appropriate collection instance.
 * <p/>
 * All fields will be processed, including public, protected, default (package) access, private and
 * inherited fields.
 * <p/>
 *
 * ported from dalvik.runner.OptionParser
 * @see ArgsOptionParser
 */
@SuppressWarnings("rawtypes")
public class OptionSetter {

    static final String BOOL_FALSE_PREFIX = "no-";
    private static final HashMap<Class<?>, Handler> handlers = new HashMap<Class<?>, Handler>();
    static final char NAMESPACE_SEPARATOR = ':';
    static final Pattern USE_KEYSTORE_REGEX = Pattern.compile("USE_KEYSTORE@(.*)");
    private IKeyStoreClient mKeyStoreClient = null;

    static {
        handlers.put(boolean.class, new BooleanHandler());
        handlers.put(Boolean.class, new BooleanHandler());

        handlers.put(byte.class, new ByteHandler());
        handlers.put(Byte.class, new ByteHandler());
        handlers.put(short.class, new ShortHandler());
        handlers.put(Short.class, new ShortHandler());
        handlers.put(int.class, new IntegerHandler());
        handlers.put(Integer.class, new IntegerHandler());
        handlers.put(long.class, new LongHandler());
        handlers.put(Long.class, new LongHandler());

        handlers.put(float.class, new FloatHandler());
        handlers.put(Float.class, new FloatHandler());
        handlers.put(double.class, new DoubleHandler());
        handlers.put(Double.class, new DoubleHandler());

        handlers.put(String.class, new StringHandler());
        handlers.put(File.class, new FileHandler());
        handlers.put(TimeVal.class, new TimeValHandler());
    }


    static class FieldDef {
        Object object;
        Field field;
        Object key;

        FieldDef(Object object, Field field, Object key) {
            this.object = object;
            this.field = field;
            this.key = key;
        }

        @Override
        public boolean equals(Object obj) {
            if (obj == this) {
                return true;
            }

            if (obj instanceof FieldDef) {
                FieldDef other = (FieldDef)obj;
                return Objects.equal(this.object, other.object) &&
                        Objects.equal(this.field, other.field) &&
                        Objects.equal(this.key, other.key);
            }

            return false;
        }

        @Override
        public int hashCode() {
            return Objects.hashCode(object, field, key);
        }
    }


    private static Handler getHandler(Type type) throws ConfigurationException {
        if (type instanceof ParameterizedType) {
            ParameterizedType parameterizedType = (ParameterizedType) type;
            Class<?> rawClass = (Class<?>) parameterizedType.getRawType();
            if (Collection.class.isAssignableFrom(rawClass)) {
                // handle Collection
                Type actualType = parameterizedType.getActualTypeArguments()[0];
                if (!(actualType instanceof Class)) {
                    throw new ConfigurationException(
                            "cannot handle nested parameterized type " + type);
                }
                return getHandler(actualType);
            } else if (Map.class.isAssignableFrom(rawClass) ||
                    MultiMap.class.isAssignableFrom(rawClass)) {
                // handle Map
                Type keyType = parameterizedType.getActualTypeArguments()[0];
                Type valueType = parameterizedType.getActualTypeArguments()[1];
                if (!(keyType instanceof Class)) {
                    throw new ConfigurationException(
                            "cannot handle nested parameterized type " + keyType);
                } else if (!(valueType instanceof Class)) {
                    throw new ConfigurationException(
                            "cannot handle nested parameterized type " + valueType);
                }

                return new MapHandler(getHandler(keyType), getHandler(valueType));
            } else {
                throw new ConfigurationException(String.format(
                        "can't handle parameterized type %s; only Collection, Map, and MultiMap "
                        + "are supported", type));
            }
        }
        if (type instanceof Class) {
            Class<?> cType = (Class<?>) type;

            if (cType.isEnum()) {
                return new EnumHandler(cType);
            } else if (Collection.class.isAssignableFrom(cType)) {
                // could handle by just having a default of treating
                // contents as String but consciously decided this
                // should be an error
                throw new ConfigurationException(String.format(
                        "Cannot handle non-parameterized collection %s.  Use a generic Collection "
                        + "to specify a desired element type.", type));
            } else if (Map.class.isAssignableFrom(cType)) {
                // could handle by just having a default of treating
                // contents as String but consciously decided this
                // should be an error
                throw new ConfigurationException(String.format(
                        "Cannot handle non-parameterized map %s.  Use a generic Map to specify "
                        + "desired element types.", type));
            } else if (MultiMap.class.isAssignableFrom(cType)) {
                // could handle by just having a default of treating
                // contents as String but consciously decided this
                // should be an error
                throw new ConfigurationException(String.format(
                        "Cannot handle non-parameterized multimap %s.  Use a generic MultiMap to "
                        + "specify desired element types.", type));
            }
            return handlers.get(cType);
        }
        throw new ConfigurationException(String.format("cannot handle unknown field type %s",
                type));
    }

    /**
     * Does some magic to distinguish TimeVal long field from normal long fields, then calls
     * {@link #getHandler(Type)} in the appropriate manner.
     */
    private Handler getHandlerOrTimeVal(Field field, Object optionSource)
            throws ConfigurationException {
        // Do some magic to distinguish TimeVal long fields from normal long fields
        final Option option = field.getAnnotation(Option.class);
        if (option == null) {
            // Shouldn't happen, but better to check.
            throw new ConfigurationException(String.format(
                    "internal error: @Option annotation for field %s in class %s was " +
                    "unexpectedly null",
                    field.getName(), optionSource.getClass().getName()));
        }

        final Type type = field.getGenericType();
        if (option.isTimeVal()) {
            // We've got a field that marks itself as a time value.  First off, verify that it's
            // a compatible type
            if (type instanceof Class) {
                final Class<?> cType = (Class<?>) type;
                if (long.class.equals(cType) || Long.class.equals(cType)) {
                    // Parse time value and return a Long
                    return new TimeValLongHandler();

                } else if (TimeVal.class.equals(cType)) {
                    // Parse time value and return a TimeVal object
                    return new TimeValHandler();
                }
            }

            throw new ConfigurationException(String.format("Only fields of type long, " +
                    "Long, or TimeVal may be declared as isTimeVal.  Field %s has " +
                    "incompatible type %s.", field.getName(), field.getGenericType()));

        } else {
            // Note that fields declared as TimeVal (or Generic types with TimeVal parameters) will
            // follow this branch, but will still work as expected.
            return getHandler(type);
        }
    }


    private final Collection<Object> mOptionSources;
    private final Map<String, OptionFieldsForName> mOptionMap;

    /**
     * Container for the list of option fields with given name.
     * <p/>
     * Used to enforce constraint that fields with same name can exist in different option sources,
     * but not the same option source
     */
    private class OptionFieldsForName implements Iterable<Map.Entry<Object, Field>> {

        private Map<Object, Field> mSourceFieldMap = new HashMap<Object, Field>();

        void addField(String name, Object source, Field field) throws ConfigurationException {
            if (size() > 0) {
                Handler existingFieldHandler = getHandler(getFirstField().getGenericType());
                Handler newFieldHandler = getHandler(field.getGenericType());
                if (existingFieldHandler == null || newFieldHandler == null ||
                        !existingFieldHandler.getClass().equals(newFieldHandler.getClass())) {
                    throw new ConfigurationException(String.format(
                            "@Option field with name '%s' in class '%s' is defined with a " +
                            "different type than same option in class '%s'",
                            name, source.getClass().getName(),
                            getFirstObject().getClass().getName()));
                }
            }
            if (mSourceFieldMap.put(source, field) != null) {
                throw new ConfigurationException(String.format(
                        "@Option field with name '%s' is defined more than once in class '%s'",
                        name, source.getClass().getName()));
            }
        }

        public int size() {
            return mSourceFieldMap.size();
        }

        public Field getFirstField() throws ConfigurationException {
            if (size() <= 0) {
                // should never happen
                throw new ConfigurationException("no option fields found");
            }
            return mSourceFieldMap.values().iterator().next();
        }

        public Object getFirstObject() throws ConfigurationException {
            if (size() <= 0) {
                // should never happen
                throw new ConfigurationException("no option fields found");
            }
            return mSourceFieldMap.keySet().iterator().next();
        }

        @Override
        public Iterator<Map.Entry<Object, Field>> iterator() {
            return mSourceFieldMap.entrySet().iterator();
        }
    }

    /**
     * Constructs a new OptionParser for setting the @Option fields of 'optionSources'.
     * @throws ConfigurationException
     */
    public OptionSetter(Object... optionSources) throws ConfigurationException {
        this(Arrays.asList(optionSources));
    }

    /**
     * Constructs a new OptionParser for setting the @Option fields of 'optionSources'.
     * @throws ConfigurationException
     */
    public OptionSetter(Collection<Object> optionSources) throws ConfigurationException {
        mOptionSources = optionSources;
        mOptionMap = makeOptionMap();
    }

    public void setKeyStore(IKeyStoreClient keyStore) {
        mKeyStoreClient = keyStore;
    }

    public IKeyStoreClient getKeyStore() {
        return mKeyStoreClient;
    }

    private OptionFieldsForName fieldsForArg(String name) throws ConfigurationException {
        OptionFieldsForName fields = mOptionMap.get(name);
        if (fields == null || fields.size() == 0) {
            throw new ConfigurationException(String.format("Could not find option with name %s",
                    name));
        }
        return fields;
    }

    /**
     * Returns a string describing the type of the field with given name.
     *
     * @param name the {@link Option} field name
     * @return a {@link String} describing the field's type
     * @throws ConfigurationException if field could not be found
     */
    public String getTypeForOption(String name) throws ConfigurationException {
        return fieldsForArg(name).getFirstField().getType().getSimpleName().toLowerCase();
    }

    /**
     * Sets the value for a non-map option.
     *
     * @param optionName the name of Option to set
     * @param valueText the value
     * @return A list of {@link FieldDef}s corresponding to each object field that was modified.
     * @throws ConfigurationException if Option cannot be found or valueText is wrong type
     */
    public List<FieldDef> setOptionValue(String optionName, String valueText)
            throws ConfigurationException {
        return setOptionValue(optionName, null, valueText);
    }

    /**
     * Sets the value for an option.
     *
     * @param optionName the name of Option to set
     * @param keyText the key for Map options, or null.
     * @param valueText the value
     * @return A list of {@link FieldDef}s corresponding to each object field that was modified.
     * @throws ConfigurationException if Option cannot be found or valueText is wrong type
     */
    public List<FieldDef> setOptionValue(String optionName, String keyText, String valueText)
            throws ConfigurationException {

        List<FieldDef> ret = new ArrayList<>();

        // For each of the applicable object fields
        final OptionFieldsForName optionFields = fieldsForArg(optionName);
        for (Map.Entry<Object, Field> fieldEntry : optionFields) {

            // Retrieve an appropriate handler for this field's type
            final Object optionSource = fieldEntry.getKey();
            final Field field = fieldEntry.getValue();
            final Handler handler = getHandlerOrTimeVal(field, optionSource);

            // Translate the string value to the actual type of the field
            Object value = handler.translate(valueText);
            if (value == null) {
                String type = field.getType().getSimpleName();
                if (handler.isMap()) {
                    ParameterizedType pType = (ParameterizedType) field.getGenericType();
                    Type valueType = pType.getActualTypeArguments()[1];
                    type = ((Class<?>)valueType).getSimpleName().toLowerCase();
                }
                throw new ConfigurationException(String.format(
                        "Couldn't convert value '%s' to a %s for option '%s'", valueText, type,
                        optionName));
            }

            // For maps, also translate the key value
            Object key = null;
            if (handler.isMap()) {
                key = ((MapHandler)handler).translateKey(keyText);
                if (key == null) {
                    ParameterizedType pType = (ParameterizedType) field.getGenericType();
                    Type keyType = pType.getActualTypeArguments()[0];
                    String type = ((Class<?>)keyType).getSimpleName().toLowerCase();
                    throw new ConfigurationException(String.format(
                            "Couldn't convert key '%s' to a %s for option '%s'", keyText, type,
                            optionName));
                }
            }

            // Actually set the field value
            if (setFieldValue(optionName, optionSource, field, key, value)) {
                ret.add(new FieldDef(optionSource, field, key));
            }
        }

        return ret;
    }


    /**
     * Sets the given {@link Option} field's value.
     *
     * @param optionName the name specified in {@link Option}
     * @param optionSource the {@link Object} to set
     * @param field the {@link Field}
     * @param key the key to an entry in a {@link Map} or {@link MultiMap} field or null.
     * @param value the value to set
     * @return Whether the field was set.
     * @throws ConfigurationException
     * @see OptionUpdateRule
     */
    @SuppressWarnings("unchecked")
    static boolean setFieldValue(String optionName, Object optionSource, Field field, Object key,
            Object value) throws ConfigurationException {

        boolean fieldWasSet = true;

        try {
            field.setAccessible(true);

            if (Collection.class.isAssignableFrom(field.getType())) {
                if (key != null) {
                    throw new ConfigurationException(String.format(
                            "key not applicable for Collection field '%s'", field.getName()));
                }
                Collection collection = (Collection)field.get(optionSource);
                if (collection == null) {
                    throw new ConfigurationException(String.format(
                            "Unable to add value to field '%s'. Field is null.", field.getName()));
                }
                ParameterizedType pType = (ParameterizedType) field.getGenericType();
                Type fieldType = pType.getActualTypeArguments()[0];
                if (value instanceof Collection) {
                    collection.addAll((Collection)value);
                } else if (!((Class<?>) fieldType).isInstance(value)) {
                    // Ensure that the value being copied is of the right type for the collection.
                    throw new ConfigurationException(
                            String.format(
                                    "Value '%s' is not of type '%s' like the Collection.",
                                    value, fieldType));
                } else {
                    collection.add(value);
                }
            } else if (Map.class.isAssignableFrom(field.getType())) {
                // TODO: check if type of the value can be added safely to the Map.
                Map map = (Map) field.get(optionSource);
                if (map == null) {
                    throw new ConfigurationException(String.format(
                            "Unable to add value to field '%s'. Field is null.", field.getName()));
                }
                if (value instanceof Map) {
                    if (key != null) {
                        throw new ConfigurationException(String.format(
                                "Key not applicable when setting Map field '%s' from map value",
                                field.getName()));
                    }
                    map.putAll((Map)value);
                } else {
                    if (key == null) {
                        throw new ConfigurationException(String.format(
                                "Unable to add value to map field '%s'. Key is null.",
                                field.getName()));
                    }
                    map.put(key, value);
                }
            } else if (MultiMap.class.isAssignableFrom(field.getType())) {
                // TODO: see if we can combine this with Map logic above
                MultiMap map = (MultiMap)field.get(optionSource);
                if (map == null) {
                    throw new ConfigurationException(String.format(
                            "Unable to add value to field '%s'. Field is null.", field.getName()));
                }
                if (value instanceof MultiMap) {
                    if (key != null) {
                        throw new ConfigurationException(String.format(
                                "Key not applicable when setting Map field '%s' from map value",
                                field.getName()));
                    }
                    map.putAll((MultiMap)value);
                } else {
                    if (key == null) {
                        throw new ConfigurationException(String.format(
                                "Unable to add value to map field '%s'. Key is null.",
                                field.getName()));
                    }
                    map.put(key, value);
                }
            } else {
                if (key != null) {
                    throw new ConfigurationException(String.format(
                            "Key not applicable when setting non-map field '%s'", field.getName()));
                }
                final Option option = field.getAnnotation(Option.class);
                if (option == null) {
                    // By virtue of us having gotten here, this should never happen.  But better
                    // safe than sorry
                    throw new ConfigurationException(String.format(
                            "internal error: @Option annotation for field %s in class %s was " +
                            "unexpectedly null",
                            field.getName(), optionSource.getClass().getName()));
                }
                OptionUpdateRule rule = option.updateRule();
                if (rule.shouldUpdate(optionName, optionSource, field, value)) {
                    field.set(optionSource, value);
                } else {
                    fieldWasSet = false;
                }
            }
        } catch (IllegalAccessException | IllegalArgumentException e) {
            throw new ConfigurationException(String.format(
                    "internal error when setting option '%s'", optionName), e);

        }

        return fieldWasSet;
    }


    /**
     * Sets the given {@link Option} fields value.
     *
     * @param optionName the name specified in {@link Option}
     * @param optionSource the {@link Object} to set
     * @param field the {@link Field}
     * @param value the value to set
     * @throws ConfigurationException
     */
    static void setFieldValue(String optionName, Object optionSource, Field field, Object value)
            throws ConfigurationException {

        setFieldValue(optionName, optionSource, field, null, value);
    }

    /**
     * Cache the available options and report any problems with the options themselves right away.
     *
     * @return a {@link Map} of {@link Option} field name to {@link OptionFieldsForName}s
     * @throws ConfigurationException if any {@link Option} are incorrectly specified
     */
    private Map<String, OptionFieldsForName> makeOptionMap() throws ConfigurationException {
        final Map<String, Integer> freqMap = new HashMap<String, Integer>(mOptionSources.size());
        final Map<String, OptionFieldsForName> optionMap =
                new HashMap<String, OptionFieldsForName>();
        for (Object objectSource : mOptionSources) {
            final String className = objectSource.getClass().getName();

            // Keep track of how many times we've seen this className.  This assumes that we
            // maintain the optionSources in a universally-knowable order internally (which we do --
            // they remain in the order in which they were passed to the constructor).  Thus, the
            // index can serve as a unique identifier for each instance of className as long as
            // other upstream classes use the same 1-based ordered numbering scheme.
            Integer index = freqMap.get(className);
            index = index == null ? 1 : index + 1;
            freqMap.put(className, index);
            addOptionsForObject(objectSource, optionMap, index, null);

            if (objectSource instanceof IDeviceConfiguration) {
                for (Object deviceObject : ((IDeviceConfiguration)objectSource).getAllObjects()) {
                    index = freqMap.get(deviceObject.getClass().getName());
                    index = index == null ? 1 : index + 1;
                    freqMap.put(deviceObject.getClass().getName(), index);
                    Integer tracked =
                            ((IDeviceConfiguration) objectSource).getFrequency(deviceObject);
                    if (tracked != null && !index.equals(tracked)) {
                        index = tracked;
                    }
                    addOptionsForObject(deviceObject, optionMap, index,
                            ((IDeviceConfiguration)objectSource).getDeviceName());
                }
            }
        }
        return optionMap;
    }

    /**
     * Adds all option fields (both declared and inherited) to the <var>optionMap</var> for
     * provided <var>optionClass</var>.
     * <p>
     * Also adds option fields with all the alias namespaced from the class they are found in, and
     * their child classes.
     * <p>
     * For example:
     * if class1(@alias1) extends class2(@alias2), all the option from class2 will be available
     * with the alias1 and alias2. All the option from class1 are available with alias1 only.
     *
     * @param optionSource
     * @param optionMap
     * @param index The unique index of this instance of the optionSource class.  Should equal the
     *              number of instances of this class that we've already seen, plus 1.
     * @param deviceName the Configuration Device Name that this attributes belong to. can be null.
     * @throws ConfigurationException
     */
    private void addOptionsForObject(Object optionSource,
            Map<String, OptionFieldsForName> optionMap, Integer index, String deviceName)
            throws ConfigurationException {
        Collection<Field> optionFields = getOptionFieldsForClass(optionSource.getClass());
        for (Field field : optionFields) {
            final Option option = field.getAnnotation(Option.class);
            if (option.name().indexOf(NAMESPACE_SEPARATOR) != -1) {
                throw new ConfigurationException(String.format(
                        "Option name '%s' in class '%s' is invalid. " +
                        "Option names cannot contain the namespace separator character '%c'",
                        option.name(), optionSource.getClass().getName(), NAMESPACE_SEPARATOR));
            }

            // Make sure the source doesn't use GREATEST or LEAST for a non-Comparable field.
            final Type type = field.getGenericType();
            if ((type instanceof Class) && !(type instanceof ParameterizedType)) {
                // Not a parameterized type
                if ((option.updateRule() == OptionUpdateRule.GREATEST) ||
                        (option.updateRule() == OptionUpdateRule.LEAST)) {
                    Class cType = (Class) type;
                    if (!(Comparable.class.isAssignableFrom(cType))) {
                        throw new ConfigurationException(String.format(
                                "Option '%s' in class '%s' attempts to use updateRule %s with " +
                                "non-Comparable type '%s'.", option.name(),
                                optionSource.getClass().getName(), option.updateRule(),
                                field.getGenericType()));
                    }
                }

                // don't allow 'final' for non-Collections
                if ((field.getModifiers() & Modifier.FINAL) != 0) {
                    throw new ConfigurationException(String.format(
                            "Option '%s' in class '%s' is final and cannot be set", option.name(),
                            optionSource.getClass().getName()));
                }
            }

            // Allow classes to opt out of the global Option namespace
            boolean addToGlobalNamespace = true;
            if (optionSource.getClass().isAnnotationPresent(OptionClass.class)) {
                final OptionClass classAnnotation = optionSource.getClass().getAnnotation(
                        OptionClass.class);
                addToGlobalNamespace = classAnnotation.global_namespace();
            }

            if (addToGlobalNamespace) {
                addNameToMap(optionMap, optionSource, option.name(), field);
                if (deviceName != null) {
                    addNameToMap(optionMap, optionSource,
                            String.format("{%s}%s", deviceName, option.name()), field);
                }
            }
            addNamespacedOptionToMap(optionMap, optionSource, option.name(), field, index,
                    deviceName);
            if (option.shortName() != Option.NO_SHORT_NAME) {
                if (addToGlobalNamespace) {
                    // Note that shortName is not supported with device specified, full name needs
                    // to be use
                    addNameToMap(optionMap, optionSource, String.valueOf(option.shortName()),
                            field);
                }
                addNamespacedOptionToMap(optionMap, optionSource,
                        String.valueOf(option.shortName()), field, index, deviceName);
            }
            if (isBooleanField(field)) {
                // add the corresponding "no" option to make boolean false
                if (addToGlobalNamespace) {
                    addNameToMap(optionMap, optionSource, BOOL_FALSE_PREFIX + option.name(), field);
                    if (deviceName != null) {
                        addNameToMap(optionMap, optionSource, String.format("{%s}%s", deviceName,
                                        BOOL_FALSE_PREFIX + option.name()), field);
                    }
                }
                addNamespacedOptionToMap(optionMap, optionSource, BOOL_FALSE_PREFIX + option.name(),
                        field, index, deviceName);
            }
        }
    }

    /**
     * Returns the names of all of the {@link Option}s that are marked as {@code mandatory} but
     * remain unset.
     *
     * @return A {@link Collection} of {@link String}s containing the (unqualified) names of unset
     *         mandatory options.
     * @throws ConfigurationException if a field to be checked is inaccessible
     */
    protected Collection<String> getUnsetMandatoryOptions() throws ConfigurationException {
        Collection<String> unsetOptions = new HashSet<String>();
        for (Map.Entry<String, OptionFieldsForName> optionPair : mOptionMap.entrySet()) {
            final String optName = optionPair.getKey();
            final OptionFieldsForName optionFields = optionPair.getValue();
            if (optName.indexOf(NAMESPACE_SEPARATOR) >= 0) {
                // Only return unqualified option names
                continue;
            }

            for (Map.Entry<Object, Field> fieldEntry : optionFields) {
                final Object obj = fieldEntry.getKey();
                final Field field = fieldEntry.getValue();
                final Option option = field.getAnnotation(Option.class);
                if (option == null) {
                    continue;
                } else if (!option.mandatory()) {
                    continue;
                }

                // At this point, we know this is a mandatory field; make sure it's set
                field.setAccessible(true);
                final Object value;
                try {
                    value = field.get(obj);
                } catch (IllegalAccessException e) {
                    throw new ConfigurationException(String.format("internal error: %s",
                            e.getMessage()));
                }

                final String realOptName = String.format("--%s", option.name());
                if (value == null) {
                    unsetOptions.add(realOptName);
                } else if (value instanceof Collection) {
                    Collection c = (Collection) value;
                    if (c.isEmpty()) {
                        unsetOptions.add(realOptName);
                    }
                } else if (value instanceof Map) {
                    Map m = (Map) value;
                    if (m.isEmpty()) {
                        unsetOptions.add(realOptName);
                    }
                } else if (value instanceof MultiMap) {
                    MultiMap m = (MultiMap) value;
                    if (m.isEmpty()) {
                        unsetOptions.add(realOptName);
                    }
                }
            }
        }
        return unsetOptions;
    }

    /**
     * Gets a list of all {@link Option} fields (both declared and inherited) for given class.
     *
     * @param optionClass the {@link Class} to search
     * @return a {@link Collection} of fields annotated with {@link Option}
     */
    static Collection<Field> getOptionFieldsForClass(final Class<?> optionClass) {
        Collection<Field> fieldList = new ArrayList<Field>();
        buildOptionFieldsForClass(optionClass, fieldList);
        return fieldList;
    }

    /**
     * Recursive method that adds all option fields (both declared and inherited) to the
     * <var>optionFields</var> for provided <var>optionClass</var>
     *
     * @param optionClass
     * @param optionFields
     */
    private static void buildOptionFieldsForClass(final Class<?> optionClass,
            Collection<Field> optionFields) {
        for (Field field : optionClass.getDeclaredFields()) {
            if (field.isAnnotationPresent(Option.class)) {
                optionFields.add(field);
            }
        }
        Class<?> superClass = optionClass.getSuperclass();
        if (superClass != null) {
            buildOptionFieldsForClass(superClass, optionFields);
        }
    }

    /**
     * Return the given {@link Field}'s value as a {@link String}.
     *
     * @param field the {@link Field}
     * @param optionObject the {@link Object} to get field's value from.
     * @return the field's value as a {@link String}, or <code>null</code> if field is not set or is
     *         empty (in case of {@link Collection}s
     */
    static String getFieldValueAsString(Field field, Object optionObject) {
        Object fieldValue = getFieldValue(field, optionObject);
        if (fieldValue == null) {
            return null;
        }
        if (fieldValue instanceof Collection) {
            Collection collection = (Collection)fieldValue;
            if (collection.isEmpty()) {
                return null;
            }
        } else if (fieldValue instanceof Map) {
            Map map = (Map)fieldValue;
            if (map.isEmpty()) {
                return null;
            }
        } else if (fieldValue instanceof MultiMap) {
            MultiMap multimap = (MultiMap)fieldValue;
            if (multimap.isEmpty()) {
                return null;
            }
        }
        return fieldValue.toString();
    }

    /**
     * Return the given {@link Field}'s value, handling any exceptions.
     *
     * @param field the {@link Field}
     * @param optionObject the {@link Object} to get field's value from.
     * @return the field's value as a {@link Object}, or <code>null</code>
     */
    static Object getFieldValue(Field field, Object optionObject) {
        try {
            field.setAccessible(true);
            return field.get(optionObject);
        } catch (IllegalArgumentException e) {
            CLog.w("Could not read value for field %s in class %s. Reason: %s", field.getName(),
                    optionObject.getClass().getName(), e);
            return null;
        } catch (IllegalAccessException e) {
            CLog.w("Could not read value for field %s in class %s. Reason: %s", field.getName(),
                    optionObject.getClass().getName(), e);
            return null;
        }
    }

    /**
     * Returns the help text describing the valid values for the Enum field.
     *
     * @param field the {@link Field} to get values for
     * @return the appropriate help text, or an empty {@link String} if the field is not an Enum.
     */
    static String getEnumFieldValuesAsString(Field field) {
        Class<?> type = field.getType();
        Object[] vals = type.getEnumConstants();
        if (vals == null) {
            return "";
        }

        StringBuilder sb = new StringBuilder(" Valid values: [");
        sb.append(ArrayUtil.join(", ", vals));
        sb.append("]");
        return sb.toString();
    }

    public boolean isBooleanOption(String name) throws ConfigurationException {
        Field field = fieldsForArg(name).getFirstField();
        return isBooleanField(field);
    }

    static boolean isBooleanField(Field field) throws ConfigurationException {
        return getHandler(field.getGenericType()).isBoolean();
    }

    public boolean isMapOption(String name) throws ConfigurationException {
        Field field = fieldsForArg(name).getFirstField();
        return isMapField(field);
    }

    static boolean isMapField(Field field) throws ConfigurationException {
        return getHandler(field.getGenericType()).isMap();
    }

    private void addNameToMap(Map<String, OptionFieldsForName> optionMap, Object optionSource,
            String name, Field field) throws ConfigurationException {
        OptionFieldsForName fields = optionMap.get(name);
        if (fields == null) {
            fields = new OptionFieldsForName();
            optionMap.put(name, fields);
        }

        fields.addField(name, optionSource, field);
        if (getHandler(field.getGenericType()) == null) {
            throw new ConfigurationException(String.format(
                    "Option name '%s' in class '%s' is invalid. Unsupported @Option field type "
                    + "'%s'", name, optionSource.getClass().getName(), field.getType()));
        }
    }

    /**
     * Adds the namespaced versions of the option to the map
     *
     * See {@link #makeOptionMap()} for details on the enumeration scheme
     */
    private void addNamespacedOptionToMap(Map<String, OptionFieldsForName> optionMap,
            Object optionSource, String name, Field field, int index, String deviceName)
            throws ConfigurationException {
        final String className = optionSource.getClass().getName();

        if (optionSource.getClass().isAnnotationPresent(OptionClass.class)) {
            final OptionClass classAnnotation = optionSource.getClass().getAnnotation(
                    OptionClass.class);
            addNamespacedAliasOptionToMap(optionMap, optionSource, name, field, index, deviceName,
                    classAnnotation.alias());
        }

        // Allows use of a className-delimited namespace.
        // Example option name: com.fully.qualified.ClassName:option-name
        addNameToMap(optionMap, optionSource, String.format("%s%c%s",
                className, NAMESPACE_SEPARATOR, name), field);

        // Allows use of an enumerated namespace, to enable options to map to specific instances of
        // a className, rather than just to all instances of that particular className.
        // Example option name: com.fully.qualified.ClassName:2:option-name
        addNameToMap(optionMap, optionSource, String.format("%s%c%d%c%s",
                className, NAMESPACE_SEPARATOR, index, NAMESPACE_SEPARATOR, name), field);

        if (deviceName != null) {
            // Example option name: {device1}com.fully.qualified.ClassName:option-name
            addNameToMap(optionMap, optionSource, String.format("{%s}%s%c%s",
                    deviceName, className, NAMESPACE_SEPARATOR, name), field);

            // Allows use of an enumerated namespace, to enable options to map to specific
            // instances of a className inside a device configuration holder,
            // rather than just to all instances of that particular className.
            // Example option name: {device1}com.fully.qualified.ClassName:2:option-name
            addNameToMap(optionMap, optionSource, String.format("{%s}%s%c%d%c%s",
                    deviceName, className, NAMESPACE_SEPARATOR, index, NAMESPACE_SEPARATOR, name),
                    field);
        }
    }

    /**
     * Adds the alias namespaced versions of the option to the map
     *
     * See {@link #makeOptionMap()} for details on the enumeration scheme
     */
    private void addNamespacedAliasOptionToMap(Map<String, OptionFieldsForName> optionMap,
            Object optionSource, String name, Field field, int index, String deviceName,
            String alias) throws ConfigurationException {
        addNameToMap(optionMap, optionSource, String.format("%s%c%s", alias,
                NAMESPACE_SEPARATOR, name), field);

        // Allows use of an enumerated namespace, to enable options to map to specific instances
        // of a class alias, rather than just to all instances of that particular alias.
        // Example option name: alias:2:option-name
        addNameToMap(optionMap, optionSource, String.format("%s%c%d%c%s",
                alias, NAMESPACE_SEPARATOR, index, NAMESPACE_SEPARATOR, name),
                field);

        if (deviceName != null) {
            addNameToMap(optionMap, optionSource, String.format("{%s}%s%c%s", deviceName,
                    alias, NAMESPACE_SEPARATOR, name), field);
            // Allows use of an enumerated namespace, to enable options to map to specific
            // instances of a class alias inside a device configuration holder,
            // rather than just to all instances of that particular alias.
            // Example option name: {device1}alias:2:option-name
            addNameToMap(optionMap, optionSource, String.format("{%s}%s%c%d%c%s",
                    deviceName, alias, NAMESPACE_SEPARATOR, index,
                    NAMESPACE_SEPARATOR, name), field);
        }
    }

    private abstract static class Handler {
        // Only BooleanHandler should ever override this.
        boolean isBoolean() {
            return false;
        }

        // Only MapHandler should ever override this.
        boolean isMap() {
            return false;
        }

        /**
         * Returns an object of appropriate type for the given Handle, corresponding to 'valueText'.
         * Returns null on failure.
         */
        abstract Object translate(String valueText);
    }

    private static class BooleanHandler extends Handler {
        @Override boolean isBoolean() {
            return true;
        }

        @Override
        Object translate(String valueText) {
            if (valueText.equalsIgnoreCase("true") || valueText.equalsIgnoreCase("yes")) {
                return Boolean.TRUE;
            } else if (valueText.equalsIgnoreCase("false") || valueText.equalsIgnoreCase("no")) {
                return Boolean.FALSE;
            }
            return null;
        }
    }

    private static class ByteHandler extends Handler {
        @Override
        Object translate(String valueText) {
            try {
                return Byte.parseByte(valueText);
            } catch (NumberFormatException ex) {
                return null;
            }
        }
    }

    private static class ShortHandler extends Handler {
        @Override
        Object translate(String valueText) {
            try {
                return Short.parseShort(valueText);
            } catch (NumberFormatException ex) {
                return null;
            }
        }
    }

    private static class IntegerHandler extends Handler {
        @Override
        Object translate(String valueText) {
            try {
                return Integer.parseInt(valueText);
            } catch (NumberFormatException ex) {
                return null;
            }
        }
    }

    private static class LongHandler extends Handler {
        @Override
        Object translate(String valueText) {
            try {
                return Long.parseLong(valueText);
            } catch (NumberFormatException ex) {
                return null;
            }
        }
    }

    private static class TimeValLongHandler extends Handler {
        /**
         * We parse the string as a time value, and return a {@code long}
         */
        @Override
        Object translate(String valueText) {
            try {
                return TimeVal.fromString(valueText);

            } catch (NumberFormatException ex) {
                return null;
            }
        }
    }

    private static class TimeValHandler extends Handler {
        /**
         * We parse the string as a time value, and return a {@code TimeVal}
         */
        @Override
        Object translate(String valueText) {
            try {
                return new TimeVal(valueText);

            } catch (NumberFormatException ex) {
                return null;
            }
        }
    }

    private static class FloatHandler extends Handler {
        @Override
        Object translate(String valueText) {
            try {
                return Float.parseFloat(valueText);
            } catch (NumberFormatException ex) {
                return null;
            }
        }
    }

    private static class DoubleHandler extends Handler {
        @Override
        Object translate(String valueText) {
            try {
                return Double.parseDouble(valueText);
            } catch (NumberFormatException ex) {
                return null;
            }
        }
    }

    private static class StringHandler extends Handler {
        @Override
        Object translate(String valueText) {
            return valueText;
        }
    }

    private static class FileHandler extends Handler {
        @Override
        Object translate(String valueText) {
            return new File(valueText);
        }
    }

    /**
     * A {@link Handler} to handle values for Map fields.  The {@code Object} returned is a
     * MapEntry
     */
    private static class MapHandler extends Handler {
        private Handler mKeyHandler;
        private Handler mValueHandler;

        MapHandler(Handler keyHandler, Handler valueHandler) {
            if (keyHandler == null || valueHandler == null) {
                throw new NullPointerException();
            }

            mKeyHandler = keyHandler;
            mValueHandler = valueHandler;
        }

        Handler getKeyHandler() {
            return mKeyHandler;
        }

        Handler getValueHandler() {
            return mValueHandler;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        boolean isMap() {
            return true;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public int hashCode() {
            return Objects.hashCode(MapHandler.class, mKeyHandler, mValueHandler);
        }

        /**
         * Define two {@link MapHandler}s as equivalent if their key and value Handlers are
         * respectively equivalent.
         * <p />
         * {@inheritDoc}
         */
        @Override
        public boolean equals(Object otherObj) {
            if ((otherObj != null) && (otherObj instanceof MapHandler)) {
                MapHandler other = (MapHandler) otherObj;
                Handler otherKeyHandler = other.getKeyHandler();
                Handler otherValueHandler = other.getValueHandler();

                return mKeyHandler.equals(otherKeyHandler)
                        && mValueHandler.equals(otherValueHandler);
            }

            return false;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        Object translate(String valueText) {
            return mValueHandler.translate(valueText);
        }

        Object translateKey(String keyText) {
            return mKeyHandler.translate(keyText);
        }
    }

    /**
     * A {@link Handler} to handle values for {@link Enum} fields.
     */
    private static class EnumHandler extends Handler {
        private final Class mEnumType;

        EnumHandler(Class<?> enumType) {
            mEnumType = enumType;
        }

        Class<?> getEnumType() {
            return mEnumType;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public int hashCode() {
            return Objects.hashCode(EnumHandler.class, mEnumType);
        }

        /**
         * Define two EnumHandlers as equivalent if their EnumTypes are mutually assignable
         * <p />
         * {@inheritDoc}
         */
        @SuppressWarnings("unchecked")
        @Override
        public boolean equals(Object otherObj) {
            if ((otherObj != null) && (otherObj instanceof EnumHandler)) {
                EnumHandler other = (EnumHandler) otherObj;
                Class<?> otherType = other.getEnumType();

                return mEnumType.isAssignableFrom(otherType)
                        && otherType.isAssignableFrom(mEnumType);
            }

            return false;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        Object translate(String valueText) {
            return translate(valueText, true);
        }

        @SuppressWarnings("unchecked")
        Object translate(String valueText, boolean shouldTryUpperCase) {
            try {
                return Enum.valueOf(mEnumType, valueText);
            } catch (IllegalArgumentException e) {
                // Will be thrown if the value can't be mapped back to the enum
                if (shouldTryUpperCase) {
                    // Try to automatically map variable-case strings to uppercase.  This is
                    // reasonable since most Enum constants tend to be uppercase by convention.
                    return translate(valueText.toUpperCase(Locale.ENGLISH), false);
                } else {
                    return null;
                }
            }
        }
    }
}
