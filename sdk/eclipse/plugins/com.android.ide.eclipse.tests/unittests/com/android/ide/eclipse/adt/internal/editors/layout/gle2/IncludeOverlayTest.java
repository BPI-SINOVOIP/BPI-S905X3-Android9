/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.ide.eclipse.adt.internal.editors.layout.gle2;

import org.eclipse.swt.graphics.Rectangle;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Random;

import junit.framework.TestCase;

public class IncludeOverlayTest extends TestCase {

    public void testSubtractRectangles() throws Exception {
        checkSubtract(new Rectangle(0, 0, 100, 80), Collections.<Rectangle> emptyList());

        checkSubtract(new Rectangle(0, 0, 100, 80), Arrays.asList(new Rectangle(50, 50, 20, 20)));

        checkSubtract(new Rectangle(0, 0, 100, 80), Arrays.asList(new Rectangle(50, 50, 20, 20),
                new Rectangle(90, 90, 10, 10)));

        checkSubtract(new Rectangle(0, 0, 100, 80), Arrays.asList(new Rectangle(50, 50, 20, 20),
                new Rectangle(90, 90, 10, 10), new Rectangle(0, 0, 10, 10)));

    }

    private void checkSubtract(Rectangle rectangle, List<Rectangle> holes) {
        Collection<Rectangle> result = IncludeOverlay.subtractRectangles(rectangle, holes);

        // Do some Monte Carlo testing - pick random coordinates and check that if they
        // are within one of the holes then they are not in the result list and vice versa
        Random random = new Random(42L);
        for (int i = 0; i < 1000; i++) {
            int x = random.nextInt(rectangle.width);
            int y = random.nextInt(rectangle.height);

            boolean inHole = false;
            for (Rectangle hole : holes) {
                if (hole.contains(x, y)) {
                    inHole = true;
                }
            }

            boolean inResult = false;
            for (Rectangle r : result) {
                if (r.contains(x, y)) {
                    inResult = true;
                    break;
                }
            }

            if (inHole == inResult) {
                fail("Wrong result at (" + x + "," + y + ") for rectangle=" + rectangle
                        + " and holes=" + holes + " where inHole=inResult="
                        + inResult);
            }
        }
    }
}
