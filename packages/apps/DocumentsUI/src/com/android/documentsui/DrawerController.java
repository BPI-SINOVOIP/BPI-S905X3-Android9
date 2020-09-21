/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.documentsui;

import static com.android.documentsui.base.SharedMinimal.DEBUG;

import android.annotation.IntDef;
import android.app.Activity;
import android.support.annotation.ColorRes;
import android.support.v4.app.ActionBarDrawerToggle;
import android.support.v4.widget.DrawerLayout;
import android.support.v4.widget.DrawerLayout.DrawerListener;
import android.util.Log;
import android.view.View;
import android.widget.Toolbar;

import com.android.documentsui.base.Display;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * A facade over the various pieces comprising "roots fragment in a Drawer".
 *
 * @see DrawerController#create(DrawerLayout)
 */
public abstract class DrawerController implements DrawerListener {
    public static final String TAG = "DrawerController";

    public abstract void update();
    public abstract void setOpen(boolean open);
    public abstract boolean isPresent();
    public abstract boolean isOpen();
    abstract void setTitle(String title);

    /**
     * Returns a controller suitable for {@code Layout}.
     */
    public static DrawerController create(Activity activity, ActivityConfig activityConfig) {

        DrawerLayout layout = (DrawerLayout) activity.findViewById(R.id.drawer_layout);

        if (layout == null) {
            return new DummyDrawerController();
        }

        View drawer = activity.findViewById(R.id.drawer_roots);
        Toolbar toolbar = (Toolbar) activity.findViewById(R.id.roots_toolbar);

        drawer.getLayoutParams().width = calculateDrawerWidth(activity);

        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                activity,
                layout,
                R.drawable.ic_hamburger,
                R.string.drawer_open,
                R.string.drawer_close);

        return new RuntimeDrawerController(layout, drawer, toggle, toolbar, activityConfig);
    }

    /**
     * Returns a controller suitable for {@code Layout}.
     */
    static DrawerController createDummy() {
        return new DummyDrawerController();
    }

    private static int calculateDrawerWidth(Activity activity) {
        // Material design specification for navigation drawer:
        // https://www.google.com/design/spec/patterns/navigation-drawer.html
        float width = Display.screenWidth(activity) - Display.actionBarHeight(activity);
        float maxWidth = activity.getResources().getDimension(R.dimen.max_drawer_width);
        int finalWidth = (int) ((width > maxWidth ? maxWidth : width));

        if (DEBUG)
            Log.d(TAG, "Calculated drawer width:" + (finalWidth / Display.density(activity)));

        return finalWidth;
    }

    /**
     * Runtime controller that manages a real drawer.
     */
    private static final class RuntimeDrawerController extends DrawerController
            implements ItemDragListener.DragHost {
        private static final int SPRING_TIMEOUT = 750;
        private final ActionBarDrawerToggle mToggle;
        private DrawerLayout mLayout;
        private View mDrawer;
        private Toolbar mToolbar;

        public RuntimeDrawerController(
                DrawerLayout layout,
                View drawer,
                ActionBarDrawerToggle toggle,
                Toolbar drawerToolbar,
                ActivityConfig activityConfig) {
            mToolbar = drawerToolbar;
            assert(layout != null);

            mLayout = layout;
            mDrawer = drawer;
            mToggle = toggle;

            mLayout.setDrawerListener(this);

            if (activityConfig.dragAndDropEnabled()) {
                View edge = layout.findViewById(R.id.drawer_edge);
                edge.setOnDragListener(new ItemDragListener<>(this, SPRING_TIMEOUT));
            }
        }

        @Override
        public void runOnUiThread(Runnable runnable) {
            mDrawer.post(runnable);
        }

        @Override
        public void setDropTargetHighlight(View v, boolean highlight) {
            assert (v.getId() == R.id.drawer_edge);

            @ColorRes int id = highlight ? R.color.item_doc_background_selected :
                android.R.color.transparent;
            v.setBackgroundColor(id);
        }

        @Override
        public void onDragEntered(View v) {
            // do nothing; let drawer only open for onViewHovered
        }

        @Override
        public void onDragExited(View v) {
            // do nothing
        }

        @Override
        public void onViewHovered(View v) {
            assert (v.getId() == R.id.drawer_edge);

            setOpen(true);
        }

        @Override
        public void onDragEnded() {
            // do nothing
        }

        @Override
        public void setOpen(boolean open) {
            if (open) {
                mLayout.openDrawer(mDrawer);
            } else {
                mLayout.closeDrawer(mDrawer);
            }
        }

        @Override
        public boolean isOpen() {
            return mLayout.isDrawerOpen(mDrawer);
        }

        @Override
        public boolean isPresent() {
            return true;
        }

        @Override
        void setTitle(String title) {
            mToolbar.setTitle(title);
        }

        @Override
        public void update() {
            mToggle.syncState();
        }

        @Override
        public void onDrawerSlide(View drawerView, float slideOffset) {
            mToggle.onDrawerSlide(drawerView, slideOffset);
        }

        @Override
        public void onDrawerOpened(View drawerView) {
            mToggle.onDrawerOpened(drawerView);
        }

        @Override
        public void onDrawerClosed(View drawerView) {
            mToggle.onDrawerClosed(drawerView);
        }

        @Override
        public void onDrawerStateChanged(int newState) {
            mToggle.onDrawerStateChanged(newState);
        }
    }

    /*
     * Dummy controller useful with clients that don't host a real drawer.
     */
    private static final class DummyDrawerController extends DrawerController {

        @Override
        public void setOpen(boolean open) {}

        @Override
        public boolean isOpen() {
            return false;
        }

        @Override
        public boolean isPresent() {
            return false;
        }

        @Override
        void setTitle(String title) {}

        @Override
        public void update() {}

        @Override
        public void onDrawerSlide(View drawerView, float slideOffset) {}

        @Override
        public void onDrawerOpened(View drawerView) {}

        @Override
        public void onDrawerClosed(View drawerView) {}

        @Override
        public void onDrawerStateChanged(int newState) {}
    }
}
