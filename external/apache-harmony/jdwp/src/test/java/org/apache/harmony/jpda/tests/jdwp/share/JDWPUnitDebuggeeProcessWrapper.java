/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @author Vitaly A. Provodin
 */

package org.apache.harmony.jpda.tests.jdwp.share;

import java.lang.reflect.Field;

import java.io.*;
import java.nio.file.*;

import java.io.IOException;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.framework.StreamRedirector;
import org.apache.harmony.jpda.tests.framework.TestErrorException;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPDebuggeeWrapper;
import org.apache.harmony.jpda.tests.share.JPDATestOptions;

/**
 * This class provides basic DebuggeeWrapper implementation based on JUnit framework,
 * which can launch and control debuggee process.
 */
public class JDWPUnitDebuggeeProcessWrapper extends JDWPDebuggeeWrapper {

    /**
     * Target VM debuggee process.
     */
    public Process process;

    protected StreamRedirector errRedir;
    protected StreamRedirector outRedir;

    /**
     * The expected exit code for the debuggee process.
     */
    private int expectedExitCode = 0;

    /**
     * Creates new instance with given data.
     * 
     * @param settings
     *            test run options
     * @param logWriter
     *            where to print log messages
     */
    public JDWPUnitDebuggeeProcessWrapper(JPDATestOptions settings, LogWriter logWriter) {
        super(settings, logWriter);
    }

    /**
     * Sets the expected exit code. This is meant to be used by tests that will request target
     * VM termination with VirtualMachine.Exit command.
     */
    public void setExpectedExitCode(int expectedExitCode) {
        this.expectedExitCode = expectedExitCode;
    }

    /**
     * Launches process and redirects output.
     */
    public void launchProcessAndRedirectors(String cmdLine) throws IOException {
        logWriter.println("Launch process: " + cmdLine);
        process = launchProcess(cmdLine);
        logWriter.println("Launched process");
        if (process != null) {
            logWriter.println("Start redirectors");
            errRedir = new StreamRedirector(process.getErrorStream(), logWriter, "STDERR");
            errRedir.setDaemon(true);
            errRedir.start();
            outRedir = new StreamRedirector(process.getInputStream(), logWriter, "STDOUT");
            outRedir.setDaemon(true);
            outRedir.start();
            logWriter.println("Started redirectors");
        }
    }

    /**
     * Waits for process to exit and closes output redirectors
     */
    public void finishProcessAndRedirectors() {
        if (process != null) {
            try {
                logWriter.println("Waiting for process exit");
                WaitForProcessExit(process);
                logWriter.println("Finished process");
            } catch (IOException e) {
                throw new TestErrorException("IOException in waiting for process exit: ", e);
            }

            logWriter.println("Waiting for redirectors finish");
            if (outRedir != null) {
                outRedir.exit();
                try {
                    outRedir.join(settings.getTimeout());
                } catch (InterruptedException e) {
                    logWriter.println("InterruptedException in stopping outRedirector: " + e);
                }
                if (outRedir.isAlive()) {
                    logWriter.println("WARNING: redirector not stopped: " + outRedir.getName());
                }
            }
            if (errRedir != null) {
                errRedir.exit();
                try {
                    errRedir.join(settings.getTimeout());
                } catch (InterruptedException e) {
                    logWriter.println("InterruptedException in stopping errRedirector: " + e);
                }
                if (errRedir.isAlive()) {
                    logWriter.println("WARNING: redirector not stopped: " + errRedir.getName());
                }
            }
            logWriter.println("Finished redirectors");
        }
    }

    /**
     * Launches process with given command line.
     * 
     * @param cmdLine
     *            command line
     * @return associated Process object or null if not available
     * @throws IOException
     *             if error occurred in launching process
     */
    protected Process launchProcess(String cmdLine) throws IOException {

    	// Runtime.exec(String) does not preserve quoted arguments
    	// process = Runtime.getRuntime().exec(cmdLine);

    	String args[] = splitCommandLine(cmdLine);
    	process = Runtime.getRuntime().exec(args);
        return process;
    }

    /**
     * Splits command line into arguments preserving spaces in quoted arguments
     * either with single and double quotes (not prefixed by '\').
     *
     * @param cmd
     *            command line
     * @return associated Process object or null if not available
     */
/*
    public String[] splitCommandLine(String cmd) {

        // allocate array for parsed arguments
        int max_argc = 250;
        Vector argv = new Vector();

        // parse command line
        int len = cmd.length();
        if (len > 0) {
            for (int arg = 0; arg < len;) {
                // skip initial spaces
                while (Character.isWhitespace(cmd.charAt(arg))) arg++;
                // parse non-spaced or quoted argument 
                for (int p = arg; ; p++) {
                    // check for ending separator
                    if (p >= len || Character.isWhitespace(cmd.charAt(p))) {
                    	if (p > len) p = len;
                    	String val = cmd.substring(arg, p);
                        argv.add(val);
                        arg = p + 1;
                        break;
                    }     

                    // check for starting quote
                    if (cmd.charAt(p) == '\"') {
                         char quote = cmd.charAt(p++);
                         // skip all chars until terminating quote or end of line
                         for (; p < len; p++) {
                             // check for terminating quote
                             if (cmd.charAt(p) == quote) 
                            	 break;
                             // skip escaped quote
                             if (cmd.charAt(p) == '\\' && (p+1) < len && cmd.charAt(p+1) == quote) 
                            	 p++;
                         }
                     }

                    // skip escaped quote
                    if (cmd.charAt(p) == '\\' && (p+1) < len && cmd.charAt(p+1) == '\"') { 
                    	p++;
                    }
                }
            }
        }

    	logWriter.println("Splitted command line: " + argv);
        int size = argv.size();
        String args[] = new String[size];
        return (String[])argv.toArray(args);
	}
*/
    public String[] splitCommandLine(String cmd) {

        int len = cmd.length();
        char chars[] = new char[len];
        Vector<String> argv = new Vector<String>();

        if (len > 0) {
            for (int arg = 0; arg < len;) {
                // skip initial spaces
                while (Character.isWhitespace(cmd.charAt(arg))) arg++;
                // parse non-spaced or quoted argument 
                for (int p = arg, i = 0; ; p++) {
                    // check for starting quote
                    if (p < len && (cmd.charAt(p) == '\"' || cmd.charAt(p) == '\'')) {
                         char quote = cmd.charAt(p++);
                         // copy all chars until terminating quote or end of line
                         for (; p < len; p++) {
                             // check for terminating quote
                             if (cmd.charAt(p) == quote) { 
                            	 p++;
                            	 break;
                             }
                             // preserve escaped quote
                             if (cmd.charAt(p) == '\\' && (p+1) < len && cmd.charAt(p+1) == quote) 
                            	 p++;
                             chars[i++] = cmd.charAt(p);
                         }
                     }

                    // check for ending separator
                    if (p >= len || Character.isWhitespace(cmd.charAt(p))) {
                    	String val = new String(chars, 0, i);
                        argv.add(val);
                        arg = p + 1;
                        break;
                    }     

                    // preserve escaped quote
                    if (cmd.charAt(p) == '\\' && (p+1) < len 
                    		&& (cmd.charAt(p+1) == '\"' || cmd.charAt(p+1) == '\'')) { 
                    	p++;
                    }

                    // copy current char
                    chars[i++] = cmd.charAt(p);
                }
            }
        }

    	logWriter.println("Splitted command line: " + argv);
        int size = argv.size();
        String args[] = new String[size];
        return argv.toArray(args);
	}

    /**
     * Waits for launched process to exit.
     * 
     * @param process
     *            associated Process object or null if not available
     * @throws IOException
     *             if any exception occurs in waiting
     */
    protected void WaitForProcessExit(Process process) throws IOException {
        ProcessWaiter thrd = new ProcessWaiter();
        thrd.setDaemon(true);
        thrd.start();
        try {
            thrd.join(settings.getTimeout());
        } catch (InterruptedException e) {
            throw new TestErrorException(e);
        }

        if (thrd.isAlive()) {
            // ProcessWaiter thread is still running (after we wait until a timeout) but is
            // waiting for the debuggee process to exit. We send an interrupt request to
            // that thread so it receives an InterrupedException and terminates.
            thrd.interrupt();
        }

        try {
            int exitCode = process.exitValue();
            logWriter.println("Finished debuggee with exit code: " + exitCode);
            if (exitCode != expectedExitCode) {
                throw new TestErrorException("Debuggee exited with code " + exitCode +
                        " but we expected code " + expectedExitCode);
            }
        } catch (IllegalThreadStateException e) {
            logWriter.println("Terminate debuggee process with " + e);
            GetRemoteStackTrace(process);
            throw new TestErrorException("Debuggee process did not finish during timeout", e);
        } finally {
            // dispose any resources of the process
            process.destroy();
        }
    }

    private int GetRealPid(Process process) {
      int initial_pid = GetInitialPid(process);
      logWriter.println("initial pid: " + initial_pid);
      String[] names = new String[] { "java", "dalvikvm", "dalvikvm32", "dalvikvm64" };
      return FindPidFor(Paths.get("/proc").resolve(Integer.toString(initial_pid)), names);
    }

    private int getPid(Path proc) throws IOException {
      try {
        // See man 5 proc for information on stat file. All we really care about is that it is a
        // list of various pieces of information about the process separated by ' '. The first of
        // these is the PID.
        return Integer.valueOf(new String(Files.readAllBytes(proc.resolve("stat"))).split(" ")[0]);
      } catch (IOException e) {
        return -1;
      }
    }

    private int FindPidFor(Path cur, String[] names) {
      try {
        if (!cur.toFile().exists()) {
          return -1;
        }
        int pid = getPid(cur);
        if (Arrays.stream(names).anyMatch(
            (x) -> {
              try {
                return cur.resolve("exe").toRealPath().endsWith(x);
              } catch (Exception e) {
                return false;
              }
            })) {
          return pid;
        } else {
          Path children = cur.resolve("task").resolve(Integer.toString(pid)).resolve("children");
          if (!children.toFile().exists()) {
            return -1;
          } else {
            for (String child_pid : ReadChildPids(children.toFile())) {
              logWriter.println("Examining pid: " + child_pid);
              int res = FindPidFor(Paths.get("/proc").resolve(child_pid), names);
              if (res != -1) {
                return res;
              }
            }
            return -1;
          }
        }
      } catch (Exception e) {
        return -1;
      }
    }

    private String[] ReadChildPids(File children) throws Exception {
      if (!children.exists()) {
        return new String[0];
      } else {
        return new String(Files.readAllBytes(children.toPath())).split(" ");
      }
    }

    private int GetInitialPid(Process process) {
      try {
        Class<?> unix_process_class = Class.forName("java.lang.UNIXProcess");
        if (!process.getClass().equals(unix_process_class)) {
          throw new TestErrorException("Process is of unexpected type. Timeout happened.");
        }
        Field pid = unix_process_class.getDeclaredField("pid");
        pid.setAccessible(true);
        return (int)pid.get(process);
      } catch (ReflectiveOperationException e) {
        throw new TestErrorException("Unable to get pid from process to debug timeout!", e);
      }
    }

    private void GetRemoteStackTrace(Process process) throws IOException {
      String pid = Integer.toString(GetRealPid(process));
      logWriter.println("trying to dump " + pid);
      List<String> cmd = new ArrayList<>(Arrays.asList(splitCommandLine(settings.getDumpProcessCommand())));
      cmd.add(pid);
      logWriter.println("command: " + cmd);

      ProcessBuilder b = new ProcessBuilder(cmd).redirectErrorStream(true);
      Process p = null;
      StreamRedirector out = null;
      try {
        p = b.start();
        out = new StreamRedirector(p.getInputStream(), logWriter, "dump-" + pid);
        out.setDaemon(true);
        out.start();
        p.waitFor();
        // Wait 5 seconds for the streams to catch up.
        Thread.sleep(5000);
      } catch (Exception e) {
        throw new TestErrorException("Unable to get process dump!", e);
      } finally {
        if (p != null) {
          p.destroy();
        }
        if (out != null) {
          try {
            out.exit();
            out.join();
          } catch (Exception e) {
            logWriter.println("Suppressing error: " + e);
          }
        }
      }
    }

    /**
     * Separate thread for waiting for process exit for specified timeout.
     */
    class ProcessWaiter extends Thread {
        @Override
        public void run() {
            try {
                process.waitFor();
            } catch (InterruptedException e) {
                logWriter.println("Ignoring exception in ProcessWaiter thread interrupted: " + e);
            }
        }
    }

    @Override
    public void start() {
    }

    @Override
    public void stop() {
    }
}
