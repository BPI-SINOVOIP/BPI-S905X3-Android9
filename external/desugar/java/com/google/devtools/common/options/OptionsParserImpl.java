// Copyright 2014 The Bazel Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.devtools.common.options;

import static java.util.Comparator.comparing;
import static java.util.stream.Collectors.toCollection;

import com.google.common.base.Joiner;
import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterators;
import com.google.devtools.common.options.OptionPriority.PriorityCategory;
import com.google.devtools.common.options.OptionValueDescription.ExpansionBundle;
import com.google.devtools.common.options.OptionsParser.OptionDescription;
import java.lang.reflect.Constructor;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import javax.annotation.Nullable;

/**
 * The implementation of the options parser. This is intentionally package
 * private for full flexibility. Use {@link OptionsParser} or {@link Options}
 * if you're a consumer.
 */
class OptionsParserImpl {

  private final OptionsData optionsData;

  /**
   * We store the results of option parsing in here - since there can only be one value per option
   * field, this is where the different instances of an option have been combined and the final
   * value is tracked. It'll look like
   *
   * <pre>
   *   OptionDefinition("--host") -> "www.google.com"
   *   OptionDefinition("--port") -> 80
   * </pre>
   *
   * This map is modified by repeated calls to {@link #parse(OptionPriority.PriorityCategory,
   * Function,List)}.
   */
  private final Map<OptionDefinition, OptionValueDescription> optionValues = new HashMap<>();

  /**
   * Explicit option tracking, tracking each option as it was provided, after they have been parsed.
   *
   * <p>The value is unconverted, still the string as it was read from the input, or partially
   * altered in cases where the flag was set by non {@code --flag=value} forms; e.g. {@code --nofoo}
   * becomes {@code --foo=0}.
   */
  private final List<ParsedOptionDescription> parsedOptions = new ArrayList<>();

  private final List<String> warnings = new ArrayList<>();

  /**
   * Since parse() expects multiple calls to it with the same {@link PriorityCategory} to be treated
   * as though the args in the later call have higher priority over the earlier calls, we need to
   * track the high water mark of option priority at each category. Each call to parse will start at
   * this level.
   */
  private final Map<PriorityCategory, OptionPriority> nextPriorityPerPriorityCategory =
      Stream.of(PriorityCategory.values())
          .collect(Collectors.toMap(p -> p, OptionPriority::lowestOptionPriorityAtCategory));

  private boolean allowSingleDashLongOptions = false;

  private ArgsPreProcessor argsPreProcessor = args -> args;

  /** Create a new parser object. Do not accept a null OptionsData object. */
  OptionsParserImpl(OptionsData optionsData) {
    Preconditions.checkNotNull(optionsData);
    this.optionsData = optionsData;
  }

  OptionsData getOptionsData() {
    return optionsData;
  }

  /**
   * Indicates whether or not the parser will allow long options with a
   * single-dash, instead of the usual double-dash, too, eg. -example instead of just --example.
   */
  void setAllowSingleDashLongOptions(boolean allowSingleDashLongOptions) {
    this.allowSingleDashLongOptions = allowSingleDashLongOptions;
  }

  /** Sets the ArgsPreProcessor for manipulations of the options before parsing. */
  void setArgsPreProcessor(ArgsPreProcessor preProcessor) {
    this.argsPreProcessor = Preconditions.checkNotNull(preProcessor);
  }

  /** Implements {@link OptionsParser#asCompleteListOfParsedOptions()}. */
  List<ParsedOptionDescription> asCompleteListOfParsedOptions() {
    return parsedOptions
        .stream()
        // It is vital that this sort is stable so that options on the same priority are not
        // reordered.
        .sorted(comparing(ParsedOptionDescription::getPriority))
        .collect(toCollection(ArrayList::new));
  }

  /** Implements {@link OptionsParser#asListOfExplicitOptions()}. */
  List<ParsedOptionDescription> asListOfExplicitOptions() {
    return parsedOptions
        .stream()
        .filter(ParsedOptionDescription::isExplicit)
        // It is vital that this sort is stable so that options on the same priority are not
        // reordered.
        .sorted(comparing(ParsedOptionDescription::getPriority))
        .collect(toCollection(ArrayList::new));
  }

  /** Implements {@link OptionsParser#canonicalize}. */
  List<String> asCanonicalizedList() {
    return asCanonicalizedListOfParsedOptions()
        .stream()
        .map(ParsedOptionDescription::getDeprecatedCanonicalForm)
        .collect(ImmutableList.toImmutableList());
  }

  /** Implements {@link OptionsParser#canonicalize}. */
  List<ParsedOptionDescription> asCanonicalizedListOfParsedOptions() {
    return optionValues
        .keySet()
        .stream()
        .sorted()
        .map(optionDefinition -> optionValues.get(optionDefinition).getCanonicalInstances())
        .flatMap(Collection::stream)
        .collect(ImmutableList.toImmutableList());
  }

  /** Implements {@link OptionsParser#asListOfOptionValues()}. */
  List<OptionValueDescription> asListOfEffectiveOptions() {
    List<OptionValueDescription> result = new ArrayList<>();
    for (Map.Entry<String, OptionDefinition> mapEntry : optionsData.getAllOptionDefinitions()) {
      OptionDefinition optionDefinition = mapEntry.getValue();
      OptionValueDescription optionValue = optionValues.get(optionDefinition);
      if (optionValue == null) {
        result.add(OptionValueDescription.getDefaultOptionValue(optionDefinition));
      } else {
        result.add(optionValue);
      }
    }
    return result;
  }

  private void maybeAddDeprecationWarning(OptionDefinition optionDefinition) {
    // Continue to support the old behavior for @Deprecated options.
    String warning = optionDefinition.getDeprecationWarning();
    if (!warning.isEmpty() || (optionDefinition.getField().isAnnotationPresent(Deprecated.class))) {
      addDeprecationWarning(optionDefinition.getOptionName(), warning);
    }
  }

  private void addDeprecationWarning(String optionName, String warning) {
    warnings.add(
        String.format(
            "Option '%s' is deprecated%s", optionName, (warning.isEmpty() ? "" : ": " + warning)));
  }


  OptionValueDescription clearValue(OptionDefinition optionDefinition)
      throws OptionsParsingException {
    return optionValues.remove(optionDefinition);
  }

  OptionValueDescription getOptionValueDescription(String name) {
    OptionDefinition optionDefinition = optionsData.getOptionDefinitionFromName(name);
    if (optionDefinition == null) {
      throw new IllegalArgumentException("No such option '" + name + "'");
    }
    return optionValues.get(optionDefinition);
  }

  OptionDescription getOptionDescription(String name) throws OptionsParsingException {
    OptionDefinition optionDefinition = optionsData.getOptionDefinitionFromName(name);
    if (optionDefinition == null) {
      return null;
    }
    return new OptionDescription(optionDefinition, optionsData);
  }

  /**
   * Implementation of {@link OptionsParser#getExpansionValueDescriptions(OptionDefinition,
   * OptionInstanceOrigin)}
   */
  ImmutableList<ParsedOptionDescription> getExpansionValueDescriptions(
      OptionDefinition expansionFlag, OptionInstanceOrigin originOfExpansionFlag)
      throws OptionsParsingException {
    ImmutableList.Builder<ParsedOptionDescription> builder = ImmutableList.builder();
    OptionInstanceOrigin originOfSubflags;
    ImmutableList<String> options;
    if (expansionFlag.hasImplicitRequirements()) {
      options = ImmutableList.copyOf(expansionFlag.getImplicitRequirements());
      originOfSubflags =
          new OptionInstanceOrigin(
              originOfExpansionFlag.getPriority(),
              String.format(
                  "implicitly required by %s (source: %s)",
                  expansionFlag, originOfExpansionFlag.getSource()),
              expansionFlag,
              null);
    } else if (expansionFlag.isExpansionOption()) {
      options = optionsData.getEvaluatedExpansion(expansionFlag);
      originOfSubflags =
          new OptionInstanceOrigin(
              originOfExpansionFlag.getPriority(),
              String.format(
                  "expanded by %s (source: %s)", expansionFlag, originOfExpansionFlag.getSource()),
              null,
              expansionFlag);
    } else {
      return ImmutableList.of();
    }

    Iterator<String> optionsIterator = options.iterator();
    while (optionsIterator.hasNext()) {
      String unparsedFlagExpression = optionsIterator.next();
      ParsedOptionDescription parsedOption =
          identifyOptionAndPossibleArgument(
              unparsedFlagExpression,
              optionsIterator,
              originOfSubflags.getPriority(),
              o -> originOfSubflags.getSource(),
              originOfSubflags.getImplicitDependent(),
              originOfSubflags.getExpandedFrom());
      builder.add(parsedOption);
    }
    return builder.build();
  }

  boolean containsExplicitOption(String name) {
    OptionDefinition optionDefinition = optionsData.getOptionDefinitionFromName(name);
    if (optionDefinition == null) {
      throw new IllegalArgumentException("No such option '" + name + "'");
    }
    return optionValues.get(optionDefinition) != null;
  }

  /**
   * Parses the args, and returns what it doesn't parse. May be called multiple times, and may be
   * called recursively. The option's definition dictates how it reacts to multiple settings. By
   * default, the arg seen last at the highest priority takes precedence, overriding the early
   * values. Options that accumulate multiple values will track them in priority and appearance
   * order.
   */
  List<String> parse(
      PriorityCategory priorityCat,
      Function<OptionDefinition, String> sourceFunction,
      List<String> args)
      throws OptionsParsingException {
    ResidueAndPriority residueAndPriority =
        parse(nextPriorityPerPriorityCategory.get(priorityCat), sourceFunction, null, null, args);
    nextPriorityPerPriorityCategory.put(priorityCat, residueAndPriority.nextPriority);
    return residueAndPriority.residue;
  }

  private static final class ResidueAndPriority {
    List<String> residue;
    OptionPriority nextPriority;

    public ResidueAndPriority(List<String> residue, OptionPriority nextPriority) {
      this.residue = residue;
      this.nextPriority = nextPriority;
    }
  }

  /** Parses the args at the fixed priority. */
  List<String> parseOptionsFixedAtSpecificPriority(
      OptionPriority priority, Function<OptionDefinition, String> sourceFunction, List<String> args)
      throws OptionsParsingException {
    ResidueAndPriority residueAndPriority =
        parse(OptionPriority.getLockedPriority(priority), sourceFunction, null, null, args);
    return residueAndPriority.residue;
  }

  /**
   * Parses the args, and returns what it doesn't parse. May be called multiple times, and may be
   * called recursively. Calls may contain intersecting sets of options; in that case, the arg seen
   * last takes precedence.
   *
   * <p>The method treats options that have neither an implicitDependent nor an expandedFrom value
   * as explicitly set.
   */
  private ResidueAndPriority parse(
      OptionPriority priority,
      Function<OptionDefinition, String> sourceFunction,
      OptionDefinition implicitDependent,
      OptionDefinition expandedFrom,
      List<String> args)
      throws OptionsParsingException {
    List<String> unparsedArgs = new ArrayList<>();

    Iterator<String> argsIterator = argsPreProcessor.preProcess(args).iterator();
    while (argsIterator.hasNext()) {
      String arg = argsIterator.next();

      if (!arg.startsWith("-")) {
        unparsedArgs.add(arg);
        continue;  // not an option arg
      }

      if (arg.equals("--")) {  // "--" means all remaining args aren't options
        Iterators.addAll(unparsedArgs, argsIterator);
        break;
      }

      ParsedOptionDescription parsedOption =
          identifyOptionAndPossibleArgument(
              arg, argsIterator, priority, sourceFunction, implicitDependent, expandedFrom);
      handleNewParsedOption(parsedOption);
      priority = OptionPriority.nextOptionPriority(priority);
    }

    // Go through the final values and make sure they are valid values for their option. Unlike any
    // checks that happened above, this also checks that flags that were not set have a valid
    // default value. getValue() will throw if the value is invalid.
    for (OptionValueDescription valueDescription : asListOfEffectiveOptions()) {
      valueDescription.getValue();
    }

    return new ResidueAndPriority(unparsedArgs, priority);
  }

  /**
   * Implementation of {@link OptionsParser#addOptionValueAtSpecificPriority(OptionInstanceOrigin,
   * OptionDefinition, String)}
   */
  void addOptionValueAtSpecificPriority(
      OptionInstanceOrigin origin, OptionDefinition option, String unconvertedValue)
      throws OptionsParsingException {
    Preconditions.checkNotNull(option);
    Preconditions.checkNotNull(
        unconvertedValue,
        "Cannot set %s to a null value. Pass \"\" if an empty value is required.",
        option);
    Preconditions.checkNotNull(
        origin,
        "Cannot assign value \'%s\' to %s without a clear origin for this value.",
        unconvertedValue,
        option);
    PriorityCategory priorityCategory = origin.getPriority().getPriorityCategory();
    boolean isNotDefault = priorityCategory != OptionPriority.PriorityCategory.DEFAULT;
    Preconditions.checkArgument(
        isNotDefault,
        "Attempt to assign value \'%s\' to %s at priority %s failed. Cannot set options at "
            + "default priority - by definition, that means the option is unset.",
        unconvertedValue,
        option,
        priorityCategory);

    handleNewParsedOption(
        new ParsedOptionDescription(
            option,
            String.format("--%s=%s", option.getOptionName(), unconvertedValue),
            unconvertedValue,
            origin));
  }

  /** Takes care of tracking the parsed option's value in relation to other options. */
  private void handleNewParsedOption(ParsedOptionDescription parsedOption)
      throws OptionsParsingException {
    OptionDefinition optionDefinition = parsedOption.getOptionDefinition();
    // All options can be deprecated; check and warn before doing any option-type specific work.
    maybeAddDeprecationWarning(optionDefinition);
    // Track the value, before any remaining option-type specific work that is done outside of
    // the OptionValueDescription.
    OptionValueDescription entry =
        optionValues.computeIfAbsent(
            optionDefinition,
            def -> OptionValueDescription.createOptionValueDescription(def, optionsData));
    ExpansionBundle expansionBundle = entry.addOptionInstance(parsedOption, warnings);
    @Nullable String unconvertedValue = parsedOption.getUnconvertedValue();

    // There are 3 types of flags that expand to other flag values. Expansion flags are the
    // accepted way to do this, but two legacy features remain: implicit requirements and wrapper
    // options. We rely on the OptionProcessor compile-time check's guarantee that no option sets
    // multiple of these behaviors. (In Bazel, --config is another such flag, but that expansion
    // is not controlled within the options parser, so we ignore it here)

    // As much as possible, we want the behaviors of these different types of flags to be
    // identical, as this minimizes the number of edge cases, but we do not yet track these values
    // in the same way. Wrapper options are replaced by their value and implicit requirements are
    // hidden from the reported lists of parsed options.
    if (parsedOption.getImplicitDependent() == null && !optionDefinition.isWrapperOption()) {
      // Log explicit options and expanded options in the order they are parsed (can be sorted
      // later). This information is needed to correctly canonicalize flags.
      parsedOptions.add(parsedOption);
    }

    if (expansionBundle != null) {
      ResidueAndPriority residueAndPriority =
          parse(
              OptionPriority.getLockedPriority(parsedOption.getPriority()),
              o -> expansionBundle.sourceOfExpansionArgs,
              optionDefinition.hasImplicitRequirements() ? optionDefinition : null,
              optionDefinition.isExpansionOption() ? optionDefinition : null,
              expansionBundle.expansionArgs);
      if (!residueAndPriority.residue.isEmpty()) {
        if (optionDefinition.isWrapperOption()) {
          throw new OptionsParsingException(
              "Unparsed options remain after unwrapping "
                  + unconvertedValue
                  + ": "
                  + Joiner.on(' ').join(residueAndPriority.residue));
        } else {
          // Throw an assertion here, because this indicates an error in the definition of this
          // option's expansion or requirements, not with the input as provided by the user.
          throw new AssertionError(
              "Unparsed options remain after processing "
                  + unconvertedValue
                  + ": "
                  + Joiner.on(' ').join(residueAndPriority.residue));
        }
      }
    }
  }

  private ParsedOptionDescription identifyOptionAndPossibleArgument(
      String arg,
      Iterator<String> nextArgs,
      OptionPriority priority,
      Function<OptionDefinition, String> sourceFunction,
      OptionDefinition implicitDependent,
      OptionDefinition expandedFrom)
      throws OptionsParsingException {

    // Store the way this option was parsed on the command line.
    StringBuilder commandLineForm = new StringBuilder();
    commandLineForm.append(arg);
    String unconvertedValue = null;
    OptionDefinition optionDefinition;
    boolean booleanValue = true;

    if (arg.length() == 2) { // -l  (may be nullary or unary)
      optionDefinition = optionsData.getFieldForAbbrev(arg.charAt(1));
      booleanValue = true;

    } else if (arg.length() == 3 && arg.charAt(2) == '-') { // -l-  (boolean)
      optionDefinition = optionsData.getFieldForAbbrev(arg.charAt(1));
      booleanValue = false;

    } else if (allowSingleDashLongOptions // -long_option
        || arg.startsWith("--")) { // or --long_option

      int equalsAt = arg.indexOf('=');
      int nameStartsAt = arg.startsWith("--") ? 2 : 1;
      String name =
          equalsAt == -1 ? arg.substring(nameStartsAt) : arg.substring(nameStartsAt, equalsAt);
      if (name.trim().isEmpty()) {
        throw new OptionsParsingException("Invalid options syntax: " + arg, arg);
      }
      unconvertedValue = equalsAt == -1 ? null : arg.substring(equalsAt + 1);
      optionDefinition = optionsData.getOptionDefinitionFromName(name);

      // Look for a "no"-prefixed option name: "no<optionName>".
      if (optionDefinition == null && name.startsWith("no")) {
        name = name.substring(2);
        optionDefinition = optionsData.getOptionDefinitionFromName(name);
        booleanValue = false;
        if (optionDefinition != null) {
          // TODO(bazel-team): Add tests for these cases.
          if (!optionDefinition.usesBooleanValueSyntax()) {
            throw new OptionsParsingException(
                "Illegal use of 'no' prefix on non-boolean option: " + arg, arg);
          }
          if (unconvertedValue != null) {
            throw new OptionsParsingException(
                "Unexpected value after boolean option: " + arg, arg);
          }
          // "no<optionname>" signifies a boolean option w/ false value
          unconvertedValue = "0";
        }
      }
    } else {
      throw new OptionsParsingException("Invalid options syntax: " + arg, arg);
    }

    if (optionDefinition == null
        || ImmutableList.copyOf(optionDefinition.getOptionMetadataTags())
            .contains(OptionMetadataTag.INTERNAL)) {
      // Do not recognize internal options, which are treated as if they did not exist.
      throw new OptionsParsingException("Unrecognized option: " + arg, arg);
    }

    if (unconvertedValue == null) {
      // Special-case boolean to supply value based on presence of "no" prefix.
      if (optionDefinition.usesBooleanValueSyntax()) {
        unconvertedValue = booleanValue ? "1" : "0";
      } else if (optionDefinition.getType().equals(Void.class)
          && !optionDefinition.isWrapperOption()) {
        // This is expected, Void type options have no args (unless they're wrapper options).
      } else if (nextArgs.hasNext()) {
        // "--flag value" form
        unconvertedValue = nextArgs.next();
        commandLineForm.append(" ").append(unconvertedValue);
      } else {
        throw new OptionsParsingException("Expected value after " + arg);
      }
    }

    return new ParsedOptionDescription(
        optionDefinition,
        commandLineForm.toString(),
        unconvertedValue,
        new OptionInstanceOrigin(
            priority, sourceFunction.apply(optionDefinition), implicitDependent, expandedFrom));
  }

  /**
   * Gets the result of parsing the options.
   */
  <O extends OptionsBase> O getParsedOptions(Class<O> optionsClass) {
    // Create the instance:
    O optionsInstance;
    try {
      Constructor<O> constructor = optionsData.getConstructor(optionsClass);
      if (constructor == null) {
        return null;
      }
      optionsInstance = constructor.newInstance();
    } catch (ReflectiveOperationException e) {
      throw new IllegalStateException("Error while instantiating options class", e);
    }

    // Set the fields
    for (OptionDefinition optionDefinition :
        OptionsData.getAllOptionDefinitionsForClass(optionsClass)) {
      Object value;
      OptionValueDescription optionValue = optionValues.get(optionDefinition);
      if (optionValue == null) {
        value = optionDefinition.getDefaultValue();
      } else {
        value = optionValue.getValue();
      }
      try {
        optionDefinition.getField().set(optionsInstance, value);
      } catch (IllegalArgumentException e) {
        throw new IllegalStateException(
            String.format("Unable to set %s to value '%s'.", optionDefinition, value), e);
      } catch (IllegalAccessException e) {
        throw new IllegalStateException(
            "Could not set the field due to access issues. This is impossible, as the "
                + "OptionProcessor checks that all options are non-final public fields.",
            e);
      }
    }
    return optionsInstance;
  }

  List<String> getWarnings() {
    return ImmutableList.copyOf(warnings);
  }
}
