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

package com.android.tools.appbundle.bundletool;

import com.android.tools.appbundle.bundletool.utils.FlagParser;
import java.io.IOException;
import java.util.List;

/**
 * Main entry point of the bundle tool.
 *
 * <p>Consider running with -Dsun.zip.disableMemoryMapping when dealing with large bundles.
 */
public class BundleToolMain {

  public static final String LINK_CMD = "link";
  public static final String HELP_CMD = "help";

  /** Parses the flags and routes to the appropriate command handler */
  public static void main(String[] args) throws IOException {

    FlagParser flagParser = new FlagParser();
    try {
      flagParser.parse(args);
    } catch (FlagParser.ParseException e) {
      System.out.println(String.format("Error while parsing the flags: %s", e.getMessage()));
      return;
    }
    List<String> commands = flagParser.getCommands();

    if (commands.isEmpty()) {
      System.out.println("Error: you have to specify a command");
      help();
      return;
    }

    try {
      switch (commands.get(0)) {
        case BuildModuleCommand.COMMAND_NAME:
          BuildModuleCommand.fromFlags(flagParser).execute();
          break;
        case SplitModuleCommand.COMMAND_NAME:
          new SplitModuleCommand(flagParser).execute();
          break;
        case LINK_CMD:
          throw new UnsupportedOperationException("Not implemented.");
        case HELP_CMD:
          if (commands.size() > 1) {
            help(commands.get(1));
          } else {
            help();
          }
          return;
        default:
          System.out.println("Error: unrecognized command.");
          help();
      }
    } catch (Exception e) {
      System.out.println("Error: " + e.getMessage());
    }
  }

  /** Displays a general help. */
  public static void help() {
    System.out.println(
        String.format(
            "bundletool [%s|%s|%s|%s] ...",
            BuildModuleCommand.COMMAND_NAME, SplitModuleCommand.COMMAND_NAME, LINK_CMD, HELP_CMD));
    System.out.println("Type: bundletool help [command] to learn more about a given command.");
  }

  /** Displays help about a given command. */
  public static void help(String commandName) {
    switch (commandName) {
      case BuildModuleCommand.COMMAND_NAME:
        BuildModuleCommand.help();
        break;
      case SplitModuleCommand.COMMAND_NAME:
        SplitModuleCommand.help();
        break;
      case LINK_CMD:
        System.out.println("Help is not yet available.");
        break;
      default:
        System.out.println("Unrecognized command.");
        help();
        break;
    }
  }
}
