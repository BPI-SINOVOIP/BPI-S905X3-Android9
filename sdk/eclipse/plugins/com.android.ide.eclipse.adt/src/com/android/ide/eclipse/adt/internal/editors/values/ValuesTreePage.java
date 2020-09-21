/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.ide.eclipse.adt.internal.editors.values;

import com.android.ide.common.resources.ResourceFolder;
import com.android.ide.eclipse.adt.AdtPlugin;
import com.android.ide.eclipse.adt.internal.editors.IPageImageProvider;
import com.android.ide.eclipse.adt.internal.editors.IconFactory;
import com.android.ide.eclipse.adt.internal.editors.common.CommonXmlEditor;
import com.android.ide.eclipse.adt.internal.editors.layout.configuration.FlagManager;
import com.android.ide.eclipse.adt.internal.editors.ui.tree.UiTreeBlock;
import com.android.ide.eclipse.adt.internal.editors.uimodel.UiElementNode;
import com.android.ide.eclipse.adt.internal.resources.manager.ResourceManager;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IFile;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.forms.IManagedForm;
import org.eclipse.ui.forms.editor.FormPage;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.part.FileEditorInput;

/**
 * Page for instrumentation settings, part of the AndroidManifest form editor.
 */
public final class ValuesTreePage extends FormPage implements IPageImageProvider {
    /** Page id used for switching tabs programmatically */
    public final static String PAGE_ID = "res_tree_page"; //$NON-NLS-1$

    /** Container editor */
    CommonXmlEditor mEditor;

    public ValuesTreePage(CommonXmlEditor editor) {
        super(editor, PAGE_ID, "Resources");  // tab's label, keep it short
        mEditor = editor;
    }

    @Override
    public Image getPageImage() {
        // See if we should use a flag icon if this is a language-specific configuration
        IFile file = mEditor.getInputFile();
        if (file != null) {
            IContainer parent = file.getParent();
            if (parent != null) {
                Image flag = FlagManager.get().getFlagForFolderName(parent.getName());
                if (flag != null) {
                    return flag;
                }
            }
        }

        return IconFactory.getInstance().getIcon("editor_page_design");  //$NON-NLS-1$
    }

    /**
     * Creates the content in the form hosted in this page.
     *
     * @param managedForm the form hosted in this page.
     */
    @Override
    protected void createFormContent(IManagedForm managedForm) {
        super.createFormContent(managedForm);
        ScrolledForm form = managedForm.getForm();

        String configText = null;
        IEditorInput input = mEditor.getEditorInput();
        if (input instanceof FileEditorInput) {
            FileEditorInput fileInput = (FileEditorInput)input;
            IFile iFile = fileInput.getFile();

            ResourceFolder resFolder = ResourceManager.getInstance().getResourceFolder(iFile);
            if (resFolder != null) {
                configText = resFolder.getConfiguration().toDisplayString();
            }
        }

        if (configText != null) {
            form.setText(String.format("Android Resources (%1$s)", configText));
        } else {
            form.setText("Android Resources");
        }

        form.setImage(AdtPlugin.getAndroidLogo());

        UiElementNode resources = mEditor.getUiRootNode();
        UiTreeBlock block = new UiTreeBlock(mEditor, resources,
                true /* autoCreateRoot */,
                null /* no element filters */,
                "Resources Elements",
                "List of all resources elements in this XML file.");
        block.createContent(managedForm);
    }
}
