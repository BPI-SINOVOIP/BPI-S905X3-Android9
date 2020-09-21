/*
 * Copyright (C) 2014 Google, Inc.
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
package dagger.producers.internal;

import com.google.common.util.concurrent.Futures;
import com.google.common.util.concurrent.ListenableFuture;
import com.google.common.util.concurrent.SettableFuture;
import dagger.producers.Producer;
import dagger.producers.monitoring.ProducerMonitor;
import dagger.producers.monitoring.ProducerToken;
import dagger.producers.monitoring.ProductionComponentMonitor;
import java.util.concurrent.ExecutionException;
import javax.inject.Provider;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

/**
 * Tests {@link AbstractProducer}.
 */
@RunWith(JUnit4.class)
public class AbstractProducerTest {
  @Mock private ProductionComponentMonitor componentMonitor;
  private ProducerMonitor monitor;
  private Provider<ProductionComponentMonitor> componentMonitorProvider;

  @Before
  public void initMocks() {
    MockitoAnnotations.initMocks(this);
    monitor = Mockito.mock(ProducerMonitor.class, Mockito.CALLS_REAL_METHODS);
    when(componentMonitor.producerMonitorFor(any(ProducerToken.class))).thenReturn(monitor);
    componentMonitorProvider =
        new Provider<ProductionComponentMonitor>() {
          @Override
          public ProductionComponentMonitor get() {
            return componentMonitor;
          }
        };
  }

  @Test
  public void get_nullPointerException() {
    Producer<Object> producer = new DelegateProducer<>(componentMonitorProvider, null);
    try {
      producer.get();
      fail();
    } catch (NullPointerException expected) {
    }
  }

  @Test public void get() throws Exception {
    Producer<Integer> producer =
        new AbstractProducer<Integer>(componentMonitorProvider, null) {
          int i = 0;

          @Override
          public ListenableFuture<Integer> compute(ProducerMonitor unusedMonitor) {
            return Futures.immediateFuture(i++);
          }
        };
    assertThat(producer.get().get()).isEqualTo(0);
    assertThat(producer.get().get()).isEqualTo(0);
    assertThat(producer.get().get()).isEqualTo(0);
  }

  @Test
  public void monitor_success() throws Exception {
    SettableFuture<Integer> delegateFuture = SettableFuture.create();
    Producer<Integer> producer = new DelegateProducer<>(componentMonitorProvider, delegateFuture);

    ListenableFuture<Integer> future = producer.get();
    assertThat(future.isDone()).isFalse();
    verify(monitor).addCallbackTo(any(ListenableFuture.class));
    delegateFuture.set(-42);
    assertThat(future.get()).isEqualTo(-42);
    verify(monitor).succeeded(-42);
    verifyNoMoreInteractions(monitor);
  }

  @Test
  public void monitor_failure() throws Exception {
    SettableFuture<Integer> delegateFuture = SettableFuture.create();
    Producer<Integer> producer = new DelegateProducer<>(componentMonitorProvider, delegateFuture);

    ListenableFuture<Integer> future = producer.get();
    assertThat(future.isDone()).isFalse();
    verify(monitor).addCallbackTo(any(ListenableFuture.class));
    Throwable t = new RuntimeException("monkey");
    delegateFuture.setException(t);
    try {
      future.get();
      fail();
    } catch (ExecutionException e) {
      assertThat(e.getCause()).isSameAs(t);
    }
    verify(monitor).failed(t);
    verifyNoMoreInteractions(monitor);
  }

  @Test(expected = NullPointerException.class)
  public void monitor_null() throws Exception {
    new DelegateProducer<>(null, Futures.immediateFuture(42));
  }

  static final class DelegateProducer<T> extends AbstractProducer<T> {
    private final ListenableFuture<T> delegate;

    DelegateProducer(
        Provider<ProductionComponentMonitor> componentMonitorProvider,
        ListenableFuture<T> delegate) {
      super(componentMonitorProvider, null);
      this.delegate = delegate;
    }

    @Override
    public ListenableFuture<T> compute(ProducerMonitor unusedMonitor) {
      return delegate;
    }
  }
}
