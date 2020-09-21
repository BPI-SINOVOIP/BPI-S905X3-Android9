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
 * This class implements the GWT serialization of {@link ImmutableEnumSet}.
 *
 * @author Hayward Chan
 */
public class ImmutableEnumSet_CustomFieldSerializer {

  public static void deserialize(SerializationStreamReader reader,
      ImmutableEnumSet<?> instance) {
  }

  public static <E extends Enum<E>> ImmutableEnumSet<?> instantiate(
      SerializationStreamReader reader) throws SerializationException {
    List<E> deserialized = Lists.newArrayList();
    Collection_CustomFieldSerializerBase.deserialize(reader, deserialized);
    /*
     * It is safe to cast to ImmutableEnumSet because in order for it to be
     * serialized as an ImmutableEnumSet, it must be non-empty to start
     * with.
     */
    return (ImmutableEnumSet<?>) Sets.immutableEnumSet(deserialized);
  }

  public static void serialize(SerializationStreamWriter writer,
      ImmutableEnumSet<?> instance) throws SerializationException {
    Collection_CustomFieldSerializerBase.serialize(writer, instance);
  }

}
