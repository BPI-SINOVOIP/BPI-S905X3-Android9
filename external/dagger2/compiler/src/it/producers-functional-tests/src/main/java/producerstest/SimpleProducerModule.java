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

import com.google.common.collect.ImmutableSet;
import com.google.common.util.concurrent.Futures;
import com.google.common.util.concurrent.ListenableFuture;
import com.google.common.util.concurrent.SettableFuture;
import dagger.producers.Produced;
import dagger.producers.Producer;
import dagger.producers.ProducerModule;
import dagger.producers.Produces;
import java.io.IOException;
import java.util.Set;
import javax.inject.Provider;
import javax.inject.Qualifier;

import static dagger.producers.Produces.Type.SET;
import static dagger.producers.Produces.Type.SET_VALUES;

/**
 * A module that contains various signatures of produces methods. This is not used in any
 * components.
 */
@ProducerModule
final class SimpleProducerModule {
  @Qualifier @interface Qual {
    int value();
  }

  // Unique bindings.

  @Produces
  @Qual(-2)
  static ListenableFuture<String> throwingProducer() {
    throw new RuntimeException("monkey");
  }

  @Produces
  @Qual(-1)
  static ListenableFuture<String> settableFutureStr(SettableFuture<String> future) {
    return future;
  }

  @Produces
  @Qual(0)
  static String str() {
    return "str";
  }

  @Produces
  @Qual(1)
  static ListenableFuture<String> futureStr() {
    return Futures.immediateFuture("future str");
  }

  @Produces
  @Qual(2)
  static String strWithArg(int i) {
    return "str with arg";
  }

  @Produces
  @Qual(3)
  static ListenableFuture<String> futureStrWithArg(int i) {
    return Futures.immediateFuture("future str with arg");
  }

  @Produces
  @Qual(4)
  static String strThrowingException() throws IOException {
    return "str throwing exception";
  }

  @Produces
  @Qual(5)
  static ListenableFuture<String> futureStrThrowingException() throws IOException {
    return Futures.immediateFuture("future str throwing exception");
  }

  @Produces
  @Qual(6)
  static String strWithArgThrowingException(int i) throws IOException {
    return "str with arg throwing exception";
  }

  @Produces
  @Qual(7)
  static ListenableFuture<String> futureStrWithArgThrowingException(int i) throws IOException {
    return Futures.immediateFuture("future str with arg throwing exception");
  }

  @Produces
  @Qual(8)
  static String strWithArgs(int i, Produced<Double> b, Producer<Object> c, Provider<Boolean> d) {
    return "str with args";
  }

  @Produces
  @Qual(9)
  static String strWithArgsThrowingException(
      int i, Produced<Double> b, Producer<Object> c, Provider<Boolean> d) throws IOException {
    return "str with args throwing exception";
  }

  @Produces
  @Qual(10)
  static ListenableFuture<String> futureStrWithArgs(
      int i, Produced<Double> b, Producer<Object> c, Provider<Boolean> d) {
    return Futures.immediateFuture("future str with args");
  }

  @Produces
  @Qual(11)
  static ListenableFuture<String> futureStrWithArgsThrowingException(
      int i, Produced<Double> b, Producer<Object> c, Provider<Boolean> d) throws IOException {
    return Futures.immediateFuture("str with args throwing exception");
  }

  // Set bindings.

  @Produces(type = SET)
  static String setOfStrElement() {
    return "set of str element";
  }

  @Produces(type = SET)
  static String setOfStrElementThrowingException() throws IOException {
    return "set of str element throwing exception";
  }

  @Produces(type = SET)
  static ListenableFuture<String> setOfStrFutureElement() {
    return Futures.immediateFuture("set of str element");
  }

  @Produces(type = SET)
  static ListenableFuture<String> setOfStrFutureElementThrowingException() throws IOException {
    return Futures.immediateFuture("set of str element throwing exception");
  }

  @Produces(type = SET)
  static String setOfStrElementWithArg(int i) {
    return "set of str element with arg";
  }

  @Produces(type = SET)
  static String setOfStrElementWithArgThrowingException(int i) throws IOException {
    return "set of str element with arg throwing exception";
  }

  @Produces(type = SET)
  static ListenableFuture<String> setOfStrFutureElementWithArg(int i) {
    return Futures.immediateFuture("set of str element with arg");
  }

  @Produces(type = SET)
  static ListenableFuture<String> setOfStrFutureElementWithArgThrowingException(int i)
      throws IOException {
    return Futures.immediateFuture("set of str element with arg throwing exception");
  }

  @Produces(type = SET_VALUES)
  static Set<String> setOfStrValues() {
    return ImmutableSet.of("set of str 1", "set of str 2");
  }

  @Produces(type = SET_VALUES)
  static Set<String> setOfStrValuesThrowingException() throws IOException {
    return ImmutableSet.of("set of str 1", "set of str 2 throwing exception");
  }

  @Produces(type = SET_VALUES)
  static ListenableFuture<Set<String>> setOfStrFutureValues() {
    return Futures.<Set<String>>immediateFuture(ImmutableSet.of("set of str 1", "set of str 2"));
  }

  @Produces(type = SET_VALUES)
  static ListenableFuture<Set<String>> setOfStrFutureValuesThrowingException() throws IOException {
    return Futures.<Set<String>>immediateFuture(
        ImmutableSet.of("set of str 1", "set of str 2 throwing exception"));
  }

  @Produces(type = SET_VALUES)
  static Set<String> setOfStrValuesWithArg(int i) {
    return ImmutableSet.of("set of str with arg 1", "set of str with arg 2");
  }

  @Produces(type = SET_VALUES)
  static Set<String> setOfStrValuesWithArgThrowingException(int i) throws IOException {
    return ImmutableSet.of("set of str with arg 1", "set of str with arg 2 throwing exception");
  }

  @Produces(type = SET_VALUES)
  static ListenableFuture<Set<String>> setOfStrFutureValuesWithArg(int i) {
    return Futures.<Set<String>>immediateFuture(
        ImmutableSet.of("set of str with arg 1", "set of str with arg 2"));
  }

  @Produces(type = SET_VALUES)
  static ListenableFuture<Set<String>> setOfStrFutureValuesWithArgThrowingException(int i)
      throws IOException {
    return Futures.<Set<String>>immediateFuture(
        ImmutableSet.of("set of str with arg 1", "set of str with arg 2 throwing exception"));
  }
}
