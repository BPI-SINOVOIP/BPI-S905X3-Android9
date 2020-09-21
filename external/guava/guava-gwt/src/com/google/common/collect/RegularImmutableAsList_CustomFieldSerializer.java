/*
 * Copyright (C) 2010 The Guava Authors
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

import com.google.common.annotations.GwtCompatible;
import com.google.gwt.user.client.rpc.SerializationException;
import com.google.gwt.user.client.rpc.SerializationStreamReader;
import com.google.gwt.user.client.rpc.SerializationStreamWriter;

/**
 * This class implements the server-side GWT serialization of
 * {@link RegularImmutableAsList}.
 *
 * @author Hayward Chan
 */
@GwtCompatible(emulated = true)
public class RegularImmutableAsList_CustomFieldSerializer {

  public static void deserialize(SerializationStreamReader reader,
      RegularImmutableAsList<?> instance) {
  }

  public static RegularImmutableAsList<Object> instantiate(
      SerializationStreamReader reader) throws SerializationException {
    @SuppressWarnings("unchecked") // serialization is necessarily type unsafe
    ImmutableCollection<Object> delegateCollection = (ImmutableCollection) reader.readObject();
    ImmutableList<?> delegateList = (ImmutableList<?>) reader.readObject();
    return new RegularImmutableAsList<Object>(delegateCollection, delegateList);
  }

  public static void serialize(SerializationStreamWriter writer,
      RegularImmutableAsList<?> instance) throws SerializationException {
    writer.writeObject(instance.delegateCollection());
    writer.writeObject(instance.delegateList());
  }
}
