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
package test;

import dagger.Component;
import test.sub.Exposed;
import test.sub.PublicSubclass;

@Component(modules = {ChildDoubleModule.class, ChildIntegerModule.class})
interface GenericComponent {
  ReferencesGeneric referencesGeneric();
  GenericDoubleReferences<A> doubleGenericA();
  GenericDoubleReferences<B> doubleGenericB();
  ComplexGenerics complexGenerics();
  GenericNoDeps<A> noDepsA();
  GenericNoDeps<B> noDepsB();

  void injectA(GenericChild<A> childA);
  void injectB(GenericChild<B> childB);

  Exposed exposed();
  PublicSubclass publicSubclass();
  
  Iterable<Integer> iterableInt();
  Iterable<Double> iterableDouble();
}
