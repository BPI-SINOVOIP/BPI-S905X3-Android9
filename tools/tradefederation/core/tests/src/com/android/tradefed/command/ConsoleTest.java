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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.command.Console.CaptureList;
import com.android.tradefed.util.RegexTrie;
import com.android.tradefed.util.RunUtil;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.IOException;
import java.util.Arrays;
import java.util.List;

/** Unit tests for {@link Console}. */
@RunWith(JUnit4.class)
public class ConsoleTest {

    private ICommandScheduler mMockScheduler;
    private Console mConsole;
    private ProxyExceptionHandler mProxyExceptionHandler;
    private boolean mIsConsoleFunctional;

    private static class ProxyExceptionHandler implements Thread.UncaughtExceptionHandler {
        private Throwable mThrowable = null;

        @Override
        public void uncaughtException(Thread t, Throwable e) {
            mThrowable = e;
        }

        public void verify() throws Throwable {
            if (mThrowable != null) {
                throw mThrowable;
            }
        }
    }

    @Before
    public void setUp() throws Exception {
        mMockScheduler = EasyMock.createStrictMock(ICommandScheduler.class);
        mIsConsoleFunctional = false;
        /**
         * Note: Eclipse doesn't play nice with consoles allocated like {@code new ConsoleReader()}.
         * To make an actual ConsoleReader instance, you should likely use the four-arg
         * {@link jline.ConsoleReader} constructor and use {@link jline.UnsupportedTerminal} or
         * similar as the implementation.
         */
        mConsole = new Console(null) {
            @Override
            boolean isConsoleFunctional() {
                return mIsConsoleFunctional;
            }
        };
        mConsole.setCommandScheduler(mMockScheduler);
        mProxyExceptionHandler = new ProxyExceptionHandler();
        mConsole.setUncaughtExceptionHandler(mProxyExceptionHandler);
    }

    @After
    public void tearDown() {
        if (mConsole != null) {
            mConsole.exitConsole();
            mConsole.interrupt();
        }
    }

    /** Test normal Console run when system console is available */
    @Test
    public void testRun_withConsole() throws Throwable {
        mConsole.setName("testRun_withConsole");
        mIsConsoleFunctional = true;

        mMockScheduler.start();
        mMockScheduler.await();
        EasyMock.expectLastCall().anyTimes();
        mMockScheduler.shutdown();  // after we discover that we can't read console input

        EasyMock.replay(mMockScheduler);

        // non interactive mode needs some args to start
        mConsole.setArgs(Arrays.asList("help"));
        mConsole.start();
        mConsole.join();
        mProxyExceptionHandler.verify();
        EasyMock.verify(mMockScheduler);
    }

    /**
     * Test that an interactive console does return and doesn't not stay up when started with
     * 'help'.
     */
    @Test
    public void testRun_withConsoleInteractiveHelp() throws Throwable {
        mConsole = new Console() {
            @Override
            boolean isConsoleFunctional() {
                return mIsConsoleFunctional;
            }
        };
        mConsole.setName("testRun_withConsoleInteractiveHelp");
        mConsole.setCommandScheduler(mMockScheduler);
        mProxyExceptionHandler = new ProxyExceptionHandler();
        mConsole.setUncaughtExceptionHandler(mProxyExceptionHandler);
        mIsConsoleFunctional = true;

        mMockScheduler.start();
        mMockScheduler.await();
        EasyMock.expectLastCall().anyTimes();
        mMockScheduler.shutdown();  // after we discover that it was started with help
        EasyMock.replay(mMockScheduler);

        mConsole.setArgs(Arrays.asList("help"));
        mConsole.start();
        // join has a timeout otherwise it may hang forever.
        mConsole.join(2000);
        assertFalse(mConsole.isAlive());
        mProxyExceptionHandler.verify();
        EasyMock.verify(mMockScheduler);
    }

    /**
     * Test that an interactive console stays up when started without 'help' and scheduler does not
     * shutdown.
     */
    @Test
    public void testRun_withConsoleInteractive_noHelp() throws Throwable {
        mConsole =
                new Console() {
                    @Override
                    boolean isConsoleFunctional() {
                        return mIsConsoleFunctional;
                    }

                    @Override
                    String getConsoleInput() throws IOException {
                        return "test";
                    }
                };
        mConsole.setName("testRun_withConsoleInteractive_noHelp");
        mConsole.setCommandScheduler(mMockScheduler);
        mProxyExceptionHandler = new ProxyExceptionHandler();
        mConsole.setUncaughtExceptionHandler(mProxyExceptionHandler);
        mIsConsoleFunctional = true;

        mMockScheduler.start();
        mMockScheduler.await();
        EasyMock.expectLastCall().anyTimes();
        // No scheduler shutdown is expected.
        EasyMock.replay(mMockScheduler);
        try {
            mConsole.start();
            // join has a timeout otherwise it hangs forever.
            mConsole.join(100);
            assertTrue(mConsole.isAlive());
            mProxyExceptionHandler.verify();
            EasyMock.verify(mMockScheduler);
        } finally {
            mConsole.exitConsole();
            RunUtil.getDefault().interrupt(mConsole, "interrupting");
            mConsole.interrupt();
            mConsole.join(2000);
        }
        assertFalse("Console thread has not stopped.", mConsole.isAlive());
    }

    /** Test normal Console run when system console is _not_ available */
    @Test
    public void testRun_noConsole() throws Throwable {
        mConsole.setName("testRun_noConsole");
        mIsConsoleFunctional = false;

        mMockScheduler.start();
        mMockScheduler.await();
        EasyMock.expectLastCall().anyTimes();
        // after we run the initial command and then immediately quit.
        mMockScheduler.shutdown();

        EasyMock.replay(mMockScheduler);

        // non interactive mode needs some args to start
        mConsole.setArgs(Arrays.asList("help"));
        mConsole.start();
        mConsole.join();
        mProxyExceptionHandler.verify();
        EasyMock.verify(mMockScheduler);
    }

    /** Make sure that "run command foo config.xml" works properly. */
    @Test
    public void testRunCommand() throws Exception {
        mConsole.setName("testRunCommand");
        String[] command = new String[] {"run", "command", "--arg", "value", "config.xml"};
        String[] expected = new String[] {"--arg", "value", "config.xml"};
        CaptureList captures = new CaptureList();
        RegexTrie<Runnable> trie = mConsole.getCommandTrie();

        EasyMock.expect(mMockScheduler.addCommand(EasyMock.aryEq(expected))).andReturn(
                Boolean.TRUE);
        EasyMock.replay(mMockScheduler);

        Runnable runnable = trie.retrieve(captures, command);
        assertNotNull(String.format("Console didn't match input %s", Arrays.toString(command)),
                runnable);
        mConsole.executeCmdRunnable(runnable, captures);
        EasyMock.verify(mMockScheduler);
    }

    /** Make sure that the "run foo config.xml" shortcut works properly. */
    @Test
    public void testRunCommand_shortcut() throws Exception {
        mConsole.setName("testRunCommand_shortcut");
        String[] command = new String[] {"run", "--arg", "value", "config.xml"};
        String[] expected = new String[] {"--arg", "value", "config.xml"};
        CaptureList captures = new CaptureList();
        RegexTrie<Runnable> trie = mConsole.getCommandTrie();

        EasyMock.expect(mMockScheduler.addCommand(EasyMock.aryEq(expected))).andReturn(
                Boolean.TRUE);
        EasyMock.replay(mMockScheduler);

        Runnable runnable = trie.retrieve(captures, command);
        assertNotNull(String.format("Console didn't match input %s", Arrays.toString(command)),
                runnable);
        mConsole.executeCmdRunnable(runnable, captures);
        EasyMock.verify(mMockScheduler);
    }

    /**
     * Make sure that the command "run command command foo config.xml" properly considers the second
     * "command" to be the first token of the command to be executed.
     */
    @Test
    public void testRunCommand_startsWithCommand() throws Exception {
        mConsole.setName("testRunCommand_startsWithCommand");
        String[] command = new String[] {"run", "command", "command", "--arg", "value",
                "config.xml"};
        String[] expected = new String[] {"command", "--arg", "value", "config.xml"};
        CaptureList captures = new CaptureList();
        RegexTrie<Runnable> trie = mConsole.getCommandTrie();

        EasyMock.expect(mMockScheduler.addCommand(EasyMock.aryEq(expected))).andReturn(
                Boolean.TRUE);
        EasyMock.replay(mMockScheduler);

        Runnable runnable = trie.retrieve(captures, command);
        assertNotNull(String.format("Console didn't match input %s", Arrays.toString(command)),
                runnable);
        mConsole.executeCmdRunnable(runnable, captures);
        EasyMock.verify(mMockScheduler);
    }

    /** Make sure that {@link Console#getFlatArgs} works as expected. */
    @Test
    public void testFlatten() throws Exception {
        CaptureList cl = new CaptureList();
        cl.add(Arrays.asList("run", null));
        cl.add(Arrays.asList("alpha"));
        cl.add(Arrays.asList("beta"));
        List<String> flat = Console.getFlatArgs(1, cl);
        assertEquals(2, flat.size());
        assertEquals("alpha", flat.get(0));
        assertEquals("beta", flat.get(1));
    }

    /** Make sure that {@link Console#getFlatArgs} throws an exception when argIdx is wrong. */
    @Test
    public void testFlatten_wrongArgIdx() throws Exception {
        CaptureList cl = new CaptureList();
        cl.add(Arrays.asList("run", null));
        cl.add(Arrays.asList("alpha"));
        cl.add(Arrays.asList("beta"));
        // argIdx is 0, and element 0 has size 2
        try {
            Console.getFlatArgs(0, cl);
            fail("IllegalArgumentException not thrown!");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /** Make sure that {@link Console#getFlatArgs} throws an exception when argIdx is OOB. */
    @Test
    public void testFlatten_argIdxOOB() throws Exception {
        CaptureList cl = new CaptureList();
        cl.add(Arrays.asList("run", null));
        cl.add(Arrays.asList("alpha"));
        cl.add(Arrays.asList("beta"));
        try {
            Console.getFlatArgs(1 + cl.size(), cl);
            fail("IndexOutOfBoundsException not thrown!");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }
    }
}

