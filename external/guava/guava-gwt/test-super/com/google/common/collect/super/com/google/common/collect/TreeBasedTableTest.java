/*
 * Copyright (C) 2008 The Guava Authors
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

import static com.google.common.truth.Truth.assertThat;

import com.google.common.annotations.GwtCompatible;
import com.google.common.collect.testing.SortedMapInterfaceTest;

import java.util.Collections;
import java.util.Comparator;
import java.util.Map;
import java.util.Set;
import java.util.SortedMap;

/**
 * Test cases for {@link TreeBasedTable}.
 *
 * @author Jared Levy
 * @author Louis Wasserman
 */
@GwtCompatible(emulated = true)
public class TreeBasedTableTest extends AbstractTableTest {

  public static class TreeRowTest extends
      SortedMapInterfaceTest<String, String> {
    public TreeRowTest() {
      super(false, false, true, true, true);
    }

    @Override protected SortedMap<String, String> makeEmptyMap() {
      TreeBasedTable<String, String, String> table = TreeBasedTable.create();
      table.put("a", "b", "c");
      table.put("c", "b", "a");
      table.put("a", "a", "d");
      return table.row("b");
    }

    @Override protected SortedMap<String, String> makePopulatedMap() {
      TreeBasedTable<String, String, String> table = TreeBasedTable.create();
      table.put("a", "b", "c");
      table.put("c", "b", "a");
      table.put("b", "b", "x");
      table.put("b", "c", "y");
      table.put("b", "x", "n");
      table.put("a", "a", "d");
      return table.row("b");
    }

    @Override protected String getKeyNotInPopulatedMap() {
      return "q";
    }

    @Override protected String getValueNotInPopulatedMap() {
      return "p";
    }

    public void testClearSubMapOfRowMap() {
      TreeBasedTable<String, String, String> table = TreeBasedTable.create();
      table.put("a", "b", "c");
      table.put("c", "b", "a");
      table.put("b", "b", "x");
      table.put("b", "c", "y");
      table.put("b", "x", "n");
      table.put("a", "a", "d");
      table.row("b").subMap("c", "x").clear();
      assertEquals(table.row("b"), ImmutableMap.of("b", "x", "x", "n"));
      table.row("b").subMap("b", "y").clear();
      assertEquals(table.row("b"), ImmutableMap.of());
      assertFalse(table.backingMap.containsKey("b"));
    }
  }

  private TreeBasedTable<String, Integer, Character> sortedTable;

  protected TreeBasedTable<String, Integer, Character> create(
    Comparator<? super String> rowComparator,
    Comparator<? super Integer> columnComparator,
    Object... data) {
    TreeBasedTable<String, Integer, Character> table =
        TreeBasedTable.create(rowComparator, columnComparator);
    table.put("foo", 4, 'a');
    table.put("cat", 1, 'b');
    table.clear();
    populate(table, data);
    return table;
  }

  @Override protected TreeBasedTable<String, Integer, Character> create(
      Object... data) {
    TreeBasedTable<String, Integer, Character> table = TreeBasedTable.create();
    table.put("foo", 4, 'a');
    table.put("cat", 1, 'b');
    table.clear();
    populate(table, data);
    return table;
  }

  public void testCreateExplicitComparators() {
    table = TreeBasedTable.create(
        Collections.reverseOrder(), Ordering.usingToString());
    table.put("foo", 3, 'a');
    table.put("foo", 12, 'b');
    table.put("bar", 5, 'c');
    table.put("cat", 8, 'd');
    assertThat(table.rowKeySet()).has().exactly("foo", "cat", "bar").inOrder();
    assertThat(table.row("foo").keySet()).has().exactly(12, 3).inOrder();
  }

  public void testCreateCopy() {
    TreeBasedTable<String, Integer, Character> original = TreeBasedTable.create(
        Collections.reverseOrder(), Ordering.usingToString());
    original.put("foo", 3, 'a');
    original.put("foo", 12, 'b');
    original.put("bar", 5, 'c');
    original.put("cat", 8, 'd');
    table = TreeBasedTable.create(original);
    assertThat(table.rowKeySet()).has().exactly("foo", "cat", "bar").inOrder();
    assertThat(table.row("foo").keySet()).has().exactly(12, 3).inOrder();
    assertEquals(original, table);
  }

  public void testToString_ordered() {
    table = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    assertEquals("{bar={1=b}, foo={1=a, 3=c}}", table.toString());
    assertEquals("{bar={1=b}, foo={1=a, 3=c}}", table.rowMap().toString());
  }

  public void testCellSetToString_ordered() {
    table = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    assertEquals("[(bar,1)=b, (foo,1)=a, (foo,3)=c]",
        table.cellSet().toString());
  }

  public void testRowKeySetToString_ordered() {
    table = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    assertEquals("[bar, foo]", table.rowKeySet().toString());
  }

  public void testValuesToString_ordered() {
    table = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    assertEquals("[b, a, c]", table.values().toString());
  }

  public void testRowComparator() {
    sortedTable = TreeBasedTable.create();
    assertSame(Ordering.natural(), sortedTable.rowComparator());

    sortedTable = TreeBasedTable.create(
        Collections.reverseOrder(), Ordering.usingToString());
    assertSame(Collections.reverseOrder(), sortedTable.rowComparator());
  }

  public void testColumnComparator() {
    sortedTable = TreeBasedTable.create();
    assertSame(Ordering.natural(), sortedTable.columnComparator());

    sortedTable = TreeBasedTable.create(
        Collections.reverseOrder(), Ordering.usingToString());
    assertSame(Ordering.usingToString(), sortedTable.columnComparator());
  }

  public void testRowKeySetComparator() {
    sortedTable = TreeBasedTable.create();
    assertSame(Ordering.natural(),
        sortedTable.rowKeySet().comparator());

    sortedTable = TreeBasedTable.create(
        Collections.reverseOrder(), Ordering.usingToString());
    assertSame(Collections.reverseOrder(),
        sortedTable.rowKeySet().comparator());
  }

  public void testRowKeySetFirst() {
    sortedTable = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    assertSame("bar", sortedTable.rowKeySet().first());
  }

  public void testRowKeySetLast() {
    sortedTable = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    assertSame("foo", sortedTable.rowKeySet().last());
  }

  public void testRowKeySetHeadSet() {
    sortedTable = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    Set<String> set = sortedTable.rowKeySet().headSet("cat");
    assertEquals(Collections.singleton("bar"), set);
    set.clear();
    assertTrue(set.isEmpty());
    assertEquals(Collections.singleton("foo"), sortedTable.rowKeySet());
  }

  public void testRowKeySetTailSet() {
    sortedTable = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    Set<String> set = sortedTable.rowKeySet().tailSet("cat");
    assertEquals(Collections.singleton("foo"), set);
    set.clear();
    assertTrue(set.isEmpty());
    assertEquals(Collections.singleton("bar"), sortedTable.rowKeySet());
  }

  public void testRowKeySetSubSet() {
    sortedTable = create(
        "foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c', "dog", 2, 'd');
    Set<String> set = sortedTable.rowKeySet().subSet("cat", "egg");
    assertEquals(Collections.singleton("dog"), set);
    set.clear();
    assertTrue(set.isEmpty());
    assertEquals(ImmutableSet.of("bar", "foo"), sortedTable.rowKeySet());
  }

  public void testRowMapComparator() {
    sortedTable = TreeBasedTable.create();
    assertSame(Ordering.natural(), sortedTable.rowMap().comparator());

    sortedTable = TreeBasedTable.create(
        Collections.reverseOrder(), Ordering.usingToString());
    assertSame(Collections.reverseOrder(), sortedTable.rowMap().comparator());
  }

  public void testRowMapFirstKey() {
    sortedTable = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    assertSame("bar", sortedTable.rowMap().firstKey());
  }

  public void testRowMapLastKey() {
    sortedTable = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    assertSame("foo", sortedTable.rowMap().lastKey());
  }

  public void testRowKeyMapHeadMap() {
    sortedTable = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    Map<String, Map<Integer, Character>> map
        = sortedTable.rowMap().headMap("cat");
    assertEquals(1, map.size());
    assertEquals(ImmutableMap.of(1, 'b'), map.get("bar"));
    map.clear();
    assertTrue(map.isEmpty());
    assertEquals(Collections.singleton("foo"), sortedTable.rowKeySet());
  }

  public void testRowKeyMapTailMap() {
    sortedTable = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    Map<String, Map<Integer, Character>> map
        = sortedTable.rowMap().tailMap("cat");
    assertEquals(1, map.size());
    assertEquals(ImmutableMap.of(1, 'a', 3, 'c'), map.get("foo"));
    map.clear();
    assertTrue(map.isEmpty());
    assertEquals(Collections.singleton("bar"), sortedTable.rowKeySet());
  }

  public void testRowKeyMapSubMap() {
    sortedTable = create(
        "foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c', "dog", 2, 'd');
    Map<String, Map<Integer, Character>> map
        = sortedTable.rowMap().subMap("cat", "egg");
    assertEquals(ImmutableMap.of(2, 'd'), map.get("dog"));
    map.clear();
    assertTrue(map.isEmpty());
    assertEquals(ImmutableSet.of("bar", "foo"), sortedTable.rowKeySet());
  }

  public void testRowMapValuesAreSorted() {
    sortedTable = create(
        "foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c', "dog", 2, 'd');
    assertTrue(sortedTable.rowMap().get("foo") instanceof SortedMap);
  }

  public void testColumnKeySet_isSorted() {
    table = create("a", 2,  'X',
                   "a", 2,  'X',
                   "b", 3,  'X',
                   "b", 2,  'X',
                   "c", 10, 'X',
                   "c", 10, 'X',
                   "c", 20, 'X',
                   "d", 15, 'X',
                   "d", 20, 'X',
                   "d", 1,  'X',
                   "e", 5,  'X'
                  );
    assertEquals("[1, 2, 3, 5, 10, 15, 20]", table.columnKeySet().toString());
  }

  public void testColumnKeySet_isSortedWithRealComparator() {
    table = create(String.CASE_INSENSITIVE_ORDER,
                   Ordering.natural().reverse(),
                   "a", 2,  'X',
                   "a", 2,  'X',
                   "b", 3,  'X',
                   "b", 2,  'X',
                   "c", 10, 'X',
                   "c", 10, 'X',
                   "c", 20, 'X',
                   "d", 15, 'X',
                   "d", 20, 'X',
                   "d", 1,  'X',
                   "e", 5,  'X'
                  );
    assertEquals("[20, 15, 10, 5, 3, 2, 1]", table.columnKeySet().toString());
  }

  public void testColumnKeySet_empty() {
    table = create();
    assertEquals("[]", table.columnKeySet().toString());
  }

  public void testColumnKeySet_oneRow() {
    table = create("a", 2,  'X',
                   "a", 1,  'X'
                  );
    assertEquals("[1, 2]", table.columnKeySet().toString());
  }

  public void testColumnKeySet_oneColumn() {
    table = create("a", 1,  'X',
                   "b", 1,  'X'
                  );
    assertEquals("[1]", table.columnKeySet().toString());
  }

  public void testColumnKeySet_oneEntry() {
    table = create("a", 1,  'X');
    assertEquals("[1]", table.columnKeySet().toString());
  }

  public void testRowEntrySetContains() {
    table =
        sortedTable =
            create("a", 2, 'X', "a", 2, 'X', "b", 3, 'X', "b", 2, 'X', "c", 10,
                'X', "c", 10, 'X', "c", 20, 'X', "d", 15, 'X', "d", 20, 'X',
                "d", 1, 'X', "e", 5, 'X');
    SortedMap<Integer, Character> row = sortedTable.row("c");
    Set<Map.Entry<Integer, Character>> entrySet = row.entrySet();
    assertTrue(entrySet.contains(Maps.immutableEntry(10, 'X')));
    assertTrue(entrySet.contains(Maps.immutableEntry(20, 'X')));
    assertFalse(entrySet.contains(Maps.immutableEntry(15, 'X')));
    entrySet = row.tailMap(15).entrySet();
    assertFalse(entrySet.contains(Maps.immutableEntry(10, 'X')));
    assertTrue(entrySet.contains(Maps.immutableEntry(20, 'X')));
    assertFalse(entrySet.contains(Maps.immutableEntry(15, 'X')));
  }

  public void testRowEntrySetRemove() {
    table =
        sortedTable =
            create("a", 2, 'X', "a", 2, 'X', "b", 3, 'X', "b", 2, 'X', "c", 10,
                'X', "c", 10, 'X', "c", 20, 'X', "d", 15, 'X', "d", 20, 'X',
                "d", 1, 'X', "e", 5, 'X');
    SortedMap<Integer, Character> row = sortedTable.row("c");
    Set<Map.Entry<Integer, Character>> entrySet = row.tailMap(15).entrySet();
    assertFalse(entrySet.remove(Maps.immutableEntry(10, 'X')));
    assertTrue(entrySet.remove(Maps.immutableEntry(20, 'X')));
    assertFalse(entrySet.remove(Maps.immutableEntry(15, 'X')));
    entrySet = row.entrySet();
    assertTrue(entrySet.remove(Maps.immutableEntry(10, 'X')));
    assertFalse(entrySet.remove(Maps.immutableEntry(20, 'X')));
    assertFalse(entrySet.remove(Maps.immutableEntry(15, 'X')));
  }

  public void testRowSize() {
    table =
        sortedTable =
            create("a", 2, 'X', "a", 2, 'X', "b", 3, 'X', "b", 2, 'X', "c", 10,
                'X', "c", 10, 'X', "c", 20, 'X', "d", 15, 'X', "d", 20, 'X',
                "d", 1, 'X', "e", 5, 'X');
    SortedMap<Integer, Character> row = sortedTable.row("c");
    assertEquals(row.size(), 2);
    assertEquals(row.tailMap(15).size(), 1);
  }

  public void testSubRowClearAndPut() {
    table = create("foo", 1, 'a', "bar", 1, 'b', "foo", 3, 'c');
    SortedMap<Integer, Character> row = (SortedMap<Integer, Character>) table.row("foo");
    SortedMap<Integer, Character> subRow = row.tailMap(2);
    assertEquals(ImmutableMap.of(1, 'a', 3, 'c'), row);
    assertEquals(ImmutableMap.of(3, 'c'), subRow);
    table.remove("foo", 3);
    assertEquals(ImmutableMap.of(1, 'a'), row);
    assertEquals(ImmutableMap.of(), subRow);
    table.remove("foo", 1);
    assertEquals(ImmutableMap.of(), row);
    assertEquals(ImmutableMap.of(), subRow);
    table.put("foo", 2, 'b');
    assertEquals(ImmutableMap.of(2, 'b'), row);
    assertEquals(ImmutableMap.of(2, 'b'), subRow);
    row.clear();
    assertEquals(ImmutableMap.of(), row);
    assertEquals(ImmutableMap.of(), subRow);
    table.put("foo", 5, 'x');
    assertEquals(ImmutableMap.of(5, 'x'), row);
    assertEquals(ImmutableMap.of(5, 'x'), subRow);
  }
}

