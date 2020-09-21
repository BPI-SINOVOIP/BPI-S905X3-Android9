/*
 * Copyright (C) 2013 The Android Open Source Project
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

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * A simple class to watch a set of command files for changes, and to trigger a
 * reload of _all_ manually-loaded command files when such a change happens.
 */
class CommandFileWatcher extends Thread {
    private static final long POLL_TIME_MS = 20 * 1000;  // 20 seconds
    // thread-safe (for read-writes, not write during iteration) structure holding all commands
    // being watched. map of absolute file system path to command file
    private Map<String, CommandFile> mCmdFileMap = new Hashtable<>();
    boolean mCancelled = false;
    private final ICommandFileListener mListener;

    static interface ICommandFileListener {
        public void notifyFileChanged(File cmdFile, List<String> extraArgs);
    }

    /**
     * A simple struct to store a command file as well as its extra args
     */
    static class CommandFile {
        public final File file;
        public final long modTime;
        public final List<String> extraArgs;
        public final List<CommandFile> dependencies;

        /**
         * Construct a CommandFile with no arguments and no dependencies
         *
         * @param cmdFile a {@link File} representing the command file path
         */
        public CommandFile(File cmdFile) {
            if (cmdFile == null) throw new NullPointerException();

            this.file = cmdFile;
            this.modTime = cmdFile.lastModified();

            this.extraArgs = Collections.emptyList();
            this.dependencies = Collections.emptyList();
        }

        /**
         * Construct a CommandFile
         *
         * @param cmdFile a {@link File} representing the command file path
         * @param extraArgs A {@link List} of extra arguments that should be
         *        used when the command is rerun.
         * @param dependencies The command files that this command file
         *        requires as transitive dependencies.  A change in any of the
         *        dependencies will trigger a reload, but none of the
         *        dependencies themselves will be reloaded directly, only the
         *        main command file, {@code cmdFile}.
         */
        public CommandFile(File cmdFile, List<String> extraArgs, List<File> dependencies) {
            if (cmdFile == null) throw new NullPointerException();

            this.file = cmdFile;
            this.modTime = cmdFile.lastModified();

            if (extraArgs == null) {
                this.extraArgs = Collections.emptyList();
            } else {
                this.extraArgs = extraArgs;
            }
            if (dependencies == null) {
                this.dependencies = Collections.emptyList();

            } else {
                this.dependencies = new ArrayList<CommandFile>(dependencies.size());
                for (File f: dependencies) {
                    this.dependencies.add(new CommandFile(f));
                }
            }
        }
    }

    public CommandFileWatcher(ICommandFileListener listener) {
        super("CommandFileWatcher");  // set the thread name
        mListener = listener;
        setDaemon(true);  // Don't keep the JVM alive for this thread
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run() {
        while (!isCancelled()) {
            checkForUpdates();
            getRunUtil().sleep(POLL_TIME_MS);
        }
    }

    /**
     * Same as {@link #addCmdFile(File, List, Collection)} but accepts a list of {@link File}s
     * as dependencies
     */
    @VisibleForTesting
    void addCmdFile(File cmdFile, List<String> extraArgs, List<File> dependencies) {
        CommandFile f = new CommandFile(cmdFile, extraArgs, dependencies);
        mCmdFileMap.put(cmdFile.getAbsolutePath(), f);
    }

    /**
     * Returns true if given command gile path is currently being watched
     */
    public boolean isFileWatched(File cmdFile) {
        return mCmdFileMap.containsKey(cmdFile.getAbsolutePath());
    }

    /**
     * Terminate the watcher thread
     */
    public void cancel() {
        mCancelled = true;
        interrupt();
    }

    /**
     * Check if the thread has been signalled to stop.
     */
    public boolean isCancelled() {
        return mCancelled;
    }

    /**
     * Poll the filesystem to see if any of the files of interest have
     * changed
     * <p />
     * Exposed for unit testing
     */
    void checkForUpdates() {
        final Set<File> checkedFiles = new HashSet<File>();

        // iterate through a copy of the command list to limit time lock needs to be held
        List<CommandFile> cmdCopy;
        synchronized (mCmdFileMap) {
            cmdCopy = new ArrayList<CommandFile>(mCmdFileMap.values());
        }
        for (CommandFile cmd : cmdCopy) {
            if (checkCommandFileForUpdate(cmd, checkedFiles)) {
                mListener.notifyFileChanged(cmd.file, cmd.extraArgs);
            }
        }
    }

    boolean checkCommandFileForUpdate(CommandFile cmd, Set<File> checkedFiles) {
        if (checkedFiles.contains(cmd.file)) {
            return false;
        } else {
            checkedFiles.add(cmd.file);
        }

        final long curModTime = cmd.file.lastModified();
        if (curModTime == 0L) {
            // File doesn't exist, or had an IO error.  Don't do anything.  If a change occurs
            // that we should pay attention to, then we'll see the file actually updated, which
            // implies that the modtime will be non-zero and will also be different from what
            // we stored before.
        } else if (curModTime != cmd.modTime) {
            // Note that we land on this case if the original modtime was 0 and the modtime is
            // now non-zero, so there's a race-condition if an IO error causes us to fail to
            // read the modtime initially.  This should be okay.
            CLog.w("Found update in monitored cmdfile %s (%d -> %d)", cmd.file, cmd.modTime,
                    curModTime);
            return true;
        }

        // Now check dependencies
        for (CommandFile dep : cmd.dependencies) {
            if (checkCommandFileForUpdate(dep, checkedFiles)) {
                // dependency changed
                return true;
            }
        }

        // We didn't change, and nor did any of our dependencies
        return false;
    }

    /**
     * Factory method for creating a {@link CommandFileParser}.
     * <p/>
     * Exposed for unit testing.
     */
    CommandFileParser createCommandFileParser() {
        return new CommandFileParser();
    }

    /**
     * Utility method to fetch the default {@link IRunUtil} singleton
     * <p />
     * Exposed for unit testing.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * <p>
     * Add a command file to watch, as well as its dependencies.  When either
     * the command file itself or any of its dependencies changes, notify the registered
     * {@link ICommandFileListener}
     * </p>
     * if the cmdFile is already being watching, this call will replace the current entry
     */
    public void addCmdFile(File cmdFile, List<String> extraArgs, Collection<String> includedFiles) {
        List<File> includesAsFiles = new ArrayList<File>(includedFiles.size());
        for (String p : includedFiles) {
            includesAsFiles.add(new File(p));
        }
        addCmdFile(cmdFile, extraArgs, includesAsFiles);
    }

    /**
     * Remove all files from the watched list
     */
    public void removeAllFiles() {
        mCmdFileMap.clear();
    }

    /**
     * Retrieves the extra arguments associated with given file being watched.
     * <p>
     * TODO: extra args list should likely be stored elsewhere, and have this class just operate
     * as a generic file watcher with dependencies
     * </p>
     * @return the list of extra arguments associated with command file. Returns empty list if
     *         command path is not recognized
     */
    public List<String> getExtraArgsForFile(String cmdPath) {
        CommandFile cmdFile = mCmdFileMap.get(cmdPath);
        if (cmdFile != null) {
            return cmdFile.extraArgs;
        }
        CLog.w("Could not find cmdfile %s", cmdPath);
        return Collections.<String>emptyList();
    }
}
