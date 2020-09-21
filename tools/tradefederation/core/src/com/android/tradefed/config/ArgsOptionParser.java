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

import com.android.ddmlib.Log;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.keystore.DryRunKeyStore;
import com.android.tradefed.util.keystore.IKeyStoreClient;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.ListIterator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Populates {@link Option} fields from parsed command line arguments.
 * <p/>
 * Strings in the passed-in String[] are parsed left-to-right. Each String is classified as a short
 * option (such as "-v"), a long option (such as "--verbose"), an argument to an option (such as
 * "out.txt" in "-f out.txt"), or a non-option positional argument.
 * <p/>
 * Each option argument must map to one or more {@link Option} fields. A long option maps to the
 * {@link Option} name, and a short option maps to {@link Option} short name. Each option name and
 * option short name must be unique with respect to all other
 * {@link Option} fields within the same object.
 * <p/>
 * A single option argument can get mapped to multiple {@link Option} fields with the same name
 * across multiple objects. {@link Option} arguments can be namespaced to uniquely refer to an
 * {@link Option} field within a single object using that object's full class name or its
 * {@link OptionClass} alias value separated by ':'. ie
 *
 * <pre>
 * --classname:optionname optionvalue or
 * --optionclassalias:optionname optionvalue.
 * </pre>
 * <p/>
 * A simple short option is a "-" followed by a short option character. If the option requires an
 * argument (which is true of any non-boolean option), it may be written as a separate parameter,
 * but need not be. That is, "-f out.txt" and "-fout.txt" are both acceptable.
 * <p/>
 * It is possible to specify multiple short options after a single "-" as long as all (except
 * possibly the last) do not require arguments.
 * <p/>
 * A long option begins with "--" followed by several characters. If the option requires an
 * argument, it may be written directly after the option name, separated by "=", or as the next
 * argument. (That is, "--file=out.txt" or "--file out.txt".)
 * <p/>
 * A boolean long option '--name' automatically gets a '--no-name' companion. Given an option
 * "--flag", then, "--flag", "--no-flag", "--flag=true" and "--flag=false" are all valid, though
 * neither "--flag true" nor "--flag false" are allowed (since "--flag" by itself is sufficient, the
 * following "true" or "false" is interpreted separately). You can use "yes" and "no" as synonyms
 * for "true" and "false".
 * <p/>
 * Each String not starting with a "-" and not a required argument of a previous option is a
 * non-option positional argument, as are all successive Strings. Each String after a "--" is a
 * non-option positional argument.
 * <p/>
 * The fields corresponding to options are updated as their options are processed. Any remaining
 * positional arguments are returned as a List&lt;String&gt;.
 * <p/>
 * Here's a simple example:
 * <p/>
 *
 * <pre>
 * // Non-&#64;Option fields will be ignored.
 * class Options {
 *     &#64;Option(name = "quiet", shortName = 'q')
 *     boolean quiet = false;
 *
 *     // Here the user can use --no-color.
 *     &#64;Option(name = "color")
 *     boolean color = true;
 *
 *     &#64;Option(name = "mode", shortName = 'm')
 *     String mode = "standard; // Supply a default just by setting the field.
 *
 *     &#64;Option(name = "port", shortName = 'p')
 *     int portNumber = 8888;
 *
 *     // There's no need to offer a short name for rarely-used options.
 *     &#64;Option(name = "timeout" )
 *     double timeout = 1.0;
 *
 *     &#64;Option(name = "output-file", shortName = 'o' })
 *     File output;
 *
 *     // Multiple options are added to the collection.
 *     // The collection field itself must be non-null.
 *     &#64;Option(name = "input-file", shortName = 'i')
 *     List&lt;File&gt; inputs = new ArrayList&lt;File&gt;();
 *
 * }
 *
 * Options options = new Options();
 * List&lt;String&gt; posArgs = new OptionParser(options).parse("--input-file", "/tmp/file1.txt");
 * for (File inputFile : options.inputs) {
 *     if (!options.quiet) {
 *        ...
 *     }
 *     ...
 *
 * }
 *
 * </pre>
 *
 * See also:
 * <ul>
 * <li>the getopt(1) man page
 * <li>Python's "optparse" module (http://docs.python.org/library/optparse.html)
 * <li>the POSIX "Utility Syntax Guidelines"
 * (http://www.opengroup.org/onlinepubs/000095399/basedefs/xbd_chap12.html#tag_12_02)
 * <li>the GNU "Standards for Command Line Interfaces"
 * (http://www.gnu.org/prep/standards/standards.html#Command_002dLine-Interfaces)
 * </ul>
 *
 * @see OptionSetter
 */
public class ArgsOptionParser extends OptionSetter {
    private static final String LOG_TAG = "ArgsOptionParser";

    static final String SHORT_NAME_PREFIX = "-";
    static final String OPTION_NAME_PREFIX = "--";
    // For a boolean pattern match:  {device name}namespace:(no-)option-name
    static final Pattern BOOL_FALSE_DEVICE_PATTERN = Pattern.compile("(\\{.*\\})(.*:)?(no-)(.+)");

    /** the amount to indent an option field's description when displaying help */
    private static final int OPTION_DESCRIPTION_INDENT = 25;

    /**
     * Creates a {@link ArgsOptionParser} for a collection of objects.
     *
     * @param optionSources the config objects.
     * @throws ConfigurationException if config objects is improperly configured.
     */
    public ArgsOptionParser(Collection<Object> optionSources) throws ConfigurationException {
        super(optionSources);
    }

    /**
     * Creates a {@link ArgsOptionParser} for one or more objects.
     *
     * @param optionSources the config objects.
     * @throws ConfigurationException if config objects is improperly configured.
     */
    public ArgsOptionParser(Object... optionSources) throws ConfigurationException {
        super(optionSources);
    }

    /**
     * Parses the command-line arguments 'args', setting the @Option fields of the 'optionSource'
     * provided to the constructor.
     *
     * @return a {@link List} of the positional arguments left over after processing all options.
     * @throws ConfigurationException if error occurred parsing the arguments.
     */
    public List<String> parse(String... args) throws ConfigurationException {
        return parse(Arrays.asList(args));
    }

    /**
     * Alternate {@link #parse(String... args)} method that takes a {@link List} of arguments
     *
     * @return a {@link List} of the positional arguments left over after processing all options.
     * @throws ConfigurationException if error occurred parsing the arguments.
     */
    public List<String> parse(List<String> args) throws ConfigurationException {
        final List<String> leftovers = new ArrayList<String>();
        final ListIterator<String> argsIter = args.listIterator();

        // Scan 'args'.
        while (argsIter.hasNext() && parseArg(argsIter.next(), argsIter, leftovers)) {
            // This loop has no body.  All of the work happens by side-effect in parseArg(...)
        }

        // Package up the leftovers.
        while (argsIter.hasNext()) {
            leftovers.add(argsIter.next());
        }
        return leftovers;
    }

    /**
     * A best-effort version of {@link #parse(String... args)}.  If a ConfigurationException is
     * thrown, that exception is captured internally, and the remaining arguments (including the
     * argument which caused the exception to be thrown) are returned.  This method does not throw.
     *
     * @return a {@link List} of the left over arguments
     */
    public List<String> parseBestEffort(String... args) {
        return parseBestEffort(Arrays.asList(args));
    }

    /**
     * Alternate {@link #parseBestEffort(String... args)} method that takes a {@link List} of
     * arguments
     *
     * @return a {@link List} of the left over arguments
     */
    public List<String> parseBestEffort(List<String> args) {
        return parseBestEffort(args, false);
    }

    /**
     * Alternate {@link #parseBestEffort(String... args)} method that takes a {@link List} of
     * arguments, and can be forced to continue parsing until the end, even if some args do not
     * parse.
     *
     * @param args list that will contain the left over args.
     * @param forceContinue True if it should continue to parse even if some args do not parse.
     * @return a {@link List} of the left over arguments
     */
    public List<String> parseBestEffort(List<String> args, boolean forceContinue) {
        final List<String> leftovers = new ArrayList<String>(args.size());
        final ListIterator<String> argsIter = args.listIterator();
        int lastProcessedIdx = -1;

        /* (not javadoc)
         * Scan 'args'.  Note that each call to parseArg(...) will advance argsIter a number
         * of places that depends on the arity of the particular Option being processed.  For a
         * boolean Option, it will not advance.  For a standard unary Option, it will advance
         * 1 place.  For a Map Option, it will advance two places (1 for key, 1 for value).
         *
         * For this reason, we grab the index from the iterator itself, rather than trying to
         * increment it ourselves.
         */
        boolean lastCheckThrew = false;
        while (argsIter.hasNext()) {
            lastProcessedIdx = argsIter.nextIndex();
            final String arg = argsIter.next();
            try {
                // All of the work happens within parseArg(...) by side-effect
                if (!parseArg(arg, argsIter, leftovers)) {
                    if (!lastCheckThrew) {
                        break;
                    }
                }
                lastCheckThrew = false;
            } catch (ConfigurationException e) {
                lastCheckThrew = true;
                // Something failed.  Add all of the not-fully-processed and not-yet-processed args
                // to leftovers and return
                if (!forceContinue) {
                    leftovers.addAll(args.subList(lastProcessedIdx, args.size()));
                    return leftovers;
                } else {
                    leftovers.add(arg);
                }
            }
        }

        // Package up the leftovers.
        while (argsIter.hasNext()) {
            leftovers.add(argsIter.next());
        }
        return leftovers;
    }

    /**
     * Validates that all fields marked as mandatory have been set.
     * @throws ConfigurationException
     */
    public void validateMandatoryOptions() throws ConfigurationException {
        // Make sure that all mandatory options have been specified
        List<String> missingOptions = new ArrayList<String>(getUnsetMandatoryOptions());
        if (!missingOptions.isEmpty()) {
            throw new ConfigurationException(String.format("Found missing mandatory options: %s",
                    ArrayUtil.join(", ", missingOptions)));
        }
    }

    /**
     * Attempts to parse the specified argument.
     *
     * @return {@code true} if iteration should continue, or {@code false} if iteration should stop
     */
    private boolean parseArg(String arg, ListIterator<String> args, List<String> leftovers)
            throws ConfigurationException {
        if (arg.equals(OPTION_NAME_PREFIX)) {
            // "--" marks the end of options and the beginning of positional arguments.
            return false;

        } else if (arg.startsWith(OPTION_NAME_PREFIX)) {
            // A long option.
            parseLongOption(arg, args);
            return true;

        } else if (arg.startsWith(SHORT_NAME_PREFIX)) {
            // A short option.
            parseGroupedShortOptions(arg, args);
            return true;

        } else {
            // The first non-option marks the end of options.
            leftovers.add(arg);
            return false;
        }
    }

    private void parseLongOption(String arg, ListIterator<String> args)
            throws ConfigurationException {
        // remove prefix to just get name
        String name = arg.replaceFirst("^" + OPTION_NAME_PREFIX, "");
        String key = null;
        String value = null;

        // Support "--name=value" as well as "--name value".
        final int equalsIndex = name.indexOf('=');
        if (equalsIndex != -1) {
            value = name.substring(equalsIndex + 1);
            name = name.substring(0, equalsIndex);
        }

        if (value == null) {
            if (isBooleanOption(name)) {
                int idx = name.indexOf(NAMESPACE_SEPARATOR);
                // Detect a device tag in front of the boolean option.
                Matcher m = BOOL_FALSE_DEVICE_PATTERN.matcher(name);
                if (m.find()) {
                    value = "false";
                } else {
                    value = name.startsWith(BOOL_FALSE_PREFIX, idx + 1) ? "false" : "true";
                }
            } else if (isMapOption(name)) {
                // Support --option key=value and --option key value format
                String tmp = grabNextValue(args, name, "for its key");
                // only match = to escape use "\="
                Pattern p = Pattern.compile("(?<!\\\\)=");
                String[] parts = p.split(tmp);
                // Note that we replace escaped = (\=) to =.
                if (parts.length == 2) {
                    key = parts[0].replaceAll("\\\\=", "=");
                    value = parts[1].replaceAll("\\\\=", "=");
                } else if (parts.length > 2) {
                    throw new ConfigurationException(String.format(
                            "option '%s' has an invalid format for value %s:w",
                            name, tmp));
                } else {
                    key = tmp.replaceAll("\\\\=", "=");
                    value = grabNextValue(args, name, "for its value").replaceAll("\\\\=", "=");
                }
            } else {
                value = grabNextValue(args, name);
            }
        }

        value = getKeyStoreValueIfNeeded(value, getTypeForOption(name));
        setOptionValue(name, key, value);
    }

    // Given boolean options a and b, and non-boolean option f, we want to allow:
    // -ab
    // -abf out.txt
    // -abfout.txt
    // (But not -abf=out.txt --- POSIX doesn't mention that either way, but GNU expressly forbids
    // it.)
    private void parseGroupedShortOptions(String arg, ListIterator<String> args)
            throws ConfigurationException {
        for (int i = 1; i < arg.length(); ++i) {
            final String name = String.valueOf(arg.charAt(i));
            String value;
            if (isBooleanOption(name)) {
                value = "true";
            } else {
                // We need a value. If there's anything left, we take the rest of this
                // "short option".
                if (i + 1 < arg.length()) {
                    value = arg.substring(i + 1);
                    i = arg.length() - 1;
                } else {
                    value = grabNextValue(args, name);
                }
            }
            value = getKeyStoreValueIfNeeded(value, getTypeForOption(name));
            setOptionValue(name, value);
        }
    }

    /**
     * Returns the next element of 'args' if there is one. Uses 'name' to construct a helpful error
     * message.
     *
     * @param args the arg iterator
     * @param name the name of current argument
     * @throws ConfigurationException if no argument is present
     *
     * @returns the next element
     */
    private String grabNextValue(ListIterator<String> args, String name)
            throws ConfigurationException {
        return grabNextValue(args, name, "");
    }

    /**
     * Returns the next element of 'args' if there is one. Uses 'name' to construct a helpful error
     * message.
     *
     * @param args the arg iterator
     * @param name the name of current argument
     * @param detail a string to append to the ConfigurationException message, if one is thrown
     * @throws ConfigurationException if no argument is present
     *
     * @returns the next element
     */
    private String grabNextValue(ListIterator<String> args, String name, String detail)
            throws ConfigurationException {
        if (!args.hasNext()) {
            String type = getTypeForOption(name);
            throw new ConfigurationException(String.format("option '%s' requires a '%s' argument%s",
                    name, type, detail));
        }
        return args.next();
    }

    /**
     * Output help text for all {@link Option} fields in <param>optionObject</param>.
     * <p/>
     * The help text for each option will be in the following format
     * <pre>
     *   [-option_shortname, --option_name]          [option_description] Default:
     *   [current option field's value in optionObject]
     * </pre>
     * The 'Default..." text will be omitted if the option field is null or empty.
     *
     * @param importantOnly if <code>true</code> only print help for the important options
     * @param optionObject the object to print help text for
     * @return a String containing user-friendly help text for all Option fields
     */
    public static String getOptionHelp(boolean importantOnly, Object optionObject) {
        StringBuilder out = new StringBuilder();
        Collection<Field> optionFields = OptionSetter.getOptionFieldsForClass(
                optionObject.getClass());
        String eol = System.getProperty("line.separator");
        for (Field field : optionFields) {
            final Option option = field.getAnnotation(Option.class);
            String defaultValue = OptionSetter.getFieldValueAsString(field, optionObject);
            String optionNameHelp = buildOptionNameHelp(field, option);
            if (shouldOutputHelpForOption(importantOnly, option, defaultValue)) {
                out.append(optionNameHelp);
                // insert appropriate whitespace between the name help and the description, to
                // ensure consistent alignment
                int wsChars = 0;
                if (optionNameHelp.length() >= OPTION_DESCRIPTION_INDENT) {
                    // name help is too long, break description onto next line
                    out.append(eol);
                    wsChars = OPTION_DESCRIPTION_INDENT;
                } else {
                    // insert enough whitespace so option.description starts at
                    // OPTION_DESCRIPTION_INDENT
                    wsChars = OPTION_DESCRIPTION_INDENT - optionNameHelp.length();
                }
                for (int i = 0; i < wsChars; ++i) {
                    out.append(' ');
                }
                out.append(option.description());
                out.append(getDefaultValueHelp(defaultValue));
                out.append(OptionSetter.getEnumFieldValuesAsString(field));
                out.append(eol);
            }
        }
        return out.toString();
    }

    /**
     * Determine if help for given option should be displayed.
     *
     * @param importantOnly
     * @param option
     * @param defaultValue
     * @return <code>true</code> if help for option should be displayed
     */
    private static boolean shouldOutputHelpForOption(boolean importantOnly, Option option,
            String defaultValue) {
        if (!importantOnly) {
            return true;
        }
        switch (option.importance()) {
            case NEVER:
                return false;
            case IF_UNSET:
                return defaultValue == null;
            case ALWAYS:
                return true;
        }
        Log.e(LOG_TAG, String.format("Unrecognized importance setting '%s'",
                option.importance().toString()));
        return false;
    }

    /**
     * Builds the 'name' portion of the help text for the given option field
     *
     * @param field
     * @param option
     * @return the help text that describes the option flags
     */
    private static String buildOptionNameHelp(Field field, final Option option) {
        StringBuilder optionNameBuilder = new StringBuilder();
        optionNameBuilder.append("    ");
        if (option.shortName() != Option.NO_SHORT_NAME) {
            optionNameBuilder.append(SHORT_NAME_PREFIX);
            optionNameBuilder.append(option.shortName());
            optionNameBuilder.append(", ");
        }
        optionNameBuilder.append(OPTION_NAME_PREFIX);
        try {
            if (OptionSetter.isBooleanField(field)) {
                optionNameBuilder.append("[no-]");
            }
        } catch (ConfigurationException e) {
            // ignore
        }
        optionNameBuilder.append(option.name());
        return optionNameBuilder.toString();
    }

    /**
     * Returns the help text describing the given default value
     *
     * @param defaultValue the default value
     * @return the help text, or an empty {@link String} if <param>field</param> has no value
     */
    private static String getDefaultValueHelp(String defaultValue) {
        if (defaultValue == null) {
            return "";
        } else {
            return String.format(" Default: %s.", defaultValue);
        }
    }

    /**
     * Replaces value with key store value if needed
     *
     * @param valueText the value
     * @param optionType the type of the value expected in the option.
     * @return the value or the translated key store value.
     * @throws ConfigurationException
     */
    private String getKeyStoreValueIfNeeded(String valueText, String optionType)
            throws ConfigurationException {
        Matcher m = USE_KEYSTORE_REGEX.matcher(valueText);
        if (m.matches() && m.groupCount() > 0) {
            IKeyStoreClient c = getKeyStore();
            if (c == null) {
                throw new ConfigurationException("Key store is null, but we tried to fetch a key");
            }
            if (!c.isAvailable()) {
                throw new ConfigurationException(String.format("Key store '%s' is unavailable, but "
                        + "we tried to fetch a key", c.getClass()));
            }
            String key = m.group(1);
            String v = null;
            // if it's the special dry-run keystore, we use a special handling and don't throw.
            if (c instanceof DryRunKeyStore) {
                v = ((DryRunKeyStore) c).fetchKey(key, optionType);
            } else {
                v = c.fetchKey(key);
            }
            if (v == null) {
                throw new ConfigurationException(String.format(
                        "Failed to fetch key %s in keystore", key));
            }
            return v;
        }
        // Did not match the key store pattern, do nothing.
        return valueText;
    }
}
