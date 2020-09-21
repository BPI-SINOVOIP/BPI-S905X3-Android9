/*
 * ConnectBot: simple, powerful, open-source SSH client for Android
 * Copyright 2007 Kenny Root, Jeffrey Sharkey
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.connectbot;

import android.app.Activity;
import android.content.Intent;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.R;
import com.googlecode.android_scripting.Version;

import java.io.IOException;

/**
 *
 */
public class HelpActivity extends Activity {
  public final static String TAG = "ConnectBot.HelpActivity";

  public final static String HELPDIR = "help";
  public final static String SUFFIX = ".html";

  @Override
  public void onCreate(Bundle icicle) {
    super.onCreate(icicle);
    setContentView(R.layout.act_help);

    this.setTitle(String.format("%s: %s", getResources().getText(R.string.application_nice_title),
        getResources().getText(R.string.title_help)));

    TextView subtitle = (TextView) findViewById(R.id.version);
    subtitle.setText(getText(R.string.application_title) + " r"
        + Version.getVersion(getApplication()));

    ((TextView) findViewById(R.id.help_acks_text)).setMovementMethod(LinkMovementMethod
        .getInstance());

    AssetManager am = getAssets();
    LinearLayout content = (LinearLayout) findViewById(R.id.topics);

    try {
      for (String name : am.list(HELPDIR)) {
        if (name.endsWith(SUFFIX)) {
          Button button = new Button(this);
          final String topic = name.substring(0, name.length() - SUFFIX.length());
          button.setText(topic);

          button.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
              Intent intent = new Intent(HelpActivity.this, HelpTopicActivity.class);
              intent.putExtra(Intent.EXTRA_TITLE, topic);
              HelpActivity.this.startActivity(intent);
            }
          });

          content.addView(button);
        }
      }
    } catch (IOException e) {
      // TODO Auto-generated catch block
      Log.e("couldn't get list of help assets", e);
    }
  }
}
