/*
 * Copyright 2012 AndroidPlot.com
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

package com.androidplot.ui;

import android.graphics.RectF;

import java.util.Iterator;

public abstract class TableModel {
    private TableOrder order;

    protected TableModel(TableOrder order) {
        setOrder(order);
    }

    public abstract Iterator<RectF> getIterator(RectF tableRect, int totalElements);

    //public abstract RectF getCellRect(RectF tableRect, int numElements);

    public TableOrder getOrder() {
        return order;
    }

    public void setOrder(TableOrder order) {
        this.order = order;
    }

    public enum Axis {
        ROW,
        COLUMN
    }

    public enum CellSizingMethod {
        FIXED,
        FILL
    }
}
