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
package test.multipackage.a;

import javax.inject.Inject;
import test.multipackage.b.BChild;

public class AGrandchild extends BChild {

  @Inject APackagePrivateObject aGrandchildField;

  private APackagePrivateObject aGrandchildMethod;

  @Inject
  void aGrandchildMethod(APackagePrivateObject aGrandchildMethod) {
    this.aGrandchildMethod = aGrandchildMethod;
  }

  @Override
  @Inject
  protected void aParentMethod(APublicObject aParentMethod) {
    super.aParentMethod(aParentMethod);
  }

  @Override
  protected void aChildMethod(APublicObject aChildMethod) {
    super.aChildMethod(aChildMethod);
  }

  public APackagePrivateObject aGrandchildField() {
    return aGrandchildField;
  }

  public APackagePrivateObject aGrandchildMethod() {
    return aGrandchildMethod;
  }
}
