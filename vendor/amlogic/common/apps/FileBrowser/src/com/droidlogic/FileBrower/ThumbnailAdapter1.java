/******************************************************************
*
*Copyright (C) 2012  Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.FileBrower;

import java.io.ByteArrayInputStream;
import java.util.List;
import java.util.Map;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.widget.ImageView;
import android.widget.SimpleAdapter;

import com.droidlogic.FileBrower.FileBrowerDatabase.ThumbnailCursor;




public class ThumbnailAdapter1 extends SimpleAdapter{
    public ThumbnailAdapter1(Context context,
        List<? extends Map<String, ?>> data, int resource, String[] from,
        int[] to) {
        super(context, data, resource, from, to);
        // TODO Auto-generated constructor stub
    }

    public void setViewImage (ImageView v, String value) {
        ThumbnailCursor cc = null;
        try {
            cc = ThumbnailView1.db.getThumbnailByPath(value);
            if (cc != null && cc.moveToFirst()) {
                if (cc.getCount()  > 0) {
                    Drawable drawable = Drawable.createFromStream(
                        new ByteArrayInputStream(cc.getColFileData()),
                        "image.png");
                    if (drawable != null)
                        v.setImageDrawable(drawable);
                    else
                        super.setViewImage(v, R.drawable.item_preview_photo);
                } else
                    super.setViewImage(v, R.drawable.item_preview_photo);
            }
        } finally {
            if (cc != null) cc.close();
        }
    }
}