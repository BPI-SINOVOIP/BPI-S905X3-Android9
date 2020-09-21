// Copyright 2017 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "launcher_internal.h"

#include <Python.h>
#include <android-base/file.h>
#include <osdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

int main(int argc, char *argv[]) {
  int result = 0 /* Used to mark if current program runs with success/failure. */;

  // Clear PYTHONPATH and PYTHONHOME so Python doesn't attempt to check the local
  // disk for Python modules to load. The value of PYTHONHOME will replace "prefix"
  // and "exe_prefix" based on the description in getpath.c.
  // Please don't use PYTHONPATH and PYTHONHOME within user program.
  // TODO(nanzhang): figure out if unsetenv("PYTHONPATH") is better.
  unsetenv(const_cast<char *>("PYTHONPATH"));
  // TODO(nanzhang): figure out if Py_SetPythonHome() is better.
  unsetenv(const_cast<char *>("PYTHONHOME"));
  // PYTHONEXECUTABLE is only used on MacOs X, when the Python interpreter
  // embedded in an application bundle. It is not sure that we have this use case
  // for Android hermetic Python. So override this environment variable to empty
  // for now to make our self-contained environment more strict.
  // For user (.py) program, it can access hermetic .par file path through
  // sys.argv[0].
  unsetenv(const_cast<char *>("PYTHONEXECUTABLE"));

  // Resolving absolute path based on argv[0] is not reliable since it may
  // include something unusable, too bad.
  // android::base::GetExecutablePath() also handles for Darwin/Windows.
  std::string executable_path = android::base::GetExecutablePath();

  argv[0] = strdup(executable_path.c_str());
  // argv[0] is used for setting internal path, and Python sys.argv[0]. It
  // should not exceed MAXPATHLEN defined for CPython.
  if (!argv[0] || strlen(argv[0]) > MAXPATHLEN) {
    fprintf(stderr, "The executable path %s is NULL or of invalid length.\n", argv[0]);
    return 1;
  }

  // For debugging/logging purpose, set stdin/stdout/stderr unbuffered through
  // environment variable.
  // TODO(nanzhang): Set Py_VerboseFlag if more debugging requests needed.
  const char *unbuffered_env = getenv("PYTHONUNBUFFERED");
  if (unbuffered_env && unbuffered_env[0]) {
    #if defined(MS_WINDOWS) || defined(__CYGWIN__)
      _setmode(fileno(stdin), O_BINARY);
      _setmode(fileno(stdout), O_BINARY);
    #endif
    #ifdef HAVE_SETVBUF
      setvbuf(stdin,  (char *)NULL, _IONBF, BUFSIZ);
      setvbuf(stdout, (char *)NULL, _IONBF, BUFSIZ);
      setvbuf(stderr, (char *)NULL, _IONBF, BUFSIZ);
    #else /* !HAVE_SETVBUF */
      setbuf(stdin,  (char *)NULL);
      setbuf(stdout, (char *)NULL);
      setbuf(stderr, (char *)NULL);
    #endif /* !HAVE_SETVBUF */
  }
  //For debugging/logging purpose, Warning control.
  //Python’s warning machinery by default prints warning messages to sys.stderr.
  //The full form of argument is:action:message:category:module:line
  char *warnings_env = getenv("PYTHONWARNINGS");
  if (warnings_env && warnings_env[0]) {
      char *warnings_buf, *warning;

      // Note: "new" operation; we need free this chuck of data after use.
      warnings_buf = new char[strlen(warnings_env) + 1];
      if (warnings_buf == NULL)
          Py_FatalError(
             "not enough memory to copy PYTHONWARNINGS");
      strcpy(warnings_buf, warnings_env);
      for (warning = strtok(warnings_buf, ",");
           warning != NULL;
           warning = strtok(NULL, ","))
          PySys_AddWarnOption(warning);
      delete[] warnings_buf;
  }

  // Always enable Python "-s" option. We don't need user-site directories,
  // everything's supposed to be hermetic.
  Py_NoUserSiteDirectory = 1;

  Py_SetProgramName(argv[0]);
  Py_Initialize();
  PySys_SetArgvEx(argc, argv, 0);

  // Set sys.executable to None. The real executable is available as
  // sys.argv[0], and too many things assume sys.executable is a regular Python
  // binary, which isn't available. By setting it to None we get clear errors
  // when people try to use it.
  if (PySys_SetObject(const_cast<char *>("executable"), Py_None) < 0) {
    PyErr_Print();
    result = 1;
    goto error;
  }

  result = android::cpython2::python_launcher::RunEntryPointOrMainModule(argv[0]);
  if (result < 0) {
    PyErr_Print();
    goto error;
  }

error:
  Py_Finalize();

  free(argv[0]);
  exit(abs(result));
}
