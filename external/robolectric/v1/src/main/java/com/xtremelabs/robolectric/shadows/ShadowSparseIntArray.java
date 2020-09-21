package com.xtremelabs.robolectric.shadows;

import android.util.SparseArray;
import android.util.SparseIntArray;

import com.xtremelabs.robolectric.internal.Implementation;
import com.xtremelabs.robolectric.internal.Implements;
import com.xtremelabs.robolectric.internal.RealObject;

@Implements(SparseIntArray.class)
public class ShadowSparseIntArray {

	private SparseArray<Integer> sparseArray = new SparseArray<Integer>();
	
	@RealObject
	private SparseIntArray realObject;
	
	@Implementation
	public int get( int key ){
		return get( key, 0 );
	}
	
	@Implementation
	public int get(int key, int valueIfKeyNotFound){
		return sparseArray.get( key, valueIfKeyNotFound );
	}
	
	@Implementation
	public void put( int key, int value ){
		sparseArray.put( key, value );
	}
	
	@Implementation
	public int size() {
		return sparseArray.size();
	}
	
	@Implementation
	public int indexOfValue( int value ) {
		return sparseArray.indexOfValue( value );
	}
	
	@Implementation
	public int keyAt( int index ){
		return sparseArray.keyAt( index );
	}
	
	@Implementation
	public int valueAt( int index ){
		return sparseArray.valueAt( index );
	}
	
	@Implementation
	@Override
	public SparseIntArray clone() {
		SparseIntArray clone = new SparseIntArray();
		for (int i = 0, length = size(); i < length; i++) {
			clone.put( keyAt(i), valueAt(i) );
		}
		return clone;
	}
}
