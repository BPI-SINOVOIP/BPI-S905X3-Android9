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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

/**
 * A directed unweighted graphs implementation. The vertex type can be specified.
 */
public class DirectedGraph<V> {

    /**
     * The implementation here is basically an adjacency list, but instead
     * of an array of lists, a Map is used to map each vertex to its list of
     * adjacent vertices.
     */
    private Map<V,List<V>> neighbors = new HashMap<V, List<V>>();

    /**
     * String representation of graph.
     */
    @Override
    public String toString() {
        StringBuffer s = new StringBuffer();
        for (V v: neighbors.keySet()) s.append("\n    " + v + " -> " + neighbors.get(v));
        return s.toString();
    }

    /**
     * Add a vertex to the graph.  Inop if vertex is already in graph.
     */
    public void addVertice(V vertex) {
        if (neighbors.containsKey(vertex)) {
            return;
        }
        neighbors.put(vertex, new ArrayList<V>());
    }

    /**
     * True if graph contains vertex. False otherwise.
     */
    public boolean contains(V vertex) {
        return neighbors.containsKey(vertex);
    }

    /**
     * Add an edge to the graph; if either vertex does not exist, it's added.
     * This implementation allows the creation of multi-edges and self-loops.
     */
    public void addEdge(V from, V to) {
        this.addVertice(from);
        this.addVertice(to);
        neighbors.get(from).add(to);
    }

    /**
     * Remove an edge from the graph.
     *
     * @throws IllegalArgumentException if either vertex doesn't exist.
     */
    public void removeEdge(V from, V to) {
        if (!(this.contains(from) && this.contains(to))) {
            throw new IllegalArgumentException("Nonexistent vertex");
        }
        neighbors.get(from).remove(to);
    }

    /**
     * Return a map representation the in-degree of each vertex.
     */
    private Map<V, Integer> inDegree() {
        Map<V, Integer> result = new HashMap<V, Integer>();
        // All initial in-degrees are 0
        for (V v: neighbors.keySet()) {
            result.put(v, 0);
        }
        // Iterate over an count the in-degree
        for (V from: neighbors.keySet()) {
            for (V to: neighbors.get(from)) {
                result.put(to, result.get(to) + 1);
            }
        }
        return result;
    }

    /**
     * Return a List of the topological sort of the vertices; null for no such sort.
     */
    private List<V> topSort() {
        Map<V, Integer> degree = inDegree();
        // Determine all vertices with zero in-degree
        Stack<V> zeroVerts = new Stack<V>();
        for (V v: degree.keySet()) {
            if (degree.get(v) == 0) {
                zeroVerts.push(v);
            }
        }
        // Determine the topological order
        List<V> result = new ArrayList<V>();
        while (!zeroVerts.isEmpty()) {
            // Choose a vertex with zero in-degree
            V v = zeroVerts.pop();
            // Vertex v is next in topol order
            result.add(v);
            // "Remove" vertex v by updating its neighbors
            for (V neighbor: neighbors.get(v)) {
                degree.put(neighbor, degree.get(neighbor) - 1);
                // Remember any vertices that now have zero in-degree
                if (degree.get(neighbor) == 0) {
                    zeroVerts.push(neighbor);
                }
            }
        }
        // Check that we have used the entire graph (if not, there was a cycle)
        if (result.size() != neighbors.size()) {
            return null;
        }
        return result;
    }

    /**
     * True if graph is a dag (directed acyclic graph).
     */
    public boolean isDag() {
        return topSort() != null;
    }
}
