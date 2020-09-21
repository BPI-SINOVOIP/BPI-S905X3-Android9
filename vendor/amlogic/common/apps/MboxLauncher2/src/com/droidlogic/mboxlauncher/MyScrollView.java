/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description: java file
*/

package com.droidlogic.mboxlauncher;

import android.content.Context;
import android.widget.GridLayout;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.ScrollView;
import android.view.View;
import android.view.ViewGroup;
import android.view.MotionEvent;
import android.util.Log;
import android.util.AttributeSet;


public class MyScrollView extends ScrollView{
    private final static String TAG="MyScrollView";
    private Context mContext;

    public MyScrollView (Context context){
        super(context); 
    }
    
    public MyScrollView (Context context, AttributeSet attrs){
        super(context, attrs); 
        mContext = context;
    }
   
    @Override
    public boolean onTouchEvent (MotionEvent event){
        return false;       
    }

/*    @Override
    public boolean onGenericMotionEvent(MotionEvent event)
	{
	}*/
}
