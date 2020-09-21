package com.android.tradefed.util;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.*;

import com.android.tradefed.device.ITestDevice;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.function.Predicate;
import java.util.Vector;

/**
 * Unit tests for {@link CmdUtil}.
 */
@RunWith(JUnit4.class)
public class CmdUtilTest {
    static final String RUN_CMD = "run cmd";
    static final String TEST_CMD = "test cmd";

    class MockSleeper implements CmdUtil.ISleeper {
        @Override
        public void sleep(int seconds) throws InterruptedException {
            return;
        }
    };

    CmdUtil mCmdUtil = null;

    // Predicates to stop retrying cmd.
    private Predicate<String> mCheckEmpty = (String str) -> {
        return str.isEmpty();
    };

    @Mock private ITestDevice mDevice;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        CmdUtil.ISleeper msleeper = new MockSleeper();
        mCmdUtil = new CmdUtil();
        mCmdUtil.setSleeper(msleeper);
    }

    @Test
    public void testWaitCmdSuccess() throws Exception {
        doReturn("").when(mDevice).executeShellCommand(TEST_CMD);
        assertTrue(mCmdUtil.waitCmdResultWithDelay(mDevice, TEST_CMD, mCheckEmpty));
    }

    @Test
    public void testWaitCmdSuccessWithRetry() throws Exception {
        when(mDevice.executeShellCommand(TEST_CMD)).thenReturn("something").thenReturn("");
        assertTrue(mCmdUtil.waitCmdResultWithDelay(mDevice, TEST_CMD, mCheckEmpty));
    }

    @Test
    public void testWaitCmdSuccessFail() throws Exception {
        doReturn("something").when(mDevice).executeShellCommand(TEST_CMD);
        assertFalse(mCmdUtil.waitCmdResultWithDelay(mDevice, TEST_CMD, mCheckEmpty));
    }

    @Test
    public void testRetrySuccess() throws Exception {
        doReturn("").when(mDevice).executeShellCommand(TEST_CMD);
        assertTrue(mCmdUtil.retry(mDevice, RUN_CMD, TEST_CMD, mCheckEmpty));
        verify(mDevice, times(1)).executeShellCommand(eq(RUN_CMD));
    }

    @Test
    public void testRetrySuccessWithRetry() throws Exception {
        when(mDevice.executeShellCommand(TEST_CMD)).thenReturn("something").thenReturn("");
        assertTrue(mCmdUtil.retry(mDevice, RUN_CMD, TEST_CMD, mCheckEmpty));
        verify(mDevice, times(2)).executeShellCommand(eq(RUN_CMD));
    }

    @Test
    public void testRetryFail() throws Exception {
        doReturn("something").when(mDevice).executeShellCommand(TEST_CMD);
        assertFalse(mCmdUtil.retry(mDevice, RUN_CMD, TEST_CMD, mCheckEmpty));
        verify(mDevice, times(mCmdUtil.MAX_RETRY_COUNT)).executeShellCommand(eq(RUN_CMD));
    }

    @Test
    public void testRetryMultipleCommandsSuccess() throws Exception {
        doReturn("").when(mDevice).executeShellCommand(TEST_CMD);
        Vector<String> cmds = new Vector<String>();
        int command_count = 5;
        for (int i = 0; i < command_count; i++) {
            cmds.add(RUN_CMD);
        }
        assertTrue(mCmdUtil.retry(mDevice, cmds, TEST_CMD, mCheckEmpty));
        verify(mDevice, times(command_count)).executeShellCommand(eq(RUN_CMD));
    }
}
