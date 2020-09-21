/*
 * Copyright (C) 2008 The Guava Authors
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

package com.google.common.collect.testing;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.fail;

import com.google.common.annotations.GwtCompatible;

import junit.framework.AssertionFailedError;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.Stack;

/**
 * Most of the logic for {@link IteratorTester} and {@link ListIteratorTester}.
 *
 * @param <E> the type of element returned by the iterator
 * @param <I> the type of the iterator ({@link Iterator} or
 *     {@link ListIterator})
 *
 * @author Kevin Bourrillion
 * @author Chris Povirk
 */
@GwtCompatible
abstract class AbstractIteratorTester<E, I extends Iterator<E>> {
  private boolean whenNextThrowsExceptionStopTestingCallsToRemove;
  private boolean whenAddThrowsExceptionStopTesting;

  /**
   * Don't verify iterator behavior on remove() after a call to next()
   * throws an exception.
   *
   * <p>JDK 6 currently has a bug where some iterators get into a undefined
   * state when next() throws a NoSuchElementException. The correct
   * behavior is for remove() to remove the last element returned by
   * next, even if a subsequent next() call threw an exception; however
   * JDK 6's HashMap and related classes throw an IllegalStateException
   * in this case.
   *
   * <p>Calling this method causes the iterator tester to skip testing
   * any remove() in a stimulus sequence after the reference iterator
   * throws an exception in next().
   *
   * <p>TODO: remove this once we're on 6u5, which has the fix.
   *
   * @see <a href="http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6529795">
   *     Sun Java Bug 6529795</a>
   */
  public void ignoreSunJavaBug6529795() {
    whenNextThrowsExceptionStopTestingCallsToRemove = true;
  }

  /**
   * Don't verify iterator behavior after a call to add() throws an exception.
   *
   * <p>AbstractList's ListIterator implementation gets into a undefined state
   * when add() throws an UnsupportedOperationException. Instead of leaving the
   * iterator's position unmodified, it increments it, skipping an element or
   * even moving past the end of the list.
   *
   * <p>Calling this method causes the iterator tester to skip testing in a
   * stimulus sequence after the iterator under test throws an exception in
   * add().
   *
   * <p>TODO: remove this once the behavior is fixed.
   *
   * @see <a href="http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6533203">
   *     Sun Java Bug 6533203</a>
   */
  public void stopTestingWhenAddThrowsException() {
    whenAddThrowsExceptionStopTesting = true;
  }

  private Stimulus<E, ? super I>[] stimuli;
  private final Iterator<E> elementsToInsert;
  private final Set<IteratorFeature> features;
  private final List<E> expectedElements;
  private final int startIndex;
  private final KnownOrder knownOrder;

  /**
   * Meta-exception thrown by
   * {@link AbstractIteratorTester.MultiExceptionListIterator} instead of
   * throwing any particular exception type.
   */
  // This class is accessible but not supported in GWT.
  private static final class PermittedMetaException extends RuntimeException {
    final Set<? extends Class<? extends RuntimeException>> exceptionClasses;

    PermittedMetaException(
        Set<? extends Class<? extends RuntimeException>> exceptionClasses) {
      super("one of " + exceptionClasses);
      this.exceptionClasses = exceptionClasses;
    }

    PermittedMetaException(Class<? extends RuntimeException> exceptionClass) {
      this(Collections.singleton(exceptionClass));
    }

    // It's not supported In GWT, it always returns true.
    boolean isPermitted(RuntimeException exception) {
      for (Class<? extends RuntimeException> clazz : exceptionClasses) {
        if (Platform.checkIsInstance(clazz, exception)) {
          return true;
        }
      }
      return false;
    }

    // It's not supported in GWT, it always passes.
    void assertPermitted(RuntimeException exception) {
      if (!isPermitted(exception)) {
        // TODO: use simple class names
        String message = "Exception " + exception.getClass()
            + " was thrown; expected " + this;
        Helpers.fail(exception, message);
      }
    }

    @Override public String toString() {
      return getMessage();
    }

    private static final long serialVersionUID = 0;
  }

  private static final class UnknownElementException extends RuntimeException {
    private UnknownElementException(Collection<?> expected, Object actual) {
      super("Returned value '"
          + actual + "' not found. Remaining elements: " + expected);
    }

    private static final long serialVersionUID = 0;
  }

  /**
   * Quasi-implementation of {@link ListIterator} that works from a list of
   * elements and a set of features to support (from the enclosing
   * {@link AbstractIteratorTester} instance). Instead of throwing exceptions
   * like {@link NoSuchElementException} at the appropriate times, it throws
   * {@link PermittedMetaException} instances, which wrap a set of all
   * exceptions that the iterator could throw during the invocation of that
   * method. This is necessary because, e.g., a call to
   * {@code iterator().remove()} of an unmodifiable list could throw either
   * {@link IllegalStateException} or {@link UnsupportedOperationException}.
   * Note that iterator implementations should always throw one of the
   * exceptions in a {@code PermittedExceptions} instance, since
   * {@code PermittedExceptions} is thrown only when a method call is invalid.
   *
   * <p>This class is accessible but not supported in GWT as it references
   * {@link PermittedMetaException}.
   */
  protected final class MultiExceptionListIterator implements ListIterator<E> {
    // TODO: track seen elements when order isn't guaranteed
    // TODO: verify contents afterward
    // TODO: something shiny and new instead of Stack
    // TODO: test whether null is supported (create a Feature)
    /**
     * The elements to be returned by future calls to {@code next()}, with the
     * first at the top of the stack.
     */
    final Stack<E> nextElements = new Stack<E>();
    /**
     * The elements to be returned by future calls to {@code previous()}, with
     * the first at the top of the stack.
     */
    final Stack<E> previousElements = new Stack<E>();
    /**
     * {@link #nextElements} if {@code next()} was called more recently then
     * {@code previous}, {@link #previousElements} if the reverse is true, or --
     * overriding both of these -- {@code null} if {@code remove()} or
     * {@code add()} has been called more recently than either. We use this to
     * determine which stack to pop from on a call to {@code remove()} (or to
     * pop from and push to on a call to {@code set()}.
     */
    Stack<E> stackWithLastReturnedElementAtTop = null;

    MultiExceptionListIterator(List<E> expectedElements) {
      Helpers.addAll(nextElements, Helpers.reverse(expectedElements));
      for (int i = 0; i < startIndex; i++) {
        previousElements.push(nextElements.pop());
      }
    }

    @Override
    public void add(E e) {
      if (!features.contains(IteratorFeature.SUPPORTS_ADD)) {
        throw new PermittedMetaException(UnsupportedOperationException.class);
      }

      previousElements.push(e);
      stackWithLastReturnedElementAtTop = null;
    }

    @Override
    public boolean hasNext() {
      return !nextElements.isEmpty();
    }

    @Override
    public boolean hasPrevious() {
      return !previousElements.isEmpty();
    }

    @Override
    public E next() {
      return transferElement(nextElements, previousElements);
    }

    @Override
    public int nextIndex() {
      return previousElements.size();
    }

    @Override
    public E previous() {
      return transferElement(previousElements, nextElements);
    }

    @Override
    public int previousIndex() {
      return nextIndex() - 1;
    }

    @Override
    public void remove() {
      throwIfInvalid(IteratorFeature.SUPPORTS_REMOVE);

      stackWithLastReturnedElementAtTop.pop();
      stackWithLastReturnedElementAtTop = null;
    }

    @Override
    public void set(E e) {
      throwIfInvalid(IteratorFeature.SUPPORTS_SET);

      stackWithLastReturnedElementAtTop.pop();
      stackWithLastReturnedElementAtTop.push(e);
    }

    /**
     * Moves the given element from its current position in
     * {@link #nextElements} to the top of the stack so that it is returned by
     * the next call to {@link Iterator#next()}. If the element is not in
     * {@link #nextElements}, this method throws an
     * {@link UnknownElementException}.
     *
     * <p>This method is used when testing iterators without a known ordering.
     * We poll the target iterator's next element and pass it to the reference
     * iterator through this method so it can return the same element. This
     * enables the assertion to pass and the reference iterator to properly
     * update its state.
     */
    void promoteToNext(E e) {
      if (nextElements.remove(e)) {
        nextElements.push(e);
      } else {
        throw new UnknownElementException(nextElements, e);
      }
    }

    private E transferElement(Stack<E> source, Stack<E> destination) {
      if (source.isEmpty()) {
        throw new PermittedMetaException(NoSuchElementException.class);
      }

      destination.push(source.pop());
      stackWithLastReturnedElementAtTop = destination;
      return destination.peek();
    }

    private void throwIfInvalid(IteratorFeature methodFeature) {
      Set<Class<? extends RuntimeException>> exceptions
          = new HashSet<Class<? extends RuntimeException>>();

      if (!features.contains(methodFeature)) {
        exceptions.add(UnsupportedOperationException.class);
      }

      if (stackWithLastReturnedElementAtTop == null) {
        exceptions.add(IllegalStateException.class);
      }

      if (!exceptions.isEmpty()) {
        throw new PermittedMetaException(exceptions);
      }
    }

    private List<E> getElements() {
      List<E> elements = new ArrayList<E>();
      Helpers.addAll(elements, previousElements);
      Helpers.addAll(elements, Helpers.reverse(nextElements));
      return elements;
    }
  }

  public enum KnownOrder { KNOWN_ORDER, UNKNOWN_ORDER }

  @SuppressWarnings("unchecked") // creating array of generic class Stimulus
  AbstractIteratorTester(int steps, Iterable<E> elementsToInsertIterable,
      Iterable<? extends IteratorFeature> features,
      Iterable<E> expectedElements, KnownOrder knownOrder, int startIndex) {
    // periodically we should manually try (steps * 3 / 2) here; all tests but
    // one should still pass (testVerifyGetsCalled()).
    stimuli = new Stimulus[steps];
    if (!elementsToInsertIterable.iterator().hasNext()) {
      throw new IllegalArgumentException();
    }
    elementsToInsert = Helpers.cycle(elementsToInsertIterable);
    this.features = Helpers.copyToSet(features);
    this.expectedElements = Helpers.copyToList(expectedElements);
    this.knownOrder = knownOrder;
    this.startIndex = startIndex;
  }

  /**
   * I'd like to make this a parameter to the constructor, but I can't because
   * the stimulus instances refer to {@code this}.
   */
  protected abstract Iterable<? extends Stimulus<E, ? super I>>
      getStimulusValues();

  /**
   * Returns a new target iterator each time it's called. This is the iterator
   * you are trying to test. This must return an Iterator that returns the
   * expected elements passed to the constructor in the given order. Warning: it
   * is not enough to simply pull multiple iterators from the same source
   * Iterable, unless that Iterator is unmodifiable.
   */
  protected abstract I newTargetIterator();

  /**
   * Override this to verify anything after running a list of Stimuli.
   *
   * <p>For example, verify that calls to remove() actually removed
   * the correct elements.
   *
   * @param elements the expected elements passed to the constructor, as mutated
   *     by {@code remove()}, {@code set()}, and {@code add()} calls
   */
  protected void verify(List<E> elements) {}

  /**
   * Executes the test.
   */
  public final void test() {
    try {
      recurse(0);
    } catch (RuntimeException e) {
      throw new RuntimeException(Arrays.toString(stimuli), e);
    }
  }

  private void recurse(int level) {
    // We're going to reuse the stimuli array 3^steps times by overwriting it
    // in a recursive loop.  Sneaky.
    if (level == stimuli.length) {
      // We've filled the array.
      compareResultsForThisListOfStimuli();
    } else {
      // Keep recursing to fill the array.
      for (Stimulus<E, ? super I> stimulus : getStimulusValues()) {
        stimuli[level] = stimulus;
        recurse(level + 1);
      }
    }
  }

  private void compareResultsForThisListOfStimuli() {
    MultiExceptionListIterator reference =
        new MultiExceptionListIterator(expectedElements);
    I target = newTargetIterator();
    boolean shouldStopTestingCallsToRemove = false;
    for (int i = 0; i < stimuli.length; i++) {
      Stimulus<E, ? super I> stimulus = stimuli[i];
      if (stimulus.equals(remove) && shouldStopTestingCallsToRemove) {
        break;
      }
      try {
        boolean threwException = stimulus.executeAndCompare(reference, target);
        if (threwException
            && stimulus.equals(next)
            && whenNextThrowsExceptionStopTestingCallsToRemove) {
          shouldStopTestingCallsToRemove = true;
        }
        if (threwException
            && stimulus.equals(add)
            && whenAddThrowsExceptionStopTesting) {
          break;
        }
        List<E> elements = reference.getElements();
        verify(elements);
      } catch (AssertionFailedError cause) {
        Helpers.fail(cause,
            "failed with stimuli " + subListCopy(stimuli, i + 1));
      }
    }
  }

  private static List<Object> subListCopy(Object[] source, int size) {
    final Object[] copy = new Object[size];
    System.arraycopy(source, 0, copy, 0, size);
    return Arrays.asList(copy);
  }

  private interface IteratorOperation {
    Object execute(Iterator<?> iterator);
  }

  /**
   * Apply this method to both iterators and return normally only if both
   * produce the same response.
   *
   * @return {@code true} if an exception was thrown by the iterators.
   *
   * @see Stimulus#executeAndCompare(ListIterator, Iterator)
   */
  private <T extends Iterator<E>> boolean internalExecuteAndCompare(
      T reference, T target, IteratorOperation method)
      throws AssertionFailedError {

    Object referenceReturnValue = null;
    PermittedMetaException referenceException = null;
    Object targetReturnValue = null;
    RuntimeException targetException = null;

    try {
      targetReturnValue = method.execute(target);
    } catch (RuntimeException e) {
      targetException = e;
    }

    try {
      if (method == NEXT_METHOD && targetException == null
          && knownOrder == KnownOrder.UNKNOWN_ORDER) {
        /*
         * We already know the iterator is an Iterator<E>, and now we know that
         * we called next(), so the returned element must be of type E.
         */
        @SuppressWarnings("unchecked")
        E targetReturnValueFromNext = (E) targetReturnValue;
        /*
         * We have an Iterator<E> and want to cast it to
         * MultiExceptionListIterator. Because we're inside an
         * AbstractIteratorTester<E>, that's implicitly a cast to
         * AbstractIteratorTester<E>.MultiExceptionListIterator. The runtime
         * won't be able to verify the AbstractIteratorTester<E> part, so it's
         * an unchecked cast. We know, however, that the only possible value for
         * the type parameter is <E>, since otherwise the
         * MultiExceptionListIterator wouldn't be an Iterator<E>. The cast is
         * safe, even though javac can't tell.
         *
         * Sun bug 6665356 is an additional complication. Until OpenJDK 7, javac
         * doesn't recognize this kind of cast as unchecked cast. Neither does
         * Eclipse 3.4. Right now, this suppression is mostly unecessary.
         */
        MultiExceptionListIterator multiExceptionListIterator =
            (MultiExceptionListIterator) reference;
        multiExceptionListIterator.promoteToNext(targetReturnValueFromNext);
      }

      referenceReturnValue = method.execute(reference);
    } catch (PermittedMetaException e) {
      referenceException = e;
    } catch (UnknownElementException e) {
      Helpers.fail(e, e.getMessage());
    }

    if (referenceException == null) {
      if (targetException != null) {
        Helpers.fail(targetException,
            "Target threw exception when reference did not");
      }

      /*
       * Reference iterator returned a value, so we should expect the
       * same value from the target
       */
      assertEquals(referenceReturnValue, targetReturnValue);

      return false;
    }

    if (targetException == null) {
      fail("Target failed to throw " + referenceException);
    }

    /*
     * Reference iterator threw an exception, so we should expect an acceptable
     * exception from the target.
     */
    referenceException.assertPermitted(targetException);

    return true;
  }

  private static final IteratorOperation REMOVE_METHOD =
      new IteratorOperation() {
        @Override
        public Object execute(Iterator<?> iterator) {
          iterator.remove();
          return null;
        }
      };

  private static final IteratorOperation NEXT_METHOD =
      new IteratorOperation() {
        @Override
        public Object execute(Iterator<?> iterator) {
          return iterator.next();
        }
      };

  private static final IteratorOperation PREVIOUS_METHOD =
      new IteratorOperation() {
        @Override
        public Object execute(Iterator<?> iterator) {
          return ((ListIterator<?>) iterator).previous();
        }
      };

  private final IteratorOperation newAddMethod() {
    final Object toInsert = elementsToInsert.next();
    return new IteratorOperation() {
      @Override
      public Object execute(Iterator<?> iterator) {
        @SuppressWarnings("unchecked")
        ListIterator<Object> rawIterator = (ListIterator<Object>) iterator;
        rawIterator.add(toInsert);
        return null;
      }
    };
  }

  private final IteratorOperation newSetMethod() {
    final E toInsert = elementsToInsert.next();
    return new IteratorOperation() {
      @Override
      public Object execute(Iterator<?> iterator) {
        @SuppressWarnings("unchecked")
        ListIterator<E> li = (ListIterator<E>) iterator;
        li.set(toInsert);
        return null;
      }
    };
  }

  abstract static class Stimulus<E, T extends Iterator<E>> {
    private final String toString;

    protected Stimulus(String toString) {
      this.toString = toString;
    }

    /**
     * Send this stimulus to both iterators and return normally only if both
     * produce the same response.
     *
     * @return {@code true} if an exception was thrown by the iterators.
     */
    abstract boolean executeAndCompare(ListIterator<E> reference, T target);

    @Override public String toString() {
      return toString;
    }
  }

  Stimulus<E, Iterator<E>> hasNext = new Stimulus<E, Iterator<E>>("hasNext") {
    @Override boolean
        executeAndCompare(ListIterator<E> reference, Iterator<E> target) {
      // return only if both are true or both are false
      assertEquals(reference.hasNext(), target.hasNext());
      return false;
    }
  };
  Stimulus<E, Iterator<E>> next = new Stimulus<E, Iterator<E>>("next") {
    @Override boolean
        executeAndCompare(ListIterator<E> reference, Iterator<E> target) {
      return internalExecuteAndCompare(reference, target, NEXT_METHOD);
    }
  };
  Stimulus<E, Iterator<E>> remove = new Stimulus<E, Iterator<E>>("remove") {
    @Override boolean
        executeAndCompare(ListIterator<E> reference, Iterator<E> target) {
      return internalExecuteAndCompare(reference, target, REMOVE_METHOD);
    }
  };

  @SuppressWarnings("unchecked")
  List<Stimulus<E, Iterator<E>>> iteratorStimuli() {
    return Arrays.asList(hasNext, next, remove);
  }

  Stimulus<E, ListIterator<E>> hasPrevious =
      new Stimulus<E, ListIterator<E>>("hasPrevious") {
        @Override boolean executeAndCompare(
            ListIterator<E> reference, ListIterator<E> target) {
          // return only if both are true or both are false
          assertEquals(reference.hasPrevious(), target.hasPrevious());
          return false;
        }
      };
  Stimulus<E, ListIterator<E>> nextIndex =
      new Stimulus<E, ListIterator<E>>("nextIndex") {
        @Override boolean executeAndCompare(
            ListIterator<E> reference, ListIterator<E> target) {
          assertEquals(reference.nextIndex(), target.nextIndex());
          return false;
        }
      };
  Stimulus<E, ListIterator<E>> previousIndex =
      new Stimulus<E, ListIterator<E>>("previousIndex") {
        @Override boolean executeAndCompare(
            ListIterator<E> reference, ListIterator<E> target) {
          assertEquals(reference.previousIndex(), target.previousIndex());
          return false;
        }
      };
  Stimulus<E, ListIterator<E>> previous =
      new Stimulus<E, ListIterator<E>>("previous") {
        @Override boolean executeAndCompare(
            ListIterator<E> reference, ListIterator<E> target) {
          return internalExecuteAndCompare(reference, target, PREVIOUS_METHOD);
        }
      };
  Stimulus<E, ListIterator<E>> add = new Stimulus<E, ListIterator<E>>("add") {
    @Override boolean executeAndCompare(
        ListIterator<E> reference, ListIterator<E> target) {
      return internalExecuteAndCompare(reference, target, newAddMethod());
    }
  };
  Stimulus<E, ListIterator<E>> set = new Stimulus<E, ListIterator<E>>("set") {
    @Override boolean executeAndCompare(
        ListIterator<E> reference, ListIterator<E> target) {
      return internalExecuteAndCompare(reference, target, newSetMethod());
    }
  };

  @SuppressWarnings("unchecked")
  List<Stimulus<E, ListIterator<E>>> listIteratorStimuli() {
    return Arrays.asList(
        hasPrevious, nextIndex, previousIndex, previous, add, set);
  }
}
