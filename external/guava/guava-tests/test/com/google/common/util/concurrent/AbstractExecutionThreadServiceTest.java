/*
 * Copyright (C) 2009 The Guava Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.common.util.concurrent;

import com.google.common.testing.TearDown;
import com.google.common.testing.TearDownStack;

import junit.framework.TestCase;

import java.lang.Thread.UncaughtExceptionHandler;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Unit test for {@link AbstractExecutionThreadService}.
 *
 * @author Jesse Wilson
 */
public class AbstractExecutionThreadServiceTest extends TestCase {

  private final TearDownStack tearDownStack = new TearDownStack(true);
  private final CountDownLatch enterRun = new CountDownLatch(1);
  private final CountDownLatch exitRun = new CountDownLatch(1);

  private Thread executionThread;
  private Throwable thrownByExecutionThread;
  private final Executor exceptionCatchingExecutor = new Executor() {
    @Override
    public void execute(Runnable command) {
      executionThread = new Thread(command);
      executionThread.setUncaughtExceptionHandler(new UncaughtExceptionHandler() {
        @Override
        public void uncaughtException(Thread thread, Throwable e) {
          thrownByExecutionThread = e;
        }
      });
      executionThread.start();
    }
  };

  @Override protected final void tearDown() {
    tearDownStack.runTearDown();
  }

  public void testServiceStartStop() throws Exception {
    WaitOnRunService service = new WaitOnRunService();
    assertFalse(service.startUpCalled);

    service.startAsync().awaitRunning();
    assertTrue(service.startUpCalled);
    assertEquals(Service.State.RUNNING, service.state());

    enterRun.await(); // to avoid stopping the service until run() is invoked

    service.stopAsync().awaitTerminated();
    assertTrue(service.shutDownCalled);
    assertEquals(Service.State.TERMINATED, service.state());
    executionThread.join();
    assertNull(thrownByExecutionThread);
  }

  public void testServiceStopIdempotence() throws Exception {
    WaitOnRunService service = new WaitOnRunService();

    service.startAsync().awaitRunning();
    enterRun.await(); // to avoid stopping the service until run() is invoked

    service.stopAsync();
    service.stopAsync();
    service.stopAsync().awaitTerminated();
    assertEquals(Service.State.TERMINATED, service.state());
    service.stopAsync().awaitTerminated();
    assertEquals(Service.State.TERMINATED, service.state());

    executionThread.join();
    assertNull(thrownByExecutionThread);
  }

  public void testServiceExitingOnItsOwn() throws Exception {
    WaitOnRunService service = new WaitOnRunService();
    service.expectedShutdownState = Service.State.RUNNING;

    service.startAsync().awaitRunning();
    assertTrue(service.startUpCalled);
    assertEquals(Service.State.RUNNING, service.state());

    exitRun.countDown(); // the service will exit voluntarily
    executionThread.join();

    assertTrue(service.shutDownCalled);
    assertEquals(Service.State.TERMINATED, service.state());
    assertNull(thrownByExecutionThread);

    service.stopAsync().awaitTerminated(); // no-op
    assertEquals(Service.State.TERMINATED, service.state());
    assertTrue(service.shutDownCalled);
  }

  private class WaitOnRunService extends AbstractExecutionThreadService {
    private boolean startUpCalled = false;
    private boolean runCalled = false;
    private boolean shutDownCalled = false;
    private State expectedShutdownState = State.STOPPING;

    @Override protected void startUp() {
      assertFalse(startUpCalled);
      assertFalse(runCalled);
      assertFalse(shutDownCalled);
      startUpCalled = true;
      assertEquals(State.STARTING, state());
    }

    @Override protected void run() {
      assertTrue(startUpCalled);
      assertFalse(runCalled);
      assertFalse(shutDownCalled);
      runCalled = true;
      assertEquals(State.RUNNING, state());

      enterRun.countDown();
      try {
        exitRun.await();
      } catch (InterruptedException e) {
        throw new RuntimeException(e);
      }
    }

    @Override protected void shutDown() {
      assertTrue(startUpCalled);
      assertTrue(runCalled);
      assertFalse(shutDownCalled);
      shutDownCalled = true;
      assertEquals(expectedShutdownState, state());
    }

    @Override protected void triggerShutdown() {
      exitRun.countDown();
    }

    @Override protected Executor executor() {
      return exceptionCatchingExecutor;
    }
  }

  public void testServiceThrowOnStartUp() throws Exception {
    ThrowOnStartUpService service = new ThrowOnStartUpService();
    assertFalse(service.startUpCalled);

    service.startAsync();
    try {
      service.awaitRunning();
      fail();
    } catch (IllegalStateException expected) {
      assertEquals("kaboom!", expected.getCause().getMessage());
    }
    executionThread.join();

    assertTrue(service.startUpCalled);
    assertEquals(Service.State.FAILED, service.state());
    assertTrue(thrownByExecutionThread.getMessage().equals("kaboom!"));
  }

  private class ThrowOnStartUpService extends AbstractExecutionThreadService {
    private boolean startUpCalled = false;

    @Override protected void startUp() {
      startUpCalled = true;
      throw new UnsupportedOperationException("kaboom!");
    }

    @Override protected void run() {
      throw new AssertionError("run() should not be called");
    }

    @Override protected Executor executor() {
      return exceptionCatchingExecutor;
    }
  }

  public void testServiceThrowOnRun() throws Exception {
    ThrowOnRunService service = new ThrowOnRunService();

    service.startAsync();
    try {
      service.awaitTerminated();
      fail();
    } catch (IllegalStateException expected) {
      executionThread.join();
      assertEquals(thrownByExecutionThread, expected.getCause());
    }
    assertTrue(service.shutDownCalled);
    assertEquals(Service.State.FAILED, service.state());
    assertEquals("kaboom!", thrownByExecutionThread.getMessage());
  }

  public void testServiceThrowOnRunAndThenAgainOnShutDown() throws Exception {
    ThrowOnRunService service = new ThrowOnRunService();
    service.throwOnShutDown = true;

    service.startAsync();
    try {
      service.awaitTerminated();
      fail();
    } catch (IllegalStateException expected) {
      executionThread.join();
      assertEquals(thrownByExecutionThread, expected.getCause());
    }

    assertTrue(service.shutDownCalled);
    assertEquals(Service.State.FAILED, service.state());
    assertEquals("kaboom!", thrownByExecutionThread.getMessage());
  }

  private class ThrowOnRunService extends AbstractExecutionThreadService {
    private boolean shutDownCalled = false;
    private boolean throwOnShutDown = false;

    @Override protected void run() {
      throw new UnsupportedOperationException("kaboom!");
    }

    @Override protected void shutDown() {
      shutDownCalled = true;
      if (throwOnShutDown) {
        throw new UnsupportedOperationException("double kaboom!");
      }
    }

    @Override protected Executor executor() {
      return exceptionCatchingExecutor;
    }
  }

  public void testServiceThrowOnShutDown() throws Exception {
    ThrowOnShutDown service = new ThrowOnShutDown();

    service.startAsync().awaitRunning();
    assertEquals(Service.State.RUNNING, service.state());

    service.stopAsync();
    enterRun.countDown();
    executionThread.join();

    assertEquals(Service.State.FAILED, service.state());
    assertEquals("kaboom!", thrownByExecutionThread.getMessage());
  }

  private class ThrowOnShutDown extends AbstractExecutionThreadService {
    @Override protected void run() {
      try {
        enterRun.await();
      } catch (InterruptedException e) {
        throw new RuntimeException(e);
      }
    }

    @Override protected void shutDown() {
      throw new UnsupportedOperationException("kaboom!");
    }

    @Override protected Executor executor() {
      return exceptionCatchingExecutor;
    }
  }

  public void testServiceTimeoutOnStartUp() throws Exception {
    TimeoutOnStartUp service = new TimeoutOnStartUp();

    try {
      service.startAsync().awaitRunning(1, TimeUnit.MILLISECONDS);
      fail();
    } catch (TimeoutException e) {
      assertTrue(e.getMessage().contains(Service.State.STARTING.toString()));
    }
  }

  private class TimeoutOnStartUp extends AbstractExecutionThreadService {
    @Override protected Executor executor() {
      return new Executor() {
        @Override public void execute(Runnable command) {
        }
      };
    }

    @Override
    protected void run() throws Exception {
    }
  }

  public void testStopWhileStarting_runNotCalled() throws Exception {
    final CountDownLatch started = new CountDownLatch(1);
    FakeService service = new FakeService() {
      @Override protected void startUp() throws Exception {
        super.startUp();
        started.await();
      }
    };
    service.startAsync();
    service.stopAsync();
    started.countDown();
    service.awaitTerminated();
    assertEquals(Service.State.TERMINATED, service.state());
    assertEquals(1, service.startupCalled);
    assertEquals(0, service.runCalled);
    assertEquals(1, service.shutdownCalled);
  }

  public void testStop_noStart() {
    FakeService service = new FakeService();
    service.stopAsync().awaitTerminated();
    assertEquals(Service.State.TERMINATED, service.state());
    assertEquals(0, service.startupCalled);
    assertEquals(0, service.runCalled);
    assertEquals(0, service.shutdownCalled);
  }

  public void testDefaultService() throws InterruptedException {
    WaitOnRunService service = new WaitOnRunService();
    service.startAsync().awaitRunning();
    enterRun.await();
    service.stopAsync().awaitTerminated();
  }

  private class FakeService extends AbstractExecutionThreadService implements TearDown {

    private final ExecutorService executor = Executors.newSingleThreadExecutor();

    FakeService() {
      tearDownStack.addTearDown(this);
    }

    volatile int startupCalled = 0;
    volatile int shutdownCalled = 0;
    volatile int runCalled = 0;

    @Override protected void startUp() throws Exception {
      assertEquals(0, startupCalled);
      assertEquals(0, runCalled);
      assertEquals(0, shutdownCalled);
      startupCalled++;
    }

    @Override protected void run() throws Exception {
      assertEquals(1, startupCalled);
      assertEquals(0, runCalled);
      assertEquals(0, shutdownCalled);
      runCalled++;
    }

    @Override protected void shutDown() throws Exception {
      assertEquals(1, startupCalled);
      assertEquals(0, shutdownCalled);
      assertEquals(Service.State.STOPPING, state());
      shutdownCalled++;
    }

    @Override protected Executor executor() {
      return executor;
    }

    @Override public void tearDown() throws Exception {
      executor.shutdown();
    }
  }
}
