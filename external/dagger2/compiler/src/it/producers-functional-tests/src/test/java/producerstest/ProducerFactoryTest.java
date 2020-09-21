/*
* Copyright (C) 2015 Google, Inc.
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
package producerstest;

import com.google.common.util.concurrent.ListenableFuture;
import com.google.common.util.concurrent.MoreExecutors;
import com.google.common.util.concurrent.SettableFuture;
import dagger.producers.Producer;
import dagger.producers.monitoring.ProducerMonitor;
import dagger.producers.monitoring.ProducerToken;
import dagger.producers.monitoring.ProductionComponentMonitor;
import java.util.concurrent.ExecutionException;
import javax.inject.Provider;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.when;

@RunWith(JUnit4.class)
public class ProducerFactoryTest {
  @Mock private ProductionComponentMonitor componentMonitor;
  private ProducerMonitor monitor;
  private Provider<ProductionComponentMonitor> componentMonitorProvider;

  @Before
  public void setUpMocks() {
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
  public void noArgMethod() throws Exception {
    ProducerToken token = ProducerToken.create(SimpleProducerModule_StrFactory.class);
    Producer<String> producer =
        new SimpleProducerModule_StrFactory(
            MoreExecutors.directExecutor(), componentMonitorProvider);
    assertThat(producer.get().get()).isEqualTo("str");
    InOrder order = inOrder(componentMonitor, monitor);
    order.verify(componentMonitor).producerMonitorFor(token);
    order.verify(monitor).methodStarting();
    order.verify(monitor).methodFinished();
    order.verify(monitor).succeeded("str");
    order.verifyNoMoreInteractions();
  }

  @Test public void singleArgMethod() throws Exception {
    SettableFuture<Integer> intFuture = SettableFuture.create();
    Producer<Integer> intProducer = producerOfFuture(intFuture);
    Producer<String> producer =
        new SimpleProducerModule_StrWithArgFactory(
            MoreExecutors.directExecutor(), componentMonitorProvider, intProducer);
    assertThat(producer.get().isDone()).isFalse();
    intFuture.set(42);
    assertThat(producer.get().get()).isEqualTo("str with arg");
  }

  @Test
  public void successMonitor() throws Exception {
    ProducerToken token = ProducerToken.create(SimpleProducerModule_SettableFutureStrFactory.class);

    SettableFuture<String> strFuture = SettableFuture.create();
    SettableFuture<SettableFuture<String>> strFutureFuture = SettableFuture.create();
    Producer<SettableFuture<String>> strFutureProducer = producerOfFuture(strFutureFuture);
    Producer<String> producer =
        new SimpleProducerModule_SettableFutureStrFactory(
            MoreExecutors.directExecutor(), componentMonitorProvider, strFutureProducer);
    assertThat(producer.get().isDone()).isFalse();

    InOrder order = inOrder(componentMonitor, monitor);
    order.verify(componentMonitor).producerMonitorFor(token);

    strFutureFuture.set(strFuture);
    order.verify(monitor).methodStarting();
    order.verify(monitor).methodFinished();
    assertThat(producer.get().isDone()).isFalse();

    strFuture.set("monkey");
    assertThat(producer.get().get()).isEqualTo("monkey");
    order.verify(monitor).succeeded("monkey");

    order.verifyNoMoreInteractions();
  }

  @Test
  public void failureMonitor() throws Exception {
    ProducerToken token = ProducerToken.create(SimpleProducerModule_SettableFutureStrFactory.class);

    SettableFuture<String> strFuture = SettableFuture.create();
    SettableFuture<SettableFuture<String>> strFutureFuture = SettableFuture.create();
    Producer<SettableFuture<String>> strFutureProducer = producerOfFuture(strFutureFuture);
    Producer<String> producer =
        new SimpleProducerModule_SettableFutureStrFactory(
            MoreExecutors.directExecutor(), componentMonitorProvider, strFutureProducer);
    assertThat(producer.get().isDone()).isFalse();

    InOrder order = inOrder(componentMonitor, monitor);
    order.verify(componentMonitor).producerMonitorFor(token);

    strFutureFuture.set(strFuture);
    order.verify(monitor).methodStarting();
    order.verify(monitor).methodFinished();
    assertThat(producer.get().isDone()).isFalse();

    Throwable t = new RuntimeException("monkey");
    strFuture.setException(t);
    try {
      producer.get().get();
      fail();
    } catch (ExecutionException e) {
      assertThat(e.getCause()).isSameAs(t);
      order.verify(monitor).failed(t);
    }

    order.verifyNoMoreInteractions();
  }

  @Test
  public void failureMonitorDueToThrowingProducer() throws Exception {
    ProducerToken token = ProducerToken.create(SimpleProducerModule_ThrowingProducerFactory.class);

    Producer<String> producer =
        new SimpleProducerModule_ThrowingProducerFactory(
            MoreExecutors.directExecutor(), componentMonitorProvider);
    assertThat(producer.get().isDone()).isTrue();

    InOrder order = inOrder(componentMonitor, monitor);
    order.verify(componentMonitor).producerMonitorFor(token);

    order.verify(monitor).methodStarting();
    order.verify(monitor).methodFinished();

    try {
      producer.get().get();
      fail();
    } catch (ExecutionException e) {
      order.verify(monitor).failed(e.getCause());
    }

    order.verifyNoMoreInteractions();
  }

  @Test(expected = NullPointerException.class)
  public void nullComponentMonitorProvider() throws Exception {
    new SimpleProducerModule_StrFactory(MoreExecutors.directExecutor(), null);
  }

  private static <T> Producer<T> producerOfFuture(final ListenableFuture<T> future) {
    return new Producer<T>() {
      @Override public ListenableFuture<T> get() {
        return future;
      }
    };
  }
}
