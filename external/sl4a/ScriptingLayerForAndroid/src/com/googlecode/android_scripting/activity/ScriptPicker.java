/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.googlecode.android_scripting.activity;

import android.app.ListActivity;
import android.content.Context;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;

import com.googlecode.android_scripting.BaseApplication;
import com.googlecode.android_scripting.R;
import com.googlecode.android_scripting.ScriptListAdapter;
import com.googlecode.android_scripting.ScriptStorageAdapter;
import com.googlecode.android_scripting.interpreter.InterpreterConfiguration;
import com.googlecode.android_scripting.interpreter.InterpreterConstants;

import java.io.File;
import java.util.List;


/**
 * Presents available scripts and returns the selected one.
 *
 */
public class ScriptPicker extends ListActivity {

  private List<File> mScripts;
  private ScriptPickerAdapter mAdapter;
  private InterpreterConfiguration mConfiguration;
  private File mCurrentDir;
  private final File mBaseDir = new File(InterpreterConstants.SCRIPTS_ROOT);

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    CustomizeWindow.requestCustomTitle(this, "Scripts", R.layout.script_manager);
    mCurrentDir = mBaseDir;
    mConfiguration = ((BaseApplication) getApplication()).getInterpreterConfiguration();
    mScripts = ScriptStorageAdapter.listExecutableScripts(null, mConfiguration);
    mAdapter = new ScriptPickerAdapter(this);
    mAdapter.registerDataSetObserver(new ScriptListObserver());
    setListAdapter(mAdapter);
  }

  @Override
  protected void onListItemClick(ListView list, View view, int position, long id) {
    final File script = (File) list.getItemAtPosition(position);

    if (script.isDirectory()) {
      mCurrentDir = script;
      mAdapter.notifyDataSetInvalidated();
      return;
    }

    //TODO: Take action here based on item click
  }

  private class ScriptListObserver extends DataSetObserver {
    @SuppressWarnings("serial")
    @Override
    public void onInvalidated() {
      mScripts = ScriptStorageAdapter.listExecutableScripts(mCurrentDir, mConfiguration);
      // TODO(damonkohler): Extending the File class here seems odd.
      if (!mCurrentDir.equals(mBaseDir)) {
        mScripts.add(0, new File(mCurrentDir.getParent()) {
          @Override
          public boolean isDirectory() {
            return true;
          }

          @Override
          public String getName() {
            return "..";
          }
        });
      }
    }
  }

  private class ScriptPickerAdapter extends ScriptListAdapter {

    public ScriptPickerAdapter(Context context) {
      super(context);
    }

    @Override
    protected List<File> getScriptList() {
      return mScripts;
    }

  }
}
