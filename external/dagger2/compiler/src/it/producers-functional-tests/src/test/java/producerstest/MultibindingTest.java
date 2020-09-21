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

import com.google.common.collect.Iterables;
import com.google.common.util.concurrent.MoreExecutors;
import dagger.producers.Produced;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ExecutionException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static com.google.common.truth.Truth.assertThat;

@RunWith(JUnit4.class)
public class MultibindingTest {
  @Test
  public void setBinding() throws Exception {
    MultibindingComponent multibindingComponent =
        DaggerMultibindingComponent.builder().executor(MoreExecutors.directExecutor()).build();
    assertThat(multibindingComponent.strs().get())
        .containsExactly("foo", "foo1", "foo2", "bar", "bar1", "bar2");
    assertThat(multibindingComponent.strCount().get()).isEqualTo(6);
  }

  @Test
  public void setBindingOfProduced() throws Exception {
    MultibindingComponent multibindingComponent =
        DaggerMultibindingComponent.builder().executor(MoreExecutors.directExecutor()).build();
    assertThat(multibindingComponent.successfulSet().get())
        .containsExactly(
            Produced.successful("foo"),
            Produced.successful("foo1"),
            Produced.successful("foo2"),
            Produced.successful("bar"),
            Produced.successful("bar1"),
            Produced.successful("bar2"));
  }

  @Test
  public void setBindingOfProducedWithFailures() throws Exception {
    MultibindingComponent multibindingComponent =
        DaggerMultibindingComponent.builder().executor(MoreExecutors.directExecutor()).build();
    Set<Produced<String>> possiblyThrowingSet = multibindingComponent.possiblyThrowingSet().get();
    Set<String> successes = new HashSet<>();
    Set<ExecutionException> failures = new HashSet<>();
    for (Produced<String> str : possiblyThrowingSet) {
      try {
        successes.add(str.get());
      } catch (ExecutionException e) {
        failures.add(e);
      }
    }
    assertThat(successes).containsExactly("singleton", "double", "ton");
    assertThat(failures).hasSize(1);
    assertThat(Iterables.getOnlyElement(failures).getCause()).hasMessage("monkey");
  }
}
