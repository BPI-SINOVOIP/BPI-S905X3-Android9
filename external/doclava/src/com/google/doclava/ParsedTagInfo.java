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
import java.util.ArrayList;

public class ParsedTagInfo extends TagInfo {
  public static final ParsedTagInfo[] EMPTY_ARRAY = new ParsedTagInfo[0];

  public static ParsedTagInfo[] getArray(int size) {
      return size == 0 ? EMPTY_ARRAY : new ParsedTagInfo[size];
  }

  private ContainerInfo mContainer;
  private String mCommentText;
  private Comment mComment;

  ParsedTagInfo(String name, String kind, String text, ContainerInfo base, SourcePositionInfo sp) {
    super(name, kind, text, SourcePositionInfo.findBeginning(sp, text));
    mContainer = base;
    mCommentText = text;
  }

  public TagInfo[] commentTags() {
    if (mComment == null) {
      mComment = new Comment(mCommentText, mContainer, position());
    }
    return mComment.tags();
  }

  protected void setCommentText(String comment) {
    mCommentText = comment;
  }

  @Override
  public void makeHDF(Data data, String base) {
    super.makeHDF(data, base);
    TagInfo.makeHDF(data, base + ".commentTags", commentTags());
  }

  public static <T extends ParsedTagInfo> TagInfo[] joinTags(T[] tags) {
    ArrayList<TagInfo> list = new ArrayList<TagInfo>();
    final int N = tags.length;
    for (int i = 0; i < N; i++) {
      TagInfo[] t = tags[i].commentTags();
      final int M = t.length;
      for (int j = 0; j < M; j++) {
        list.add(t[j]);
      }
    }
    return list.toArray(TagInfo.getArray(list.size()));
  }
}
