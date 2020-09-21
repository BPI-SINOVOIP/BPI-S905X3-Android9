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

package com.google.turbine.binder.lookup;

import static com.google.common.collect.Iterables.getLast;

import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;
import com.google.common.collect.ImmutableList;
import com.google.turbine.binder.sym.ClassSymbol;
import com.google.turbine.diag.SourceFile;
import com.google.turbine.tree.Tree.ImportDecl;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;

/** An index for statically imported members, in particular constant variables. */
public class MemberImportIndex {

  /** A cache of resolved static imports, keyed by the simple name of the member. */
  private final Map<String, Supplier<ClassSymbol>> cache = new LinkedHashMap<>();

  private final ImmutableList<Supplier<ClassSymbol>> classes;

  public MemberImportIndex(
      SourceFile source,
      CanonicalSymbolResolver resolve,
      TopLevelIndex tli,
      ImmutableList<ImportDecl> imports) {
    ImmutableList.Builder<Supplier<ClassSymbol>> packageScopes = ImmutableList.builder();
    for (ImportDecl i : imports) {
      if (!i.stat()) {
        continue;
      }
      if (i.wild()) {
        packageScopes.add(
            Suppliers.memoize(
                new Supplier<ClassSymbol>() {
                  @Override
                  public ClassSymbol get() {
                    LookupResult result = tli.lookup(new LookupKey(i.type()));
                    return result != null ? resolve.resolve(source, i.position(), result) : null;
                  }
                }));
      } else {
        cache.put(
            getLast(i.type()),
            Suppliers.memoize(
                new Supplier<ClassSymbol>() {
                  @Override
                  public ClassSymbol get() {
                    LookupResult result1 = tli.lookup(new LookupKey(i.type()));
                    if (result1 == null) {
                      return null;
                    }
                    ClassSymbol sym = (ClassSymbol) result1.sym();
                    for (int i = 0; i < result1.remaining().size() - 1; i++) {
                      sym = resolve.resolveOne(sym, result1.remaining().get(i));
                    }
                    return sym;
                  }
                }));
      }
    }
    this.classes = packageScopes.build();
  }

  /** Resolves the owner of a single-member static import of the given simple name. */
  public ClassSymbol singleMemberImport(String simpleName) {
    Supplier<ClassSymbol> cachedResult = cache.get(simpleName);
    return cachedResult != null ? cachedResult.get() : null;
  }

  /**
   * Returns an iterator over all classes whose members are on-demand imported into the current
   * compilation unit.
   */
  public Iterator<ClassSymbol> onDemandImports() {
    return new WildcardSymbols(classes.iterator());
  }

  private static class WildcardSymbols implements Iterator<ClassSymbol> {
    private final Iterator<Supplier<ClassSymbol>> it;

    public WildcardSymbols(Iterator<Supplier<ClassSymbol>> it) {
      this.it = it;
    }

    @Override
    public boolean hasNext() {
      return it.hasNext();
    }

    @Override
    public ClassSymbol next() {
      return it.next().get();
    }

    @Override
    public void remove() {
      throw new UnsupportedOperationException("remove");
    }
  }
}
