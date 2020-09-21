/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.ide.eclipse.hierarchyviewer.views;

import com.android.hierarchyviewerlib.actions.LoadOverlayAction;
import com.android.hierarchyviewerlib.actions.RefreshPixelPerfectAction;
import com.android.hierarchyviewerlib.actions.SavePixelPerfectAction;
import com.android.hierarchyviewerlib.ui.PixelPerfect;

import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;

public class PixelPerfectView extends ViewPart {

    public static final String ID =
            "com.android.ide.eclipse.hierarchyviewer.views.PixelPerfectView"; //$NON-NLS-1$

    private PixelPerfect mPixelPerfect;

    @Override
    public void createPartControl(Composite parent) {
        parent.setLayout(new FillLayout());
        mPixelPerfect = new PixelPerfect(parent);

        placeActions();
    }

    private void placeActions() {
        IActionBars actionBars = getViewSite().getActionBars();

        IMenuManager mm = actionBars.getMenuManager();
        mm.removeAll();
        mm.add(SavePixelPerfectAction.getAction(getSite().getShell()));
        mm.add(RefreshPixelPerfectAction.getAction());
        mm.add(LoadOverlayAction.getAction(getSite().getShell()));

        IToolBarManager tm = actionBars.getToolBarManager();
        tm.removeAll();
        tm.add(SavePixelPerfectAction.getAction(getSite().getShell()));
        tm.add(RefreshPixelPerfectAction.getAction());
        tm.add(LoadOverlayAction.getAction(getSite().getShell()));
    }

    @Override
    public void setFocus() {
        mPixelPerfect.setFocus();
    }

}
