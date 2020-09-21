/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.ide.eclipse.adt.internal.wizards.export;

import com.android.ide.common.xml.ManifestData;
import com.android.ide.eclipse.adt.AdtConstants;
import com.android.ide.eclipse.adt.internal.editors.IconFactory;
import com.android.ide.eclipse.adt.internal.project.AndroidManifestHelper;
import com.android.ide.eclipse.adt.internal.project.BaseProjectHelper;
import com.android.ide.eclipse.adt.internal.project.ProjectChooserHelper;
import com.android.ide.eclipse.adt.internal.project.ProjectChooserHelper.NonLibraryProjectOnlyFilter;
import com.android.ide.eclipse.adt.internal.project.ProjectHelper;
import com.android.ide.eclipse.adt.internal.wizards.export.ExportWizard.ExportWizardPage;

import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.jdt.core.IJavaProject;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;

/**
 * First Export Wizard Page. Display warning/errors.
 */
final class ProjectCheckPage extends ExportWizardPage {
    private final static String IMG_ERROR = "error.png"; //$NON-NLS-1$
    private final static String IMG_WARNING = "warning.png"; //$NON-NLS-1$

    private final ExportWizard mWizard;
    private Image mError;
    private Image mWarning;
    private boolean mHasMessage = false;
    private Composite mTopComposite;
    private Composite mErrorComposite;
    private Text mProjectText;
    private ProjectChooserHelper mProjectChooserHelper;
    private boolean mFirstOnShow = true;

    protected ProjectCheckPage(ExportWizard wizard, String pageName) {
        super(pageName);
        mWizard = wizard;

        setTitle("Project Checks");
        setDescription("Performs a set of checks to make sure the application can be exported.");
    }

    @Override
    public void createControl(Composite parent) {
        mProjectChooserHelper = new ProjectChooserHelper(parent.getShell(),
                new NonLibraryProjectOnlyFilter());

        GridLayout gl = null;
        GridData gd = null;

        mTopComposite = new Composite(parent, SWT.NONE);
        mTopComposite.setLayoutData(new GridData(GridData.FILL_BOTH));
        mTopComposite.setLayout(new GridLayout(1, false));

        // composite for the project selection.
        Composite projectComposite = new Composite(mTopComposite, SWT.NONE);
        projectComposite.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        projectComposite.setLayout(gl = new GridLayout(3, false));
        gl.marginHeight = gl.marginWidth = 0;

        Label label = new Label(projectComposite, SWT.NONE);
        label.setLayoutData(gd = new GridData(GridData.FILL_HORIZONTAL));
        gd.horizontalSpan = 3;
        label.setText("Select the project to export:");

        new Label(projectComposite, SWT.NONE).setText("Project:");
        mProjectText = new Text(projectComposite, SWT.BORDER);
        mProjectText.setLayoutData(gd = new GridData(GridData.FILL_HORIZONTAL));
        mProjectText.addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e) {
                handleProjectNameChange();
            }
        });

        Button browseButton = new Button(projectComposite, SWT.PUSH);
        browseButton.setText("Browse...");
        browseButton.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e) {
                IJavaProject javaProject = mProjectChooserHelper.chooseJavaProject(
                        mProjectText.getText().trim(),
                        "Please select a project to export");

                if (javaProject != null) {
                    IProject project = javaProject.getProject();

                    // set the new name in the text field. The modify listener will take
                    // care of updating the status and the ExportWizard object.
                    mProjectText.setText(project.getName());
                }
            }
        });

        setControl(mTopComposite);
    }

    @Override
    void onShow() {
        if (mFirstOnShow) {
            // get the project and init the ui
            IProject project = mWizard.getProject();
            if (project != null) {
                mProjectText.setText(project.getName());
            }

            mFirstOnShow = false;
        }
    }

    private void buildErrorUi(IProject project) {
        // Show description the first time
        setErrorMessage(null);
        setMessage(null);
        setPageComplete(true);
        mHasMessage = false;

        // composite parent for the warning/error
        GridLayout gl = null;
        mErrorComposite = new Composite(mTopComposite, SWT.NONE);
        mErrorComposite.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        gl = new GridLayout(2, false);
        gl.marginHeight = gl.marginWidth = 0;
        gl.verticalSpacing *= 3; // more spacing than normal.
        mErrorComposite.setLayout(gl);

        if (project == null) {
            setErrorMessage("Select project to export.");
            mHasMessage = true;
        } else {
            try {
                if (project.hasNature(AdtConstants.NATURE_DEFAULT) == false) {
                    addError(mErrorComposite, "Project is not an Android project.");
                } else {
                    // check for errors
                    if (ProjectHelper.hasError(project, true))  {
                        addError(mErrorComposite, "Project has compilation error(s)");
                    }

                    // check the project output
                    IFolder outputIFolder = BaseProjectHelper.getJavaOutputFolder(project);
                    if (outputIFolder == null) {
                        addError(mErrorComposite,
                                "Unable to get the output folder of the project!");
                    }

                    // project is an android project, we check the debuggable attribute.
                    ManifestData manifestData = AndroidManifestHelper.parseForData(project);
                    Boolean debuggable = null;
                    if (manifestData != null) {
                        debuggable = manifestData.getDebuggable();
                    }

                    if (debuggable != null && debuggable == Boolean.TRUE) {
                        addWarning(mErrorComposite,
                                "The manifest 'debuggable' attribute is set to true.\n" +
                                "You should set it to false for applications that you release to the public.\n\n" +
                                "Applications with debuggable=true are compiled in debug mode always.");
                    }

                    // check for mapview stuff
                }
            } catch (CoreException e) {
                // unable to access nature
                addError(mErrorComposite, "Unable to get project nature");
            }
        }

        if (mHasMessage == false) {
            Label label = new Label(mErrorComposite, SWT.NONE);
            GridData gd = new GridData(GridData.FILL_HORIZONTAL);
            gd.horizontalSpan = 2;
            label.setLayoutData(gd);
            label.setText("No errors found. Click Next.");
        }

        mTopComposite.layout();
    }

    /**
     * Adds an error label to a {@link Composite} object.
     * @param parent the Composite parent.
     * @param message the error message.
     */
    private void addError(Composite parent, String message) {
        if (mError == null) {
            mError = IconFactory.getInstance().getIcon(IMG_ERROR);
        }

        new Label(parent, SWT.NONE).setImage(mError);
        Label label = new Label(parent, SWT.NONE);
        label.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        label.setText(message);

        setErrorMessage("Application cannot be exported due to the error(s) below.");
        setPageComplete(false);
        mHasMessage = true;
    }

    /**
     * Adds a warning label to a {@link Composite} object.
     * @param parent the Composite parent.
     * @param message the warning message.
     */
    private void addWarning(Composite parent, String message) {
        if (mWarning == null) {
            mWarning = IconFactory.getInstance().getIcon(IMG_WARNING);
        }

        new Label(parent, SWT.NONE).setImage(mWarning);
        Label label = new Label(parent, SWT.NONE);
        label.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        label.setText(message);

        mHasMessage = true;
    }

    /**
     * Checks the parameters for correctness, and update the error message and buttons.
     */
    private void handleProjectNameChange() {
        setPageComplete(false);

        if (mErrorComposite != null) {
            mErrorComposite.dispose();
            mErrorComposite = null;
        }

        // update the wizard with the new project
        mWizard.setProject(null);

        //test the project name first!
        String text = mProjectText.getText().trim();
        if (text.length() == 0) {
            setErrorMessage("Select project to export.");
        } else if (text.matches("[a-zA-Z0-9_ \\.-]+") == false) {
            setErrorMessage("Project name contains unsupported characters!");
        } else {
            IJavaProject[] projects = mProjectChooserHelper.getAndroidProjects(null);
            IProject found = null;
            for (IJavaProject javaProject : projects) {
                if (javaProject.getProject().getName().equals(text)) {
                    found = javaProject.getProject();
                    break;
                }

            }

            if (found != null) {
                setErrorMessage(null);

                // update the wizard with the new project
                mWizard.setProject(found);

                // now rebuild the error ui.
                buildErrorUi(found);
            } else {
                setErrorMessage(String.format("There is no android project named '%1$s'",
                        text));
            }
        }
    }
}
