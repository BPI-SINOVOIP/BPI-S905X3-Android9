package com.xtremelabs.robolectric.shadows;

import com.xtremelabs.robolectric.internal.Implementation;
import com.xtremelabs.robolectric.internal.Implements;

import android.database.CursorIndexOutOfBoundsException;
import android.database.MatrixCursor;

import java.util.ArrayList;
import java.util.List;

@Implements(MatrixCursor.class)
public class ShadowMatrixCursor extends ShadowAbstractCursor {
    private List<Object[]> data = new ArrayList<Object[]>();

    public void __constructor__(String[] columns) {
        __constructor__(columns, 0);
    }

    public void __constructor__(String[] columns, int initialCapacity) {
        columnNameArray = columns;
    }

    @Implementation
    public void addRow(Object[] data) {
        rowCount++;
        this.data.add(data);
    }

    @Implementation
    public String getString(int column) {
        Object columnValue = get(column);
        return columnValue == null ? null : columnValue.toString();
    }

    @Implementation
    public long getLong(int column) {
        Number numberValue = (Number) get(column);
        return numberValue == null ? 0 : numberValue.longValue();
    }

    @Implementation
    public short getShort(int column) {
        Number numberValue = (Number) get(column);
        return numberValue == null ? 0 : numberValue.shortValue();
    }

    @Implementation
    public int getInt(int column) {
        Number numberValue = (Number) get(column);
        return numberValue == null ? 0 : numberValue.intValue();
    }

    @Implementation
    public float getFloat(int column) {
        Number numberValue = (Number) get(column);
        return numberValue == null ? 0.0f : numberValue.floatValue();
    }

    @Implementation
    public double getDouble(int column) {
        Number numberValue = (Number) get(column);
        return numberValue == null ? 0.0 : numberValue.doubleValue();
    }

    @Implementation
    public byte[] getBlob(int column) {
        return (byte[]) get(column);
    }

    @Implementation
    public boolean isNull(int column) {
        return get(column) == null;
    }

    private Object get(int column) {
        if (column < 0 || column >= columnNameArray.length) {
            throw new CursorIndexOutOfBoundsException(null);
        }
        if (currentRowNumber < 0) {
            throw new CursorIndexOutOfBoundsException("Before first row.");
        }
        if (currentRowNumber >= rowCount) {
            throw new CursorIndexOutOfBoundsException("After last row.");
        }
        return data.get(currentRowNumber)[column];
    }
}
