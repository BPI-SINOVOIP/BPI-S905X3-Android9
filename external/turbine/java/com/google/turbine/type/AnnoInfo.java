/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.turbine.type;

import static java.util.Objects.requireNonNull;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.turbine.binder.sym.ClassSymbol;
import com.google.turbine.diag.SourceFile;
import com.google.turbine.model.Const;
import com.google.turbine.tree.Tree;
import com.google.turbine.tree.Tree.Anno;
import com.google.turbine.tree.Tree.Expression;

/** An annotation use. */
public class AnnoInfo {
  private final SourceFile source;
  private final ClassSymbol sym;
  private final Tree.Anno tree;
  private final ImmutableMap<String, Const> values;

  public AnnoInfo(
      SourceFile source, ClassSymbol sym, Anno tree, ImmutableMap<String, Const> values) {
    this.source = source;
    this.sym = requireNonNull(sym);
    this.tree = tree;
    this.values = values;
  }

  /** The annotation's source, for diagnostics. */
  public SourceFile source() {
    return source;
  }

  /** The annotation's diagnostic position. */
  public int position() {
    return tree.position();
  }

  /** Arguments, either assignments or a single expression. */
  public ImmutableList<Expression> args() {
    return tree.args();
  }

  /** Bound element-value pairs. */
  public ImmutableMap<String, Const> values() {
    return values;
  }

  /** The annotation's declaration. */
  public ClassSymbol sym() {
    return sym;
  }

  public AnnoInfo withValues(ImmutableMap<String, Const> values) {
    return new AnnoInfo(source, sym, tree, values);
  }
}
