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

package com.google.common.collect;

import static com.google.common.base.Preconditions.checkNotNull;

import java.io.Serializable;
import java.util.AbstractCollection;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;

import javax.annotation.Nullable;

/**
 * GWT emulated version of {@link ImmutableCollection}.
 *
 * @author Jesse Wilson
 */
@SuppressWarnings("serial") // we're overriding default serialization
public abstract class ImmutableCollection<E> extends AbstractCollection<E>
    implements Serializable {

  static final ImmutableCollection<Object> EMPTY_IMMUTABLE_COLLECTION
      = new ForwardingImmutableCollection<Object>(Collections.emptyList());

  ImmutableCollection() {}

  public abstract UnmodifiableIterator<E> iterator();

  public boolean contains(@Nullable Object object) {
    return object != null && super.contains(object);
  }

  public final boolean add(E e) {
    throw new UnsupportedOperationException();
  }

  public final boolean remove(Object object) {
    throw new UnsupportedOperationException();
  }

  public final boolean addAll(Collection<? extends E> newElements) {
    throw new UnsupportedOperationException();
  }

  public final boolean removeAll(Collection<?> oldElements) {
    throw new UnsupportedOperationException();
  }

  public final boolean retainAll(Collection<?> elementsToKeep) {
    throw new UnsupportedOperationException();
  }

  public final void clear() {
    throw new UnsupportedOperationException();
  }

  private transient ImmutableList<E> asList;

  public ImmutableList<E> asList() {
    ImmutableList<E> list = asList;
    return (list == null) ? (asList = createAsList()) : list;
  }

  ImmutableList<E> createAsList() {
    switch (size()) {
      case 0:
        return ImmutableList.of();
      case 1:
        return ImmutableList.of(iterator().next());
      default:
        return new RegularImmutableAsList<E>(this, toArray());
    }
  }
  static <E> ImmutableCollection<E> unsafeDelegate(Collection<E> delegate) {
    return new ForwardingImmutableCollection<E>(delegate);
  }

  boolean isPartialView() {
    return false;
  }

  /**
   * GWT emulated version of {@link ImmutableCollection.Builder}.
   */
  public abstract static class Builder<E> {

    Builder() {}

    public abstract Builder<E> add(E element);

    public Builder<E> add(E... elements) {
      checkNotNull(elements); // for GWT
      for (E element : elements) {
        add(checkNotNull(element));
      }
      return this;
    }

    public Builder<E> addAll(Iterable<? extends E> elements) {
      checkNotNull(elements); // for GWT
      for (E element : elements) {
        add(checkNotNull(element));
      }
      return this;
    }

    public Builder<E> addAll(Iterator<? extends E> elements) {
      checkNotNull(elements); // for GWT
      while (elements.hasNext()) {
        add(checkNotNull(elements.next()));
      }
      return this;
    }

    public abstract ImmutableCollection<E> build();
  }
}
