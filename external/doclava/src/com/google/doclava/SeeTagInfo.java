/*
 * Copyright (C) 2010 Google Inc.
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

package com.google.doclava;

import com.google.clearsilver.jsilver.data.Data;

public class SeeTagInfo extends TagInfo {
  public static final SeeTagInfo[] EMPTY_ARRAY = new SeeTagInfo[0];

  public static SeeTagInfo[] getArray(int size) {
      return size == 0 ? EMPTY_ARRAY : new SeeTagInfo[size];
  }

  private ContainerInfo mBase;
  LinkReference mLink;

  SeeTagInfo(String name, String kind, String text, ContainerInfo base, SourcePositionInfo position) {
    super(name, kind, text, position);
    mBase = base;
  }

  protected LinkReference linkReference() {
    if (mLink == null) {
      mLink =
          LinkReference.parse(text(), mBase, position(), (!"@see".equals(name()))
              && (mBase != null ? mBase.checkLevel() : true));
    }
    return mLink;
  }

  public String label() {
    return linkReference().label;
  }

  @Override
  public void makeHDF(Data data, String base) {
    LinkReference linkRef = linkReference();
    if (linkRef.kind != null) {
      // if they have a better suggestion about "kind" use that.
      // do this before super.makeHDF() so it picks it up
      setKind(linkRef.kind);
    }

    super.makeHDF(data, base);

    data.setValue(base + ".label", linkRef.label);
    if (linkRef.href != null) {
      data.setValue(base + ".href", linkRef.href);
      data.setValue(base + ".federatedSite", linkRef.federatedSite);
    }

    if (ClearPage.toroot != null) {
      data.setValue("toroot", ClearPage.toroot);
    }

    if (linkRef.federatedSite != null) {
      data.setValue("federated", linkRef.federatedSite);
    }
  }

  public boolean checkLevel() {
    return linkReference().checkLevel();
  }

  public static void makeHDF(Data data, String base, SeeTagInfo[] tags) {
    int j = 0;
    for (SeeTagInfo tag : tags) {
      if (tag.mBase.checkLevel() && tag.checkLevel()) {
        tag.makeHDF(data, base + "." + j);
        j++;
      }
    }
  }
}
