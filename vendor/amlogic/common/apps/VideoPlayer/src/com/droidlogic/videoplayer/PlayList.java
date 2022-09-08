/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/
package com.droidlogic.videoplayer;

import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import android.widget.Toast;
import android.content.Context;
import android.util.Log;
import com.droidlogic.app.SystemControlManager;


//simple list control
public class PlayList {

        private List<String> hfilelist = null;
        private int pos = 0;
        private static PlayList hlist = null;
        protected String rootPath = null;
        private List<Boolean> hErrorList = null;
        private static Context mContext = null;
        private SystemControlManager mSystemControl1 = SystemControlManager.getInstance();

        public static void setContext (Context context) {
            mContext = context;
        }

        public int getSize() {
            return hfilelist.size();
        }

        public static PlayList getinstance() {
            if (hlist == null) {
                hlist = new PlayList();
            }
            return hlist;
        }

        public String safeGetList (int pos) {
            String str = null;
            if (hfilelist != null) {
                if (!hfilelist.isEmpty()) {
                    try {
                        str = hfilelist.get (pos);
                    }
                    catch (IndexOutOfBoundsException e) {
                    }
                }
            }
            /*if(str == null) {
                Toast.makeText(mContext,
                    mContext.getText(R.string.str_no_file),
                    Toast.LENGTH_SHORT).show();
            }*/
            return str;
        }

        public  void setlist (List<String> filelist, int startpos) {
            hfilelist = filelist;
            pos = startpos;
            int total = filelist.size();
            hErrorList = new ArrayList<Boolean>();
            for (int i = 0; i < total; i++) {
                hErrorList.add (false);
            }
        }

        public void setErrorList (boolean err) {
            hErrorList.set (pos, err);
        }

        public boolean isAllError() {
            Iterator<Boolean> it = hErrorList.iterator();
            while (it.hasNext()) {
                if (!it.next()) {
                    return false;
                }
            }
            return true;
        }

        public void removeCurPath() {
            if (hfilelist != null) {
                if (!hfilelist.isEmpty()) {
                    hfilelist.remove (pos);
                }
            }
        }

        private boolean isRepeatPlayModeOnce() {
            boolean ret = mSystemControl1.getPropertyBoolean("vendor.sys.isRepeatPlayModeOnce.enable", false);
            return ret;
        }

        public String movenext() {
            if (pos < hfilelist.size() - 1) {
                pos ++;
            }
            else if (isRepeatPlayModeOnce()){
                if (pos == hfilelist.size() - 1) {
                    return null;
                }
            } else {
                pos = 0;
            }
            return safeGetList (pos);
        }


        public String moveprev() {
            if (pos > 0) {
                pos--;
            }
            else {
                pos = hfilelist.size() - 1;
            }
            return safeGetList (pos);
        }

        public String getcur() {
            //return hfilelist.get(pos);
            return safeGetList (pos);
        }

        public String get (int idx) {
            String str = null;
            if (idx < hfilelist.size()) {
                str = safeGetList (idx);
            }
            return str;
        }

        public String movehead() {
            pos = 0;
            return safeGetList (pos);
        }

        public String movelast() {
            pos = hfilelist.size() - 1;
            return safeGetList (pos);
        }

        public int getindex() {
            return pos;
        }
}
