/*
 * Copyright (C) 2009 The Guava Authors
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

import com.google.gwt.user.client.rpc.SerializationException;
import com.google.gwt.user.client.rpc.SerializationStreamReader;
import com.google.gwt.user.client.rpc.SerializationStreamWriter;

import java.util.Comparator;

/**
 * This class implements the GWT serialization of
 * {@link EmptyImmutableSortedSet}.
 * 
 * @author Chris Povirk
 */
public class EmptyImmutableSortedSet_CustomFieldSerializer {
  public static void deserialize(SerializationStreamReader reader,
      EmptyImmutableSortedSet<?> instance) {
  }

  public static EmptyImmutableSortedSet<Object> instantiate(
      SerializationStreamReader reader) throws SerializationException {
    /*
     * Nothing we can do, but we're already assuming the serialized form is
     * correctly typed, anyway.
     */
    @SuppressWarnings("unchecked")
    Comparator<Object> comparator = (Comparator<Object>) reader.readObject();

    /*
     * For this custom field serializer to be invoked, the set must have been
     * EmptyImmutableSortedSet before it's serialized. Since
     * EmptyImmutableSortedSet always has no elements, ImmutableSortedSet.copyOf
     * always return an EmptyImmutableSortedSet back.
     */
    return (EmptyImmutableSortedSet<Object>)
        ImmutableSortedSet.orderedBy(comparator).build();
  }

  public static void serialize(SerializationStreamWriter writer,
      EmptyImmutableSortedSet<?> instance) throws SerializationException {
    writer.writeObject(instance.comparator());
  }
}
