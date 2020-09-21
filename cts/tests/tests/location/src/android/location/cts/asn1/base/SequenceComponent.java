/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.location.cts.asn1.base;

import java.util.Collection;

import javax.annotation.Nullable;

/**
 * A component of an {@code Asn1Sequence}.
 *
 */
public interface SequenceComponent {

  boolean isExplicitlySet();

  boolean hasDefaultValue();

  boolean isOptional();

  Asn1Object getComponentValue();

  void setToNewInstance();

  /**
   * Returns tags that may be the initial tag in the BER encoding of this type.
   */
  Collection<Asn1Tag> getPossibleFirstTags();

  @Nullable Asn1Tag getTag();

  boolean isImplicitTagging();

  String toIndentedString(String indent);
}
