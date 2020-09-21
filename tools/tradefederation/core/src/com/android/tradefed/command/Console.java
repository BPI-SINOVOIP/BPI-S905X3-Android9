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

package com.android.tradefed.command;

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.config.ArgsOptionParser;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.log.ConsoleReaderOutputStream;
import com.android.tradefed.log.LogRegistry;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.ConfigCompletor;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.QuotationAwareTokenizer;
import com.android.tradefed.util.RegexTrie;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.TimeUtil;
import com.android.tradefed.util.VersionParser;
import com.android.tradefed.util.ZipUtil;
import com.android.tradefed.util.keystore.IKeyStoreFactory;
import com.android.tradefed.util.keystore.KeyStoreException;

import com.google.common.annotations.VisibleForTesting;

import jline.ConsoleReader;

import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.TreeMap;
import java.util.regex.Pattern;

/**
 * Main TradeFederation console providing user with the interface to interact
 * <p/>
 * Currently supports operations such as
 * <ul>
 * <li>add a command to test
 * <li>list devices and their state
 * <li>list invocations in progress
 * <li>list commands in queue
 * <li>dump invocation log to file/stdout
 * <li>shutdown
 * </ul>
 */
public class Console extends Thread {

    private static final String CONSOLE_PROMPT = "\u001B[0;32mtf >\u001B[0;0m";

    protected static final String HELP_PATTERN = "\\?|h|help";
    protected static final String LIST_PATTERN = "l(?:ist)?";
    protected static final String DUMP_PATTERN = "d(?:ump)?";
    protected static final String RUN_PATTERN = "r(?:un)?";
    protected static final String EXIT_PATTERN = "(?:q|exit)";
    protected static final String SET_PATTERN = "s(?:et)?";
    protected static final String INVOC_PATTERN = "i(?:nvocation)?";
    protected static final String VERSION_PATTERN = "version";
    protected static final String REMOVE_PATTERN = "remove";
    protected static final String DEBUG_PATTERN = "debug";
    protected static final String LIST_COMMANDS_PATTERN = "c(?:ommands)?";

    protected static final String LINE_SEPARATOR = System.getProperty("line.separator");

    private static ConsoleReaderOutputStream sConsoleStream = null;

    protected ICommandScheduler mScheduler;
    protected IKeyStoreFactory mKeyStoreFactory;
    protected ConsoleReader mConsoleReader;
    private RegexTrie<Runnable> mCommandTrie = new RegexTrie<Runnable>();
    private boolean mShouldExit = false;
    private List<String> mMainArgs = new ArrayList<String>(0);
    private long mConsoleStartTime;

    /** A convenience type for <code>{@literal List<List<String>>}</code> */
    @SuppressWarnings("serial")
    protected static class CaptureList extends LinkedList<List<String>> {
        CaptureList() {
            super();
        }

        CaptureList(Collection<? extends List<String>> c) {
            super(c);
        }
    }

    /**
     * A {@link Runnable} with a {@code run} method that can take an argument
     */
    protected abstract static class ArgRunnable<T> implements Runnable {
        @Override
        public void run() {
            run(null);
        }

        abstract public void run(T args);
    }

    /**
     * This is a sentinel class that will cause TF to shut down.  This enables a user to get TF to
     * shut down via the RegexTrie input handling mechanism.
     */
    private class QuitRunnable extends ArgRunnable<CaptureList> {
        @Option(name = "handover-port", description =
            "Used to indicate that currently managed devices should be 'handed over' to new " +
            "tradefed process, which is listening on specified port")
        private Integer mHandoverPort = null;

        @Option(name = "wait-for-commands", shortName = 'c', description =
                "only exit after all commands have executed ")
        private boolean mExitOnEmpty = false;


        @Override
        public void run(CaptureList args) {
            try {
                if (args.size() >= 2 && !args.get(1).isEmpty()) {
                    List<String> optionArgs = getFlatArgs(1, args);
                    ArgsOptionParser parser = new ArgsOptionParser(this);
                    if (mKeyStoreFactory != null) {
                        parser.setKeyStore(mKeyStoreFactory.createKeyStoreClient());
                    }
                    parser.parse(optionArgs);
                }
                String exitMode = "invocations";
                if (mHandoverPort == null) {
                    if (mExitOnEmpty) {
                        exitMode = "commands";
                        mScheduler.shutdownOnEmpty();
                    } else {
                        mScheduler.shutdown();
                    }
                } else {
                    if (!mScheduler.handoverShutdown(mHandoverPort)) {
                        // failure message should already be logged
                        return;
                    }
                }
                printLine("Signalling command scheduler for shutdown.");
                printLine(String.format("TF will exit without warning when remaining %s complete.",
                        exitMode));
            } catch (ConfigurationException e) {
                printLine(e.toString());
            } catch (KeyStoreException e) {
                printLine(e.toString());
            }
        }
    }

    /**
     * Like {@link QuitRunnable}, but attempts to harshly shut down current invocations by
     * killing the adb connection
     */
    private class ForceQuitRunnable extends QuitRunnable {
        @Override
        public void run(CaptureList args) {
            super.run(args);
            mScheduler.shutdownHard();
        }
    }

    /**
     * Retrieve the {@link RegexTrie} that defines the console behavior.  Exposed for unit testing.
     */
    RegexTrie<Runnable> getCommandTrie() {
        return mCommandTrie;
    }

    /**
     * Return a new ConsoleReader, or {@code null} if an IOException occurs.  Note that this
     * function must be static so that we can run it before the superclass constructor.
     */
    protected static ConsoleReader getReader() {
        try {
            if (sConsoleStream == null) {
                final ConsoleReader reader = new ConsoleReader();
                sConsoleStream = new ConsoleReaderOutputStream(reader);
                System.setOut(new PrintStream(sConsoleStream, true));
            }
            return sConsoleStream.getConsoleReader();
        } catch (IOException e) {
            System.err.format("Failed to initialize ConsoleReader: %s\n", e.getMessage());
            return null;
        }
     }

    protected Console() {
        this(getReader());
    }

    /**
     * Create a {@link Console} with provided console reader.
     * Also, set up console command handling.
     * <p/>
     * Exposed for unit testing
     */
    Console(ConsoleReader reader) {
        super("TfConsole");
        mConsoleStartTime = System.currentTimeMillis();
        mConsoleReader = reader;
        if (reader != null) {
            mConsoleReader.addCompletor(
                    new ConfigCompletor(getConfigurationFactory().getConfigList()));
        }

        List<String> genericHelp = new LinkedList<String>();
        Map<String, String> commandHelp = new LinkedHashMap<String, String>();
        addDefaultCommands(mCommandTrie, genericHelp, commandHelp);
        setCustomCommands(mCommandTrie, genericHelp, commandHelp);
        generateHelpListings(mCommandTrie, genericHelp, commandHelp);
    }

    void setCommandScheduler(ICommandScheduler scheduler) {
        mScheduler = scheduler;
    }

    void setKeyStoreFactory(IKeyStoreFactory factory) {
        mKeyStoreFactory = factory;
    }

    /**
     * A customization point that subclasses can use to alter which commands are available in the
     * console.
     * <p />
     * Implementations should modify the {@code genericHelp} and {@code commandHelp} variables to
     * document what functionality they may have added, modified, or removed.
     *
     * @param trie The {@link RegexTrie} to add the commands to
     * @param genericHelp A {@link List} of lines to print when the user runs the "help" command
     *        with no arguments.
     * @param commandHelp A {@link Map} containing documentation for any new commands that may have
     *        been added.  The key is a regular expression to use as a key for {@link RegexTrie}.
     *        The value should be a String containing the help text to print for that command.
     */
    protected void setCustomCommands(RegexTrie<Runnable> trie, List<String> genericHelp,
            Map<String, String> commandHelp) {
        // Meant to be overridden by subclasses
    }

    /**
     * Generate help listings based on the contents of {@code genericHelp} and {@code commandHelp}.
     *
     * @param trie The {@link RegexTrie} to add the commands to
     * @param genericHelp A {@link List} of lines to print when the user runs the "help" command
     *        with no arguments.
     * @param commandHelp A {@link Map} containing documentation for any new commands that may have
     *        been added.  The key is a regular expression to use as a key for {@link RegexTrie}.
     *        The value should be a String containing the help text to print for that command.
     */
    void generateHelpListings(RegexTrie<Runnable> trie, List<String> genericHelp,
            Map<String, String> commandHelp) {
        final String genHelpString = getGenericHelpString(genericHelp);

        final ArgRunnable<CaptureList> genericHelpRunnable = new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                printLine(genHelpString);
            }
        };
        trie.put(genericHelpRunnable, HELP_PATTERN);

        StringBuilder allHelpBuilder = new StringBuilder();

        // Add help entries for everything listed in the commandHelp map
        for (Map.Entry<String, String> helpPair : commandHelp.entrySet()) {
            final String key = helpPair.getKey();
            final String helpText = helpPair.getValue();

            trie.put(new Runnable() {
                    @Override
                    public void run() {
                        printLine(helpText);
                    }
                }, HELP_PATTERN, key);

            allHelpBuilder.append(helpText);
            allHelpBuilder.append(LINE_SEPARATOR);
        }

        final String allHelpText = allHelpBuilder.toString();
        trie.put(new Runnable() {
                @Override
                public void run() {
                    printLine(allHelpText);
                }
            }, HELP_PATTERN, "all");

        // Add a generic "not found" help message for everything else
        trie.put(new ArgRunnable<CaptureList>() {
                    @Override
                    public void run(CaptureList args) {
                        // Command will be the only capture in the second argument
                        // (first argument is helpPattern)
                        printLine(String.format(
                                "No help for '%s'; command is unknown or undocumented",
                                args.get(1).get(0)));
                        genericHelpRunnable.run(args);
                    }
                }, HELP_PATTERN, null);

        // Add a fallback input handler
        trie.put(new ArgRunnable<CaptureList>() {
                    @Override
                    public void run(CaptureList args) {
                        if (args.isEmpty()) {
                            // User hit <Enter> with a blank line
                            return;
                        }

                        // Command will be the only capture in the first argument
                        printLine(String.format("Unknown command: '%s'", args.get(0).get(0)));
                        genericHelpRunnable.run(args);
                    }
                }, (Pattern)null);
    }

    /**
     * Return the generic help string to display
     *
     * @param genericHelp a list of {@link String} representing the generic help to be aggregated.
     */
    protected String getGenericHelpString(List<String> genericHelp) {
        return ArrayUtil.join(LINE_SEPARATOR, genericHelp);
    }

    /**
     * A utility function to return the arguments that were passed to an {@link ArgRunnable}.  In
     * particular, it expects all first-level elements of {@code cl} after {@code argIdx} to be
     * singleton {@link List}s.  It will then coalesce the first element of each of those singleton
     * {@link List}s as a single {@link List}.
     *
     * @param argIdx The zero-based index of the first argument.
     * @param cl The {@link CaptureList} of arguments that was passed to the {@link ArgRunnable}
     * @return A flattened {@link List} of arguments that were passed to the {@link ArgRunnable}
     * @throws IllegalArgumentException if the data isn't formatted as expected
     * @throws IndexOutOfBoundsException if {@code argIdx} isn't consistent with {@code cl}
     */
    static List<String> getFlatArgs(int argIdx, CaptureList cl) {
        if (argIdx < 0 || argIdx >= cl.size()) {
            throw new IndexOutOfBoundsException(String.format("argIdx is %d, cl size is %d",
                    argIdx, cl.size()));
        }

        List<String> flat = new ArrayList<String>(cl.size() - argIdx);
        ListIterator<List<String>> iter = cl.listIterator(argIdx);
        while (iter.hasNext()) {
            List<String> single = iter.next();
            int len = single.size();
            if (len != 1) {
                throw new IllegalArgumentException(String.format(
                        "Expected a singleton List, but got a List with %d elements: %s",
                        len, single.toString()));
            }
            flat.add(single.get(0));
        }

        return flat;
    }

    /**
     * Utility function to actually parse and execute a command file.
     */
    void runCmdfile(String cmdfileName, List<String> extraArgs) {
        try {
            mScheduler.addCommandFile(cmdfileName, extraArgs);
        } catch (ConfigurationException e) {
            printLine(String.format("Failed to run %s: %s", cmdfileName, e));
            if (mScheduler.shouldShutdownOnCmdfileError()) {
                printLine("shutdownOnCmdFileError is enabled, stopping TF");
                mScheduler.shutdown();
            }
        }
    }

    /**
     * Add commands to create the default Console experience
     * <p />
     * Adds relevant documentation to {@code genericHelp} and {@code commandHelp}.
     *
     * @param trie The {@link RegexTrie} to add the commands to
     * @param genericHelp A {@link List} of lines to print when the user runs the "help" command
     *        with no arguments.
     * @param commandHelp A {@link Map} containing documentation for any new commands that may have
     *        been added.  The key is a regular expression to use as a key for {@link RegexTrie}.
     *        The value should be a String containing the help text to print for that command.
     */
    void addDefaultCommands(RegexTrie<Runnable> trie, List<String> genericHelp,
            Map<String, String> commandHelp) {


        // Help commands
        genericHelp.add("Enter 'q' or 'exit' to exit. " +
                "Use '--wait-for-command|-c' to exit only after all commands have executed.");
        genericHelp.add("Enter 'kill' to attempt to forcibly exit, by shutting down adb");
        genericHelp.add("");
        genericHelp.add("Enter 'help all' to see all embedded documentation at once.");
        genericHelp.add("");
        genericHelp.add("Enter 'help list'       for help with 'list' commands");
        genericHelp.add("Enter 'help run'        for help with 'run' commands");
        genericHelp.add("Enter 'help invocation' for help with 'invocation' commands");
        genericHelp.add("Enter 'help dump'       for help with 'dump' commands");
        genericHelp.add("Enter 'help set'        for help with 'set' commands");
        genericHelp.add("Enter 'help remove'     for help with 'remove' commands");
        genericHelp.add("Enter 'help debug'      for help with 'debug' commands");
        genericHelp.add("Enter 'version'  to get the current version of Tradefed");

        commandHelp.put(LIST_PATTERN, String.format(
                "%s help:" + LINE_SEPARATOR +
                "\ti[nvocations]         List all invocation threads" + LINE_SEPARATOR +
                "\td[evices]             List all detected or known devices" + LINE_SEPARATOR +
                "\tc[ommands]            List all commands currently waiting to be executed" +
                LINE_SEPARATOR +
                "\tc[ommands] [pattern]  List all commands matching the pattern and currently " +
                "waiting to be executed" + LINE_SEPARATOR +
                "\tconfigs               List all known configurations" + LINE_SEPARATOR,
                LIST_PATTERN));

        commandHelp.put(DUMP_PATTERN, String.format(
                "%s help:" + LINE_SEPARATOR +
                "\ts[tack]             Dump the stack traces of all threads" + LINE_SEPARATOR +
                "\tl[ogs]              Dump the logs of all invocations to files" + LINE_SEPARATOR +
                "\tb[ugreport]         Dump a bugreport for the running Tradefed instance" +
                LINE_SEPARATOR +
                "\tc[onfig] <config>   Dump the content of the specified config" + LINE_SEPARATOR +
                "\tcommandQueue        Dump the contents of the commmand execution queue" +
                LINE_SEPARATOR +
                "\tcommands            Dump all the config XML for the commands waiting to be " +
                "executed" + LINE_SEPARATOR +
                "\tcommands [pattern]  Dump all the config XML for the commands matching the " +
                "pattern and waiting to be executed" + LINE_SEPARATOR +
                "\te[nv]               Dump the environment variables available to test harness " +
                "process" + LINE_SEPARATOR +
                "\tu[ptime]            Dump how long the TradeFed process has been running" +
                LINE_SEPARATOR,
                DUMP_PATTERN));

        commandHelp.put(RUN_PATTERN, String.format(
                "%s help:" + LINE_SEPARATOR +
                "\tcommand <config> [options]        Run the specified command" + LINE_SEPARATOR +
                "\t<config> [options]                Shortcut for the above: run specified " +
                "command" + LINE_SEPARATOR +
                "\tcmdfile <cmdfile.txt>             Run the specified commandfile" +
                LINE_SEPARATOR +
                "\tcommandAndExit <config> [options] Run the specified command, and run " +
                "'exit -c' immediately afterward" + LINE_SEPARATOR +
                "\tcmdfileAndExit <cmdfile.txt>      Run the specified commandfile, and run " +
                "'exit -c' immediately afterward" + LINE_SEPARATOR,
                RUN_PATTERN));

        commandHelp.put(SET_PATTERN, String.format(
                "%s help:" + LINE_SEPARATOR +
                "\tlog-level-display <level>  Sets the global display log level to <level>" +
                LINE_SEPARATOR,
                SET_PATTERN));

        commandHelp.put(REMOVE_PATTERN, String.format(
                "%s help:" + LINE_SEPARATOR +
                "\tremove allCommands  Remove all commands currently waiting to be executed" +
                LINE_SEPARATOR,
                REMOVE_PATTERN));

        commandHelp.put(DEBUG_PATTERN, String.format(
                "%s help:" + LINE_SEPARATOR +
                "\tgc      Attempt to force a GC" + LINE_SEPARATOR,
                DEBUG_PATTERN));

        commandHelp.put(INVOC_PATTERN, String.format(
                "%s help:" + LINE_SEPARATOR +
                "\ti[nvocation] [Command Id]        Information of the invocation thread" +
                LINE_SEPARATOR +
                "\ti[nvocation] [Command Id] stop   Notify to stop the invocation" + LINE_SEPARATOR,
                INVOC_PATTERN));

        // Handle quit commands
        trie.put(new QuitRunnable(), EXIT_PATTERN, null);
        trie.put(new QuitRunnable(), EXIT_PATTERN);
        trie.put(new ForceQuitRunnable(), "kill");

        // List commands
        trie.put(new Runnable() {
                    @Override
                    public void run() {
                        mScheduler.displayInvocationsInfo(new PrintWriter(System.out, true));
                    }
                }, LIST_PATTERN, "i(?:nvocations)?");
        trie.put(new Runnable() {
                    @Override
                    public void run() {
                        IDeviceManager manager =
                                GlobalConfiguration.getDeviceManagerInstance();
                        manager.displayDevicesInfo(new PrintWriter(System.out, true));
                    }
                }, LIST_PATTERN, "d(?:evices)?");
        trie.put(new Runnable() {
                    @Override
                    public void run() {
                        mScheduler.displayCommandsInfo(new PrintWriter(System.out, true), null);
                    }
                }, LIST_PATTERN, LIST_COMMANDS_PATTERN);
        ArgRunnable<CaptureList> listCmdRun = new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                // Skip 2 tokens to get past listPattern and "commands"
                String pattern = args.get(2).get(0);
                mScheduler.displayCommandsInfo(new PrintWriter(System.out, true), pattern);
            }
        };
        trie.put(listCmdRun, LIST_PATTERN, LIST_COMMANDS_PATTERN, "(.*)");
        trie.put(new Runnable() {
            @Override
            public void run() {
                printLine("Use 'run command <configuration_name> --help' to get list of options "
                        + "for a configuration");
                printLine("Use 'dump config <configuration_name>' to display the configuration's "
                        + "XML content.");
                printLine("");
                printLine("Available configurations include:");
                getConfigurationFactory().printHelp(System.out);
            }
        }, LIST_PATTERN, "configs");

        // Invocation commands
        trie.put(new ArgRunnable<CaptureList>() {
                    @Override
                    public void run(CaptureList args) {
                        int invocId = Integer.parseInt(args.get(1).get(0));
                        String info = mScheduler.getInvocationInfo(invocId);
                        if (info != null) {
                            printLine(String.format("invocation %s: %s", invocId, info));
                        } else {
                            printLine(String.format("No information found for invocation %s.",
                                    invocId));
                        }
                    }
        }, INVOC_PATTERN, "([0-9]*)");
        trie.put(new ArgRunnable<CaptureList>() {
                    @Override
                    public void run(CaptureList args) {
                        int invocId = Integer.parseInt(args.get(1).get(0));
                        if (mScheduler.stopInvocation(invocId)) {
                            printLine(String.format("Invocation %s has been requested to stop."
                                    + " It may take some times.",
                                    invocId));
                        } else {
                            printLine(String.format("Could not stop invocation %s, try 'list "
                                    + "invocation' or 'invocation %s' for more information.",
                                    invocId, invocId));
                        }
                    }
        }, INVOC_PATTERN, "([0-9]*)", "stop");

        // Dump commands
        trie.put(new Runnable() {
                    @Override
                    public void run() {
                        dumpStacks(System.out);
                    }
                }, DUMP_PATTERN, "s(?:tacks?)?");
        trie.put(new Runnable() {
                    @Override
                    public void run() {
                        dumpLogs();
                    }
                }, DUMP_PATTERN, "l(?:ogs?)?");
        trie.put(new Runnable() {
                    @Override
                    public void run() {
                        dumpTfBugreport();
                    }
        }, DUMP_PATTERN, "b(?:ugreport?)?");
        trie.put(new Runnable() {
            @Override
            public void run() {
                printElapsedTime();
            }
        }, DUMP_PATTERN, "u(?:ptime?)?");
        ArgRunnable<CaptureList> dumpConfigRun = new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                // Skip 2 tokens to get past dumpPattern and "config"
                String configArg = args.get(2).get(0);
                getConfigurationFactory().dumpConfig(configArg, System.out);
            }
        };
        trie.put(dumpConfigRun, DUMP_PATTERN, "c(?:onfig?)?", "(.*)");

        trie.put(new Runnable() {
            @Override
            public void run() {
                mScheduler.displayCommandQueue(new PrintWriter(System.out, true));
            }
        }, DUMP_PATTERN, "commandQueue");

        trie.put(new Runnable() {
            @Override
            public void run() {
                mScheduler.dumpCommandsXml(new PrintWriter(System.out, true), null);
            }
        }, DUMP_PATTERN, LIST_COMMANDS_PATTERN);
        ArgRunnable<CaptureList> dumpCmdRun = new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                // Skip 2 tokens to get past listPattern and "commands"
                String pattern = args.get(2).get(0);
                mScheduler.dumpCommandsXml(new PrintWriter(System.out, true), pattern);
            }
        };
        trie.put(dumpCmdRun, DUMP_PATTERN, LIST_COMMANDS_PATTERN, "(.*)");

        trie.put(new Runnable() {
            @Override
            public void run() {
                dumpEnv();
            }
        }, DUMP_PATTERN, "e(?:nv)?");

        // Run commands
        ArgRunnable<CaptureList> runRunCommand = new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                // The second argument "command" may also be missing, if the
                // caller used the shortcut.
                int startIdx = 1;
                if (args.get(1).isEmpty()) {
                    // Empty array (that is, not even containing an empty string) means that
                    // we matched and skipped /(?:singleC|c)ommand/
                    startIdx = 2;
                }

                String[] flatArgs = new String[args.size() - startIdx];
                for (int i = startIdx; i < args.size(); i++) {
                    flatArgs[i - startIdx] = args.get(i).get(0);
                }
                try {
                    mScheduler.addCommand(flatArgs);
                } catch (ConfigurationException e) {
                    printLine("Failed to run command: " + e.toString());
                }
            }
        };
        trie.put(runRunCommand, RUN_PATTERN, "c(?:ommand)?", null);
        trie.put(runRunCommand, RUN_PATTERN, null);
        trie.put(new Runnable() {
            @Override
            public void run() {
               String version = VersionParser.fetchVersion();
               if (version != null) {
                   printLine(version);
               } else {
                   printLine("Failed to fetch version information for Tradefed.");
               }
            }
         }, VERSION_PATTERN);

        ArgRunnable<CaptureList> runAndExitCommand = new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                // Skip 2 tokens to get past runPattern and "singleCommand"
                String[] flatArgs = new String[args.size() - 2];
                for (int i = 2; i < args.size(); i++) {
                    flatArgs[i - 2] = args.get(i).get(0);
                }
                try {
                    if (mScheduler.addCommand(flatArgs)) {
                        mScheduler.shutdownOnEmpty();
                    }
                } catch (ConfigurationException e) {
                    printLine("Failed to run command: " + e.toString());
                }

                // Intentionally kill the console before CommandScheduler finishes
                mShouldExit = true;
            }
        };
        trie.put(runAndExitCommand, RUN_PATTERN, "s(?:ingleCommand)?", null);
        trie.put(runAndExitCommand, RUN_PATTERN, "commandAndExit", null);

        // Missing required argument: show help
        // FIXME: fix this functionality
        // trie.put(runHelpRun, runPattern, "(?:singleC|c)ommand");

        final ArgRunnable<CaptureList> runRunCmdfile = new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                // Skip 2 tokens to get past runPattern and "cmdfile".  We're guaranteed to have at
                // least 3 tokens if we got #run.
                int startIdx = 2;
                List<String> flatArgs = getFlatArgs(startIdx, args);
                String file = flatArgs.get(0);
                List<String> extraArgs = flatArgs.subList(1, flatArgs.size());
                printLine(String.format("Attempting to run cmdfile %s with args %s", file,
                        extraArgs.toString()));
                runCmdfile(file, extraArgs);
            }
        };
        trie.put(runRunCmdfile, RUN_PATTERN, "cmdfile", "(.*)");
        trie.put(runRunCmdfile, RUN_PATTERN, "cmdfile", "(.*)", null);

        ArgRunnable<CaptureList> runRunCmdfileAndExit = new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                runRunCmdfile.run(args);
                mScheduler.shutdownOnEmpty();
            }
        };
        trie.put(runRunCmdfileAndExit, RUN_PATTERN, "cmdfileAndExit", "(.*)");
        trie.put(runRunCmdfileAndExit, RUN_PATTERN, "cmdfileAndExit", "(.*)", null);

        ArgRunnable<CaptureList> runRunAllCmdfilesAndExit = new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                // skip 2 tokens to get past runPattern and "allCmdfilesAndExit"
                if (args.size() <= 2) {
                    printLine("No cmdfiles specified!");
                } else {
                    // Each group should have exactly one element, given how the null wildcard
                    // operates; so we flatten them.
                    for (String cmdfile : getFlatArgs(2 /* startIdx */, args)) {
                        runCmdfile(cmdfile, new ArrayList<String>(0));
                    }
                }
                mScheduler.shutdownOnEmpty();
            }
        };
        trie.put(runRunAllCmdfilesAndExit, RUN_PATTERN, "allCmdfilesAndExit");
        trie.put(runRunAllCmdfilesAndExit, RUN_PATTERN, "allCmdfilesAndExit", null);

        // Missing required argument: show help
        // FIXME: fix this functionality
        //trie.put(runHelpRun, runPattern, "cmdfile");

        // Set commands
        ArgRunnable<CaptureList> runSetLog = new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                // Skip 2 tokens to get past "set" and "log-level-display"
                String logLevelStr = args.get(2).get(0);
                LogLevel newLogLevel = LogLevel.getByString(logLevelStr);
                LogLevel currentLogLevel = LogRegistry.getLogRegistry().getGlobalLogDisplayLevel();
                if (newLogLevel != null) {
                    LogRegistry.getLogRegistry().setGlobalLogDisplayLevel(newLogLevel);
                    // Make sure that the level was set.
                    currentLogLevel = LogRegistry.getLogRegistry().getGlobalLogDisplayLevel();
                    if (currentLogLevel != null) {
                        printLine(String.format("Log level now set to '%s'.", currentLogLevel));
                    }
                } else {
                    if (currentLogLevel == null) {
                        printLine(String.format("Invalid log level '%s'.", newLogLevel));
                    } else{
                        printLine(String.format(
                                "Invalid log level '%s'; log level remains at '%s'.",
                                newLogLevel, currentLogLevel));
                    }
                }
            }
        };
        trie.put(runSetLog, SET_PATTERN, "log-level-display", "(.*)");

        // Debug commands
        trie.put(new Runnable() {
                    @Override
                    public void run() {
                        System.gc();
                    }
                }, DEBUG_PATTERN, "gc");

        // Remove commands
        trie.put(new Runnable() {
                    @Override
                    public void run() {
                        mScheduler.removeAllCommands();
                    }
                }, REMOVE_PATTERN, "allCommands");
    }

    /**
     * Print the uptime of the Tradefed process.
     */
    private void printElapsedTime() {
        long elapsedTime = System.currentTimeMillis() - mConsoleStartTime;
        String elapsed = String.format("TF has been running for %s",
                TimeUtil.formatElapsedTime(elapsedTime));
        printLine(elapsed);
    }

    /**
     * Get input from the console
     *
     * @return A {@link String} containing the input to parse and run. Will return {@code null} if
     *     console is not available or user entered EOF ({@code ^D}).
     */
    @VisibleForTesting
    String getConsoleInput() throws IOException {
        if (mConsoleReader != null) {
            if (sConsoleStream != null) {
                // While we're reading the console, the only tasks which will print to the console
                // are asynchronous.  In particular, after this point, we assume that the last line
                // on the screen is the command prompt.
                sConsoleStream.setAsyncMode();
            }

            final String input = mConsoleReader.readLine(getConsolePrompt());

            if (sConsoleStream != null) {
                // The opposite of the above.  From here on out, we should expect that the
                // command prompt is _not_ the most recent line on the screen.  In particular, while
                // synchronous tasks are running, sConsoleStream will avoid redisplaying the command
                // prompt.
                sConsoleStream.setSyncMode();
            }
            return input;
        } else {
            return null;
        }
    }

    /**
     * @return the text {@link String} to display for the console prompt
     */
    protected String getConsolePrompt() {
        return CONSOLE_PROMPT;
    }

    /**
     * Display a line of text on console
     * @param output
     */
    protected void printLine(String output) {
        System.out.print(output);
        System.out.println();
    }

    /**
     * Print the line to a Printwriter
     * @param output
     */
    protected void printLine(String output, PrintStream pw) {
        pw.print(output);
        pw.println();
    }

    /**
     * Execute a command.
     * <p />
     * Exposed for unit testing
     */
    @SuppressWarnings("unchecked")
    void executeCmdRunnable(Runnable command, CaptureList groups) {
        try {
            if (command instanceof ArgRunnable) {
                // FIXME: verify that command implements ArgRunnable<CaptureList> instead
                // FIXME: of just ArgRunnable
                ((ArgRunnable<CaptureList>) command).run(groups);
            } else {
                command.run();
            }
        } catch (RuntimeException e) {
            e.printStackTrace();
        }
    }

    /**
     * Return whether we should expect the console to be usable.
     * <p />
     * Exposed for unit testing.
     */
    boolean isConsoleFunctional() {
        return System.console() != null;
    }

    /**
     * The main method to launch the console. Will keep running until shutdown command is issued.
     */
    @Override
    public void run() {
        List<String> arrrgs = mMainArgs;

        if (mScheduler == null) {
            throw new IllegalStateException("command scheduler hasn't been set");
        }

        try {
            // Check System.console() since jline doesn't seem to consistently know whether or not
            // the console is functional.
            if (!isConsoleFunctional()) {
                if (arrrgs.isEmpty()) {
                    printLine("No commands for non-interactive mode; exiting.");
                    // FIXME: need to run the scheduler here so that the things blocking on it
                    // FIXME: will be released.
                    mScheduler.start();
                    mScheduler.await();
                    return;
                } else {
                    printLine("Non-interactive mode: Running initial command then exiting.");
                    mShouldExit = true;
                }
            }

            // Wait for the CommandScheduler to start.  It will hold the JVM open (since the Console
            // thread is a Daemon thread), and also we require it to have started so that we can
            // start processing user input.
            mScheduler.start();
            mScheduler.await();

            String input = "";
            CaptureList groups = new CaptureList();
            String[] tokens;

            // Note: since Console is a daemon thread, the JVM may exit without us actually leaving
            // this read loop.  This is by design.
            do {
                if (arrrgs.isEmpty()) {
                    input = getConsoleInput();

                    if (input == null) {
                        // Usually the result of getting EOF on the console
                        printLine("");
                        printLine("Received EOF; quitting...");
                        mShouldExit = true;
                        break;
                    }

                    tokens = null;
                    try {
                        tokens = QuotationAwareTokenizer.tokenizeLine(input);
                    } catch (IllegalArgumentException e) {
                        printLine(String.format("Invalid input: %s.", input));
                        continue;
                    }

                    if (tokens == null || tokens.length == 0) {
                        continue;
                    }
                } else {
                    printLine(String.format("Using commandline arguments as starting command: %s",
                            arrrgs));
                    if (mConsoleReader != null) {
                        // Add the starting command as the first item in the console history
                        // FIXME: this will not properly escape commands that were properly escaped
                        // FIXME: on the commandline.  That said, it will still be more convenient
                        // FIXME: than copying by hand.
                        final String cmd = ArrayUtil.join(" ", arrrgs);
                        mConsoleReader.getHistory().addToHistory(cmd);
                    }
                    tokens = arrrgs.toArray(new String[0]);
                    if (arrrgs.get(0).matches(HELP_PATTERN)) {
                        // if started from command line for help, return to shell
                        mShouldExit = true;
                    }
                    arrrgs = Collections.emptyList();
                }

                Runnable command = mCommandTrie.retrieve(groups, tokens);
                if (command != null) {
                    executeCmdRunnable(command, groups);
                } else {
                    printLine(String.format(
                            "Unable to handle command '%s'.  Enter 'help' for help.", tokens[0]));
                }
                RunUtil.getDefault().sleep(100);
            } while (!mShouldExit);
        } catch (Exception e) {
            printLine("Console received an unexpected exception (shown below); shutting down TF.");
            e.printStackTrace();
        } finally {
            mScheduler.shutdown();
            // Make sure that we don't quit with messages still in the buffers
            System.err.flush();
            System.out.flush();
        }
    }

    /**
     * set the flag to exit the console.
     */
    @VisibleForTesting
    void exitConsole() {
        mShouldExit = true;
    }

    void awaitScheduler() throws InterruptedException {
        mScheduler.await();
    }

    /**
     * Method for getting a {@link IConfigurationFactory}.
     * <p/>
     * Exposed for unit testing.
     */
    IConfigurationFactory getConfigurationFactory() {
        return ConfigurationFactory.getInstance();
    }

    private void dumpStacks(PrintStream ps) {
        Map<Thread, StackTraceElement[]> threadMap = Thread.getAllStackTraces();
        for (Map.Entry<Thread, StackTraceElement[]> threadEntry : threadMap.entrySet()) {
            dumpThreadStack(threadEntry.getKey(), threadEntry.getValue(), ps);
        }
    }

    private void dumpThreadStack(Thread thread, StackTraceElement[] trace, PrintStream ps) {
        printLine(String.format("%s", thread), ps);
        for (int i=0; i < trace.length; i++) {
            printLine(String.format("\t%s", trace[i]), ps);
        }
        printLine("", ps);
    }

    private void dumpLogs() {
        LogRegistry.getLogRegistry().dumpLogs();
    }

    /**
     * Dumps the environment variables to console, sorted by variable names
     */
    private void dumpEnv() {
        // use TreeMap to sort variables by name
        Map<String, String> env = new TreeMap<>(System.getenv());
        for (Map.Entry<String, String> entry : env.entrySet()) {
            printLine(String.format("\t%s=%s", entry.getKey(), entry.getValue()));
        }
    }

    /**
     * Dump a Tradefed Bugreport containing the stack traces and logs.
     */
    private void dumpTfBugreport() {
        File tmpBugreportDir = null;
        PrintStream ps = null;
        try {
            // dump stacks
            tmpBugreportDir = FileUtil.createNamedTempDir("bugreport_tf");
            File tmpStackFile = FileUtil.createTempFile("dump_stacks_", ".log", tmpBugreportDir);
            ps = new PrintStream(tmpStackFile);
            dumpStacks(ps);
            ps.flush();
            // dump logs
            ((LogRegistry)LogRegistry.getLogRegistry()).dumpLogsToDir(tmpBugreportDir);
            // add them to a zip and log.
            File zippedBugreport = ZipUtil.createZip(tmpBugreportDir, "tradefed_bugreport_");
            printLine(String.format("Output bugreport zip in %s",
                    zippedBugreport.getAbsolutePath()));
        } catch (IOException io) {
            printLine("Error when trying to dump bugreport");
        } finally {
            ps.close();
            FileUtil.recursiveDelete(tmpBugreportDir);
        }
    }

    /**
     * Sets the console starting arguments.
     *
     * @param mainArgs the arguments
     */
    public void setArgs(List<String> mainArgs) {
        mMainArgs = mainArgs;
    }

    public static void main(final String[] mainArgs) throws InterruptedException,
            ConfigurationException {
        Console console = new Console();
        startConsole(console, mainArgs);
    }

    /**
     * Starts the given Tradefed console with given args
     *
     * @param console the {@link Console} to start
     * @param args the command line arguments
     */
    public static void startConsole(Console console, String[] args) throws InterruptedException,
            ConfigurationException {
        List<String> nonGlobalArgs = GlobalConfiguration.createGlobalConfiguration(args);

        console.setArgs(nonGlobalArgs);
        console.setCommandScheduler(GlobalConfiguration.getInstance().getCommandScheduler());
        console.setKeyStoreFactory(GlobalConfiguration.getInstance().getKeyStoreFactory());
        console.setDaemon(true);
        console.start();

        // Wait for the CommandScheduler to get started before we exit the main thread.  See full
        // explanation near the top of #run()
        console.awaitScheduler();
    }
}
