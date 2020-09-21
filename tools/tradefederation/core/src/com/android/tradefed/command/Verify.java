/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tradefed.command;

import com.android.tradefed.config.ArgsOptionParser;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.Option;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Alternate Trade Federation entrypoint to validate command files
 */
public class Verify {

    private static final int EXIT_STATUS_OKAY = 0x0;
    private static final int EXIT_STATUS_FAILED = 0x1;

    @Option(name = "cmdfile", description = "command file to verify")
    private List<File> mCmdFiles = new ArrayList<>();

    @Option(name = "show-commands", description = "Whether to print all generated commands")
    private boolean mShowCommands = false;

    @Option(name = "quiet", shortName = 'q', description = "Whether to silence all output. " +
            "Overrides all other output-related settings.")
    private boolean mQuiet = false;

    @Option(name = "help", shortName = 'h', description = "Print help")
    private boolean mHelp = false;

    /**
     * Returns whether the "--help"/"-h" option was passed to the instance
     */
    public boolean isHelpMode() {
        return mHelp;
    }

    /**
     * Program main entrypoint
     */
    public static void main(final String[] mainArgs) throws ConfigurationException {
        try {
            Verify verify = new Verify();
            ArgsOptionParser optionSetter = new ArgsOptionParser(verify);
            optionSetter.parse(mainArgs);
            if (verify.isHelpMode()) {
                // Print help, then exit
                System.err.println(ArgsOptionParser.getOptionHelp(false, verify));
                System.exit(EXIT_STATUS_OKAY);
            }

            if (verify.run()) {
                // true == everything's good
                System.exit(EXIT_STATUS_OKAY);
            } else {
                // false == whoopsie!
                System.exit(EXIT_STATUS_FAILED);
            }

        } finally {
            System.err.flush();
            System.out.flush();
        }
    }

    /**
     * Start validating all specified cmdfiles
     */
    public boolean run() {
        boolean anyFailures = false;

        for (File cmdFile : mCmdFiles) {
            try {
                // if verify returns false, then we set anyFailures to true
                anyFailures |= !runVerify(cmdFile);

            } catch (Throwable t) {
                if (!mQuiet) {
                    System.err.format("Caught exception while parsing \"%s\"\n", cmdFile);
                    System.err.println(t);
                }
                anyFailures = true;
            }
        }

        return !anyFailures;
    }

    /**
     * Validate the specified cmdfile
     */
    public boolean runVerify(File cmdFile) {
        final CommandFileParser parser = new CommandFileParser();
        try {
            List<CommandFileParser.CommandLine> commands = parser.parseFile(cmdFile);
            if (!mQuiet) {
                System.out.format("Successfully parsed %d commands from cmdfile %s\n",
                        commands.size(), cmdFile);

                if (mShowCommands) {
                    int i = 1;
                    int digits = (int) Math.ceil(Math.log10(commands.size()));
                    // Create a format string that will leave enough space for an index prefix
                    // without mucking up alignment
                    String format = String.format("%%%dd: %%s\n", digits);
                    for (CommandFileParser.CommandLine cmd : commands) {
                        System.out.format(format, i++, cmd);
                    }
                }
                System.out.println();
            }
        } catch (ConfigurationException | IOException e) {
            if (!mQuiet) {
                System.err.format("Failed to parse %s:\n", cmdFile);
                System.err.println(e);
            }

            return false;
        }

        return true;
    }
}
