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
import com.google.gwt.user.client.rpc.core.java.util.Collection_CustomFieldSerializerBase;

import java.util.List;

/**
 * This class implements the GWT serialization of {@link RegularImmutableSet}.
 *
 * @author Hayward Chan
 */
public class RegularImmutableSet_CustomFieldSerializer {

  public static void deserialize(SerializationStreamReader reader,
      RegularImmutableSet<?> instance) {
  }

  public static RegularImmutableSet<Object> instantiate(
      SerializationStreamReader reader) throws SerializationException {
    List<Object> elements = Lists.newArrayList();
    Collection_CustomFieldSerializerBase.deserialize(reader, elements);
    /*
     * For this custom field serializer to be invoked, the set must have been
     * RegularImmutableSet before it's serialized.  Since RegularImmutableSet
     * always have two or more elements, ImmutableSet.copyOf always return
     * a RegularImmutableSet back.
     */
    return (RegularImmutableSet<Object>) ImmutableSet.copyOf(elements);
  }

  public static void serialize(SerializationStreamWriter writer,
      RegularImmutableSet<?> instance) throws SerializationException {
    Collection_CustomFieldSerializerBase.serialize(writer, instance);
  }
}
