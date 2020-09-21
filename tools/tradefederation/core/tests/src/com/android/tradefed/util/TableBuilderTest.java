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

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit test for {@link TableBuilder}. */
@RunWith(JUnit4.class)
public class TableBuilderTest {

    private static final String TABLE =
            "    +=============================Title=============================+\n"
                    + "    |  Col 1             |  Col 2  |  Col 3  |  Col 4  |  Column 5  |\n"
                    + "    +===============================================================+\n"
                    + "    |  A single line                                                |\n"
                    + "    +---------------------------------------------------------------+\n"
                    + "    |  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  |\n"
                    + "    |  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  |\n"
                    + "    |  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  |\n"
                    + "    |  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  |\n"
                    + "    |  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  |\n"
                    + "    |  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  |\n"
                    + "    |  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  |\n"
                    + "    |  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  |\n"
                    + "    |  xxxxxxxxxxxxxxxxxxxxxxxxxxxx                                 |\n"
                    + "    =###############################################################=\n"
                    + "    |  Example           |  1.11   |  2.22   |  3.33   |  4.44      |\n"
                    + "    |  Very Long String  |  1.11   |  2.22   |  3.33   |  4.44      |\n"
                    + "    |                                                               |\n"
                    + "    +---------------------------------------------------------------+\n"
                    + "    |  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  |\n"
                    + "    |  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  |\n"
                    + "    |  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  |\n"
                    + "    |  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  |\n"
                    + "    |  aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  |\n"
                    + "    |  aaaaa                                                        |\n"
                    + "    |                                                               |\n"
                    + "    +===============================================================+\n";

    @Test
    public void testBuildTable() {
        TableBuilder builder = new TableBuilder(5);
        builder.setOffset(4)
                .setPadding(2)
                .addTitle("Title")
                .addLine(new String[] {"Col 1", "Col 2", "Col 3", "Col 4", "Column 5"})
                .addDoubleLineSeparator()
                .addLine("A single line")
                .addSingleLineSeparator()
                .addLine(generateString(500, 'x'))
                .addSeparator('=', '#')
                .addLine(new String[] {"Example", "1.11", "2.22", "3.33", "4.44"})
                .addLine(new String[] {"Very Long String", "1.11", "2.22", "3.33", "4.44"})
                .addBlankLineSeparator()
                .addSingleLineSeparator()
                .addLine(generateString(300, 'a'))
                .addBlankLineSeparator()
                .addDoubleLineSeparator()
                .build();
        Assert.assertEquals(TABLE, builder.build());
    }

    private String generateString(int len, char c) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++) {
            sb.append(c);
        }
        return sb.toString();
    }
}
