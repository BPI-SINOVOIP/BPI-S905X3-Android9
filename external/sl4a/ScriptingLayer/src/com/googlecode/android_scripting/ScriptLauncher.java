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

package com.googlecode.android_scripting;

import android.content.Intent;

import com.googlecode.android_scripting.interpreter.Interpreter;
import com.googlecode.android_scripting.interpreter.InterpreterConfiguration;
import com.googlecode.android_scripting.interpreter.InterpreterProcess;

import java.io.File;

public class ScriptLauncher {

  private ScriptLauncher() {
    // Utility class.
  }

  public static InterpreterProcess launchInterpreter(final AndroidProxy proxy, Intent intent,
      InterpreterConfiguration config, Runnable shutdownHook) {
    Interpreter interpreter;
    String interpreterName;
    interpreterName = intent.getStringExtra(Constants.EXTRA_INTERPRETER_NAME);
    interpreter = config.getInterpreterByName(interpreterName);
    InterpreterProcess process = new InterpreterProcess(interpreter, proxy);
    if (shutdownHook == null) {
      process.start(new Runnable() {
        @Override
        public void run() {
          proxy.shutdown();
        }
      });
    } else {
      process.start(shutdownHook);
    }
    return process;
  }

  public static ScriptProcess launchScript(File script, InterpreterConfiguration configuration,
      final AndroidProxy proxy, Runnable shutdownHook) {
    if (!script.exists()) {
      throw new RuntimeException("No such script to launch.");
    }
    ScriptProcess process = new ScriptProcess(script, configuration, proxy);
    if (shutdownHook == null) {
      process.start(new Runnable() {
        @Override
        public void run() {
          proxy.shutdown();
        }
      });
    } else {
      process.start(shutdownHook);
    }
    return process;
  }
}
