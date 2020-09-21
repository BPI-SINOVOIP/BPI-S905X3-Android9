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

package com.google.clearsilver.jsilver.functions.escape;

import com.google.clearsilver.jsilver.functions.TextFilter;

import java.io.IOException;

/**
 * Returns the input string without any modification.
 * 
 * This function is intended to be registered as an {@code EscapingFunction} so that it can be used
 * to disable autoescaping of expressions.
 */
public class NullEscapeFunction implements TextFilter {

  public void filter(String in, Appendable out) throws IOException {
    out.append(in);
  }
}
