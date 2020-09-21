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
package android.fragment.cts;

import android.app.LoaderManager;
import android.content.AsyncTaskLoader;
import android.content.Context;
import android.content.Loader;
import android.os.Bundle;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.concurrent.CountDownLatch;

/**
 * This Activity sets the text when loading completes. It also tracks the Activity in
 * a static variable, so it must be cleared in test tear down.
 */
public class LoaderActivity extends RecreatedActivity {
    public TextView textView;
    public TextView textViewB;
    public CountDownLatch loadFinished = new CountDownLatch(1);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.text_a);
        textView = (TextView) findViewById(R.id.textA);

        ViewGroup container = (ViewGroup) textView.getParent();
        textViewB = new TextView(this);
        textViewB.setId(R.id.textB);
        container.addView(textViewB);
    }

    @Override
    protected void onResume() {
        super.onResume();
        getLoaderManager().initLoader(0, null, new TextLoaderCallback());
    }

    class TextLoaderCallback implements LoaderManager.LoaderCallbacks<String> {
        @Override
        public Loader<String> onCreateLoader(int id, Bundle args) {
            return new TextLoader(LoaderActivity.this);
        }

        @Override
        public void onLoadFinished(Loader<String> loader, String data) {
            textView.setText(data);
            loadFinished.countDown();
        }

        @Override
        public void onLoaderReset(Loader<String> loader) {
        }
    }

    static class TextLoader extends AsyncTaskLoader<String> {
        TextLoader(Context context) {
            super(context);
        }

        @Override
        protected void onStartLoading() {
            forceLoad();
        }

        @Override
        public String loadInBackground() {
            return "Loaded!";
        }
    }
}

