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
package producerstest.monitoring;

import dagger.Module;
import dagger.Provides;
import dagger.producers.monitoring.ProductionComponentMonitor;

import static dagger.Provides.Type.SET;

@Module
final class MonitoringModule {
  private final ProductionComponentMonitor.Factory monitorFactory;

  MonitoringModule(ProductionComponentMonitor.Factory monitorFactory) {
    this.monitorFactory = monitorFactory;
  }

  @Provides(type = SET)
  ProductionComponentMonitor.Factory monitorFactory() {
    return monitorFactory;
  }
}
