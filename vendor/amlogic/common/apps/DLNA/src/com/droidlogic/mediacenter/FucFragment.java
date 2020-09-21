/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
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
package com.droidlogic.mediacenter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.droidlogic.mediacenter.R;
import com.droidlogic.mediacenter.airplay.setting.SettingsPreferences;
import com.droidlogic.mediacenter.dlna.DmpFragment;
import com.droidlogic.mediacenter.dlna.DmpStartFragment;
import com.droidlogic.mediacenter.dlna.PrefUtils;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.app.ListFragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.SimpleAdapter;
/**
 * @Package
 * @Description
 *
 * Copyright (c) Inspur Group Co., Ltd. Unpublished
 *
 * Inspur Group Co., Ltd.
 * Proprietary & Confidential
 *
 * This source code and the algorithms implemented therein constitute
 * confidential information and may comprise trade secrets of Inspur
 * or its associates, and any use thereof is subject to the terms and
 * conditions of the Non-Disclosure Agreement pursuant to which this
 * source code was originally received.
 */
/**
 * @ClassName FucFragment
 * @Description TODO
 * @Date 2013-8-26
 * @Email
 * @Author
 * @Version V1.0
 */
public class FucFragment extends ListFragment {
        private static final int ITEM_DMPSCAN = 0;
        private static final int ITEM_DMPSERVICE = 1;
        private static final int ITEM_AIRPLAY = 3;
        private static final int ITEM_ABOUT = 2;
        private static final String LIST_TITLE = "title";
        private static final String LIST_IMG = "img";
        private static final String LIST_SUMMARY = "summary";
        private List<Map<String, Object>> mData;
        private boolean mDualPane;
        private int mCurCheckPosition = 3;
        /*------------------------------------------------------------------------------------*/
        @Override
        public void onActivityCreated ( Bundle savedInstanceState ) {
            super.onActivityCreated ( savedInstanceState );
            mData = getData();
            SimpleAdapter adapter = new SimpleAdapter ( getActivity(), mData,
                    R.layout.funclist, new String[] {LIST_IMG, LIST_TITLE, LIST_SUMMARY},
                    new int[] {R.id.img, (int)PrefUtils.getResource("com.android.internal.R$id","title"), (int)PrefUtils.getResource("com.android.internal.R$id","summary")} );
            this.setListAdapter ( adapter );
            View detailsFrame = getActivity().findViewById ( R.id.frag_detail );
            mDualPane = detailsFrame != null && detailsFrame.getVisibility() == View.VISIBLE;
            if ( savedInstanceState != null ) {
                mCurCheckPosition = savedInstanceState.getInt ( "curChoice", 1 );
            }
            if ( mDualPane ) {
                getListView().setChoiceMode ( ListView.CHOICE_MODE_SINGLE );
                showDetails ( mCurCheckPosition );
            }
        }
        @Override
        public void onSaveInstanceState ( Bundle outState ) {
            // TODO Auto-generated method stub
            super.onSaveInstanceState ( outState );
            outState.putInt ( "curChoice", mCurCheckPosition );
        }

        /**
         * @Description TODO
         * @param mCurCheckPosition2
         */
        private void showDetails ( int index ) {
            mCurCheckPosition = index;
            if ( mDualPane ) {
                getListView().setItemChecked ( index, true );
                Fragment dmpFragment = getFragmentManager().findFragmentById ( R.id.frag_detail );
                if ( mCurCheckPosition == ITEM_DMPSCAN ) {
                    showDmpScan ( dmpFragment );
                } else if ( mCurCheckPosition == ITEM_DMPSERVICE ) {
                    showDmpShow ( dmpFragment );
                } else if ( mCurCheckPosition == ITEM_ABOUT ) {
                    showAboutFragment ( dmpFragment );
                }
            }
        }

        private void showDmpShow ( Fragment fragment ) {
            if ( ( fragment == null ) || ! ( fragment instanceof DmpStartFragment ) ) {
                fragment = new DmpStartFragment();
                FragmentTransaction ft = getFragmentManager().beginTransaction();
                ft.replace ( R.id.frag_detail, fragment );
                ft.setTransition ( FragmentTransaction.TRANSIT_FRAGMENT_FADE );
                ft.commit();
            }
        }

        private void showAirplay ( Fragment fragment ) {
            if ( ( fragment == null ) || ! ( fragment instanceof SettingsPreferences ) ) {
                fragment = new SettingsPreferences();
                FragmentTransaction ft = getFragmentManager().beginTransaction();
                ft.replace ( R.id.frag_detail, fragment );
                ft.setTransition ( FragmentTransaction.TRANSIT_FRAGMENT_FADE );
                ft.commit();
            }
        }
        private void showAboutFragment ( Fragment fragment ) {
            if ( ( fragment == null ) || ! ( fragment instanceof SettingsFragment ) ) {
                fragment = new SettingsFragment();
                FragmentTransaction ft = getFragmentManager().beginTransaction();
                ft.replace ( R.id.frag_detail, fragment );
                ft.setTransition ( FragmentTransaction.TRANSIT_FRAGMENT_FADE );
                ft.commit();
            }
        }
        private void showDmpScan ( Fragment fragment ) {
            if ( ( fragment == null ) || ! ( fragment instanceof DmpFragment ) ) {
                fragment = new DmpFragment();
                FragmentTransaction ft = getFragmentManager().beginTransaction();
                ft.replace ( R.id.frag_detail, fragment );
                ft.setTransition ( FragmentTransaction.TRANSIT_FRAGMENT_FADE );
                ft.commit();
            }
        }

        private List<Map<String, Object>> getData() {
            List<Map<String, Object>> list = new ArrayList<Map<String, Object>>();
            Map<String, Object>map = new HashMap<String, Object>();
            map.put ( LIST_TITLE, getString ( R.string.dmp_title ) );
            map.put ( LIST_IMG, R.drawable.dmp );
            map.put ( LIST_SUMMARY, getString ( R.string.dmp_summary ) );
            list.add ( map );
            map = new HashMap<String, Object>();
            map.put ( LIST_TITLE, getString ( R.string.dmr_title ) );
            map.put ( LIST_IMG, R.drawable.dmr );
            map.put ( LIST_SUMMARY, getString ( R.string.dmr_summary ) );
            list.add ( map );
            //map = new HashMap<String, Object>();
            //map.put ( LIST_TITLE, getString ( R.string.airplay_title ) );
            //map.put ( LIST_IMG, R.drawable.airplay );
            //map.put ( LIST_SUMMARY, getString ( R.string.airplay_summary ) );
            //list.add ( map );
            map = new HashMap<String, Object>();
            map.put ( LIST_TITLE, getString ( R.string.about ) );
            map.put ( LIST_IMG, R.drawable.settings );
            map.put ( LIST_SUMMARY, getString ( R.string.settings_summary ) );
            list.add ( map );
            return list;
        }

        @Override
        public void onListItemClick ( ListView l, View v, int position, long id ) {
            showDetails ( position );
        }
}
