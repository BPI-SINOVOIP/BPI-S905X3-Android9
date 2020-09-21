/*
 * Copyright (C) 2011 The Android Open Source Project
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

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

/**
 * Helper class to display a matrix of String elements so each element column is lined up
 */
public class TableFormatter {

    private int mColumnSpacing = 2;

    /**
     * Sets the number of whitespace characters between each column.
     *
     * @param spacing the number of whitespace characters
     * @return the {@link TableFormatter}
     */
    public TableFormatter setColumnSpacing(int spacing) {
        mColumnSpacing = spacing;
        return this;
    }

    /**
     * Display given String elements as an table with aligned columns.
     *
     * @param table a matrix of String elements. Rows can be different length
     * @param writer the {@link PrintWriter} to dump output to
     */
    public void displayTable(List<List<String>> table, PrintWriter writer) {
        if (table.size() <= 0) {
            throw new IllegalArgumentException();
        }
        // build index of max column sizes
        List<Integer> maxColumnSizes = getColumnSizes(table);
        for (List<String> rowData : table) {
            for (int col = 0; col < rowData.size(); col++) {
                writer.append(rowData.get(col));
                int numPaddingChars = maxColumnSizes.get(col) - rowData.get(col).length();
                insertPadding(numPaddingChars, writer);
            }
            writer.println();
        }
    }

    private void insertPadding(int numChars, PrintWriter writer) {
        for (int i = 0; i < numChars; i++) {
            writer.append(' ');
        }
    }

    private List<Integer> getColumnSizes(List<List<String>> table) {
        List<Integer> maxColumnSizes = new ArrayList<Integer>();
        for (List<String> rowData : table) {
            for (int colIndex = 0; colIndex < rowData.size(); colIndex++) {
                if (colIndex >= maxColumnSizes.size()) {
                    maxColumnSizes.add(colIndex, 0);
                }
                // add column spacing to string length so there is at least that amount of space
                // between columns
                int stringSize = rowData.get(colIndex).length() + mColumnSpacing;
                if (maxColumnSizes.get(colIndex) < stringSize) {
                    maxColumnSizes.set(colIndex, stringSize);
                }
            }
        }

        return maxColumnSizes;
    }
}
