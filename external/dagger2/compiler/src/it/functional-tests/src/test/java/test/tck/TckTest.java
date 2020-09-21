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
package test.tck;

import junit.framework.Test;
import org.atinject.tck.Tck;
import org.atinject.tck.auto.Car;
import org.atinject.tck.auto.Convertible;

/** 
 * Test suite to execute the JSR-330 TCK in JUnit.
 */
public class TckTest {
  public static Test suite() {
    CarShop carShopComponent = DaggerCarShop.create();
    Car car = carShopComponent.make();
    Convertible.localConvertible.set((Convertible) car);
    return Tck.testsFor(car, false, false);
  }
}
