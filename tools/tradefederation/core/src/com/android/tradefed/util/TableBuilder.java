/*
 * Copyright (C) 2018 The Android Open Source Project
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

import java.util.LinkedList;
import java.util.List;
import java.util.stream.IntStream;

/** Helper class to display a matrix of String elements in a table. */
public class TableBuilder {
    /*
     *  Sample:
     *
     *  +======================Metric Regressions=======================+
     *  | Metric Name | Pre Avg | Post Avg | False Positive Probability |
     *  +===============================================================+
     *  | Run Metrics (1 compared, 0 changed)                           |
     *  +---------------------------------------------------------------+
     *  | Test Metrics (9 compared, 3 changed)                          |
     *  +---------------------------------------------------------------+
     *  | > com.android.uiautomator.samples.skeleton.DemoTestCase#testD |
     *  | emo3                                                          |
     *  | stat1       | 100.54  | 817.40   | 0.101                      |
     *  |                                                               |
     *  | > com.android.uiautomator.samples.skeleton.DemoTestCase#testD |
     *  | emo2                                                          |
     *  | stat3       | 12.32   | 100.91   | 0.101                      |
     *  | stat4       | 104.46  | 24.61    | 0.101                      |
     *  |                                                               |
     *  +===============================================================+
     */
    // Blank space on the left of the table.
    private int mTableOffset = 2;
    // Padding on left & right side of each column.
    private int mPadding = 1;
    private final int mColumns;
    private int mRowLength;
    private int[] mColumnWidth;

    private List<Line> mTable;
    private StringBuilder mBuilder;

    private interface Line {
        void build();
    }

    private class Separator implements Line {
        char mEnd;
        char mPipe;
        String mText;

        Separator(char end, char pipe, String text) {
            mEnd = end;
            mPipe = pipe;
            mText = text;
        }

        @Override
        public void build() {
            addPadding(mTableOffset);
            mBuilder.append(mEnd);
            int blanks = mRowLength - mText.length() - 2;
            for (int i = 0; i < blanks / 2; i++) {
                mBuilder.append(mPipe);
            }
            mBuilder.append(mText);
            for (int i = 0; i < blanks / 2; i++) {
                mBuilder.append(mPipe);
            }
            // Correct for odd length
            if (blanks % 2 != 0) {
                mBuilder.append(mPipe);
            }
            mBuilder.append(mEnd).append('\n');
        }
    }

    private class SingleColumn implements Line {
        String mText;

        SingleColumn(String text) {
            mText = text;
        }

        @Override
        public void build() {
            int width = mRowLength - 2 - mPadding * 2; // Deduct boundaries
            String text = mText;
            while (text != null) {
                addPadding(mTableOffset);
                mBuilder.append('|');
                addPadding(mPadding);
                if (text.length() > width) {
                    mBuilder.append(text.substring(0, width));
                    text = text.substring(width);
                } else {
                    mBuilder.append(text);
                    addPadding(width - text.length());
                    text = null;
                }
                addPadding(mPadding);
                mBuilder.append("|\n");
            }
        }
    }

    private class MultiColumn implements Line {
        String[] mColumns;

        MultiColumn(String[] columns) {
            mColumns = columns;
        }

        @Override
        public void build() {
            addPadding(mTableOffset);
            mBuilder.append("|");
            for (int i = 0; i < mColumns.length; i++) {
                addPadding(mPadding);
                mBuilder.append(mColumns[i]);
                addPadding(mColumnWidth[i] - mColumns[i].length() + mPadding);
                mBuilder.append('|');
            }
            mBuilder.append('\n');
        }
    }

    /**
     * Constructs a TableBuilder with specific number of columns.
     *
     * @param numColumns number of columns in this table.
     */
    public TableBuilder(int numColumns) {
        mColumns = numColumns;
        mColumnWidth = new int[numColumns];
        mTable = new LinkedList<>();
    }

    /**
     * Sets the number of white space before and after each column element
     *
     * @param padding the number of white space
     * @return this
     */
    public TableBuilder setPadding(int padding) {
        mPadding = padding;
        return this;
    }

    /**
     * Sets the number of white space on the left of the whole table
     *
     * @param offset the number of white space
     * @return this
     */
    public TableBuilder setOffset(int offset) {
        mTableOffset = offset;
        return this;
    }

    /**
     * Adds a title to this table. Sample: +======================TITLE=======================+
     *
     * @param title title
     * @return this
     */
    public TableBuilder addTitle(String title) {
        mTable.add(new Separator('+', '=', title));
        return this;
    }

    /**
     * Adds a row separator like: +---------------------------------------------+
     *
     * @return this
     */
    public TableBuilder addSingleLineSeparator() {
        return addSeparator('+', '-');
    }

    /**
     * Adds a row separator like: +=============================================+
     *
     * @return this
     */
    public TableBuilder addDoubleLineSeparator() {
        return addSeparator('+', '=');
    }

    /**
     * Adds a row separator like: | | (blank space between two pipes)
     *
     * @return this
     */
    public TableBuilder addBlankLineSeparator() {
        return addSeparator('|', ' ');
    }

    /**
     * Adds a custom row separator.
     *
     * @param end the two end character.
     * @param pipe the character connecting two ends
     * @return this
     */
    public TableBuilder addSeparator(char end, char pipe) {
        mTable.add(new Separator(end, pipe, ""));
        return this;
    }

    /**
     * Adds a single long line. TableBuilder will wrap it if it is too long. See example above.
     *
     * @param line the line.
     * @return this
     */
    public TableBuilder addLine(String line) {
        mTable.add(new SingleColumn(line));
        return this;
    }

    /**
     * Adds a line. The number of columns in line must equal numColumns provided in the constructor.
     *
     * @param line the line.
     * @return this
     * @throws IllegalArgumentException when the number of columns in line does not agree with
     *     numColumns provided in the constructor.
     */
    public TableBuilder addLine(String[] line) {
        if (line.length != mColumns) {
            throw new IllegalArgumentException(
                    String.format("Expect %d columns, actual %d columns", mColumns, line.length));
        }
        mTable.add(new MultiColumn(line));
        for (int i = 0; i < mColumns; i++) {
            if (line[i].length() > mColumnWidth[i]) {
                mColumnWidth[i] = line[i].length();
            }
        }
        return this;
    }

    /**
     * Builds the table and return as a string.
     *
     * @return the table in string format.
     */
    public String build() {
        mBuilder = new StringBuilder();
        mRowLength = IntStream.of(mColumnWidth).map(i -> i + 2 * mPadding + 1).sum() + 1;
        for (Line line : mTable) {
            line.build();
        }
        return mBuilder.toString();
    }

    private void addPadding(int padding) {
        for (int i = 0; i < padding; i++) {
            mBuilder.append(' ');
        }
    }
}
