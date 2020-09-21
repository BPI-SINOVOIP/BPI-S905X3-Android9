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
import com.google.gwt.user.client.rpc.core.java.util.Map_CustomFieldSerializerBase;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * This class implements the GWT serialization of {@link RegularImmutableMap}.
 *
 * @author Hayward Chan
 */
public class RegularImmutableMap_CustomFieldSerializer {

  public static void deserialize(SerializationStreamReader reader,
      RegularImmutableMap<?, ?> instance) {
  }

  public static RegularImmutableMap<Object, Object> instantiate(
      SerializationStreamReader reader) throws SerializationException {
    Map<Object, Object> entries = new LinkedHashMap<Object, Object>();
    Map_CustomFieldSerializerBase.deserialize(reader, entries);
    /*
     * For this custom field serializer to be invoked, the map must have been
     * RegularImmutableMap before it's serialized.  Since RegularImmutableMap
     * always have two or more elements, ImmutableMap.copyOf always return
     * a RegularImmutableMap back.
     */
    return (RegularImmutableMap<Object, Object>) ImmutableMap.copyOf(entries);
  }

  public static void serialize(SerializationStreamWriter writer,
      RegularImmutableMap<?, ?> instance) throws SerializationException {
    Map_CustomFieldSerializerBase.serialize(writer, instance);
  }
}
