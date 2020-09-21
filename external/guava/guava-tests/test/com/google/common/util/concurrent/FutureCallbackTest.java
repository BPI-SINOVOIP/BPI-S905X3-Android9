/*
 * Copyright (C) 2011 The Guava Authors
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

import com.google.common.base.Preconditions;

import junit.framework.TestCase;

import org.mockito.Mockito;

import java.util.concurrent.CancellationException;
import java.util.concurrent.Executor;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

/**
 * Test for {@link FutureCallback}.
 *
 * @author Anthony Zana
 */
public class FutureCallbackTest extends TestCase {
  public void testSameThreadSuccess() {
    SettableFuture<String> f = SettableFuture.create();
    MockCallback callback = new MockCallback("foo");
    Futures.addCallback(f, callback);
    f.set("foo");
  }

  public void testExecutorSuccess() {
    CountingSameThreadExecutor ex = new CountingSameThreadExecutor();
    SettableFuture<String> f = SettableFuture.create();
    MockCallback callback = new MockCallback("foo");
    Futures.addCallback(f, callback, ex);
    f.set("foo");
    assertEquals(1, ex.runCount);
  }

  // Error cases
  public void testSameThreadExecutionException() {
    SettableFuture<String> f = SettableFuture.create();
    Exception e = new IllegalArgumentException("foo not found");
    MockCallback callback = new MockCallback(e);
    Futures.addCallback(f, callback);
    f.setException(e);
  }

  public void testCancel() {
    SettableFuture<String> f = SettableFuture.create();
    FutureCallback<String> callback =
        new FutureCallback<String>() {
          private boolean called = false;
          @Override
          public void onSuccess(String result) {
            fail("Was not expecting onSuccess() to be called.");
          }

          @Override
          public synchronized void onFailure(Throwable t) {
            assertFalse(called);
            assertTrue(t instanceof CancellationException);
            called = true;
          }
        };
    Futures.addCallback(f, callback);
    f.cancel(true);
  }

  public void testThrowErrorFromGet() {
    Error error = new AssertionError("ASSERT!");
    ListenableFuture<String> f = ThrowingFuture.throwingError(error);
    MockCallback callback = new MockCallback(error);
    Futures.addCallback(f, callback);
  }

  public void testRuntimeExeceptionFromGet() {
    RuntimeException e = new IllegalArgumentException("foo not found");
    ListenableFuture<String> f = ThrowingFuture.throwingRuntimeException(e);
    MockCallback callback = new MockCallback(e);
    Futures.addCallback(f, callback);
  }

  public void testOnSuccessThrowsRuntimeException() throws Exception {
    RuntimeException exception = new RuntimeException();
    String result = "result";
    SettableFuture<String> future = SettableFuture.create();
    @SuppressWarnings("unchecked") // Safe for a mock
    FutureCallback<String> callback = Mockito.mock(FutureCallback.class);
    Futures.addCallback(future, callback);
    Mockito.doThrow(exception).when(callback).onSuccess(result);
    future.set(result);
    assertEquals(result, future.get());
    Mockito.verify(callback).onSuccess(result);
    Mockito.verifyNoMoreInteractions(callback);
  }

  public void testOnSuccessThrowsError() throws Exception {
    class TestError extends Error {}
    TestError error = new TestError();
    String result = "result";
    SettableFuture<String> future = SettableFuture.create();
    @SuppressWarnings("unchecked") // Safe for a mock
    FutureCallback<String> callback = Mockito.mock(FutureCallback.class);
    Futures.addCallback(future, callback);
    Mockito.doThrow(error).when(callback).onSuccess(result);
    try {
      future.set(result);
      fail("Should have thrown");
    } catch (TestError e) {
      assertSame(error, e);
    }
    assertEquals(result, future.get());
    Mockito.verify(callback).onSuccess(result);
    Mockito.verifyNoMoreInteractions(callback);
  }

  public void testWildcardFuture() {
    SettableFuture<String> settable = SettableFuture.create();
    ListenableFuture<?> f = settable;
    FutureCallback<Object> callback = new FutureCallback<Object>() {
      @Override
      public void onSuccess(Object result) {}

      @Override
      public void onFailure(Throwable t) {}
    };
    Futures.addCallback(f, callback);
  }

  private class CountingSameThreadExecutor implements Executor {
    int runCount = 0;
    @Override
    public void execute(Runnable command) {
      command.run();
      runCount++;
    }
  }

  // TODO(user): Move to testing, unify with RuntimeExceptionThrowingFuture

  /**
   * A {@link Future} implementation which always throws directly from calls to
   * get() (i.e. not wrapped in ExecutionException.
   * For just a normal Future failure, use {@link SettableFuture}).
   *
   * <p>Useful for testing the behavior of Future utilities against odd futures.
   *
   * @author Anthony Zana
   */
  private static class ThrowingFuture<V> implements ListenableFuture<V> {
    private final Error error;
    private final RuntimeException runtime;

    public static <V> ListenableFuture<V> throwingError(Error error) {
      return new ThrowingFuture<V>(error);
    }

    public static <V> ListenableFuture<V>
        throwingRuntimeException(RuntimeException e) {
      return new ThrowingFuture<V>(e);
    }

    private ThrowingFuture(Error error) {
      this.error = Preconditions.checkNotNull(error);
      this.runtime = null;
    }

    public ThrowingFuture(RuntimeException e) {
      this.runtime = Preconditions.checkNotNull(e);
      this.error = null;
    }

    @Override
    public boolean cancel(boolean mayInterruptIfRunning) {
      return false;
    }

    @Override
    public boolean isCancelled() {
      return false;
    }

    @Override
    public boolean isDone() {
      return true;
    }

    @Override
    public V get() {
      throwOnGet();
      throw new AssertionError("Unreachable");
    }

    @Override
    public V get(long timeout, TimeUnit unit) {
      throwOnGet();
      throw new AssertionError("Unreachable");
    }

    @Override
    public void addListener(Runnable listener, Executor executor) {
      executor.execute(listener);
    }

    private void throwOnGet() {
      if (error != null) {
        throw error;
      } else {
        throw runtime;
      }
    }
  }

  private final class MockCallback implements FutureCallback<String> {
    @Nullable private String value = null;
    @Nullable private Throwable failure = null;
    private boolean wasCalled = false;

    MockCallback(String expectedValue) {
      this.value = expectedValue;
    }

    public MockCallback(Throwable expectedFailure) {
      this.failure = expectedFailure;
    }

    @Override
    public synchronized void onSuccess(String result) {
      assertFalse(wasCalled);
      wasCalled = true;
      assertEquals(value, result);
    }

    @Override
    public synchronized void onFailure(Throwable t) {
      assertFalse(wasCalled);
      wasCalled = true;
      assertEquals(failure, t);
    }
  }
}
