/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.util;

import junit.framework.TestCase;

/**
 * Unit tests for {@link DirectedGraph}
 */
public class DirectedGraphTest extends TestCase {

    public void testBasicGraph() {
        DirectedGraph<Integer> graph = new DirectedGraph<Integer>();
        graph.addEdge(0, 1);
        graph.addEdge(1, 2);
        graph.addEdge(2, 3);
        assertTrue(graph.contains(0));
        assertTrue(graph.contains(1));
        assertTrue(graph.contains(2));
        assertTrue(graph.contains(3));
        // no cycle
        assertTrue(graph.isDag());
    }

    public void testCyclicGraph() {
        DirectedGraph<Integer> graph = new DirectedGraph<Integer>();
        graph.addEdge(0, 1); graph.addEdge(0, 2); graph.addEdge(0, 3);
        graph.addEdge(1, 2); graph.addEdge(1, 3); graph.addEdge(2, 3);
        graph.addEdge(2, 4); graph.addEdge(4, 5); graph.addEdge(5, 6);
        // not a cycle.
        assertTrue(graph.isDag());
        // Create a cycle
        graph.addEdge(4, 1);
        // cycle now
        assertFalse(graph.isDag());
        // remove the cyle edge
        graph.removeEdge(4, 1);
        assertTrue(graph.isDag());
    }

    public void testRemoveUnexistingVertex() {
        DirectedGraph<Integer> graph = new DirectedGraph<Integer>();
        try {
            graph.addEdge(0, 1);
            // 2 doesn't exists as a destination vertex
            graph.removeEdge(0, 2);
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            // expected
        }

        try {
            graph.addEdge(0, 1);
            // 3 doesn't exists as a initial vertex
            graph.removeEdge(3, 0);
            fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            // expected
        }
    }
}
