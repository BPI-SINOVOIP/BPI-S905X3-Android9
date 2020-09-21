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
 * limitations under the License
 */

package com.android.tools.appbundle.bundletool.utils;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * Utility for flag parsing, specific to the Bundle Tool.
 *
 * <p>The flags follow the below convention:
 *
 * <p>[bundle-tool] [command1] [command2] .. [command-n] [--flag1] [--flag2=v2].. [--flagn] where:
 *
 * <ul>
 *   <li>commands: cannot start with "-".
 *   <li>flags: have to start with "--". If they have "=" anything after the first occurrence is
 *       considered a flag value. By default the flag value is an empty string.
 * </ul>
 */
public class FlagParser {

  private List<String> commands = new ArrayList<>();
  private Map<String, String> flags = new HashMap<>();

  /**
   * Parses the given arguments populating the structures.
   *
   * <p>Calling this function removes any previous parsing results.
   */
  public FlagParser parse(String[] args) throws ParseException {
    this.commands.clear();
    // Need to wrap it into a proper list implementation to be able to remove elements.
    List<String> argsToProcess = new ArrayList<>(Arrays.asList(args));
    while (argsToProcess.size() > 0 && !argsToProcess.get(0).startsWith("-")) {
      commands.add(argsToProcess.get(0));
      argsToProcess.remove(0);
    }
    this.flags = parseFlags(argsToProcess);
    return this;
  }

  private Map<String, String> parseFlags(List<String> args) throws ParseException {
    Map<String, String> flagMap = new HashMap<>();
    for (String arg : args) {
      if (!arg.startsWith("--")) {
        throw new ParseException(
            String.format("Syntax error: flags should start with -- (%s)", arg));
      }
      String[] segments = arg.substring(2).split("=", 2);
      String value = "";
      if (segments.length == 2) {
        value = segments[1];
      }
      if (flagMap.putIfAbsent(segments[0], value) != null) {
        throw new ParseException(
            String.format("Flag %s has been set more than once.", segments[0]));
      }
    }
    return flagMap;
  }

  /** Returns true if a given flag has been set. */
  public boolean isFlagSet(String flagName) {
    return flags.containsKey(flagName);
  }

  /** Returns the flag value wrapped in the Optional class. */
  public Optional<String> getFlagValue(String flagName) {
    return Optional.ofNullable(flags.get(flagName));
  }

  /**
   * Returns a flag value. If absent throws IllegalStateException.
   *
   * @param flagName name of the flag to fetch
   * @return string, the value of the flag
   * @throws IllegalStateException if the flag was not set.
   */
  public String getRequiredFlagValue(String flagName) {
    return getFlagValue(flagName)
        .orElseThrow(
            () ->
                new IllegalArgumentException(
                    String.format("Missing the required --%s flag.", flagName)));
  }

  /** Returns the string value of the flag or the default if has not been set. */
  public String getFlagValueOrDefault(String flagName, String defaultValue) {
    return flags.getOrDefault(flagName, defaultValue);
  }

  public Optional<Path> getFlagValueAsPath(String flagName) {
    return Optional.ofNullable(flags.get(flagName)).map(Paths::get);
  }

  /**
   * Returns the value of the flag as list of strings.
   *
   * <p>It converts the string flag value to the list assuming it's delimited by a comma. The list
   * is empty if the flag has not been set.
   */
  public List<String> getFlagListValue(String flagName) {
    if (!isFlagSet(flagName)) {
      return Collections.emptyList();
    }
    return Arrays.asList(flags.get(flagName).split(","));
  }

  /**
   * Returns the list of commands that were parsed.
   *
   * @return the immutable list of commands.
   */
  public List<String> getCommands() {
    return Collections.unmodifiableList(commands);
  }

  /** Exception encapsulating any flag parsing errors. */
  public static class ParseException extends RuntimeException {

    public ParseException(String message) {
      super(message);
    }
  }
}
