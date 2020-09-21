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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
// Cpython built-in C functions.
/*
   read_directory(archive) -> files dict (new reference)

   Given a path to a Zip archive, build a dict, mapping file names
   (local to the archive, using SEP as a separator) to toc entries.
*/
PyObject *read_directory(const char *archive);

/* Given a path to a Zip file and a toc_entry, return the (uncompressed)
   data as a new reference. */
PyObject *get_data(const char *archive, PyObject *toc_entry);
}

namespace android {
namespace cpython2 {
namespace python_launcher {
namespace internal {

int RunModule(const char *module, int set_argv0) {
  PyObject *runpy, *runmodule, *runargs, *result;
  runpy = PyImport_ImportModule("runpy");
  if (runpy == NULL) {
    fprintf(stderr, "Could not import runpy module\n");
    return -1;
  }
  runmodule = PyObject_GetAttrString(runpy, "_run_module_as_main");
  if (runmodule == NULL) {
    fprintf(stderr, "Could not access runpy._run_module_as_main\n");
    Py_DECREF(runpy);
    return -1;
  }
  runargs = Py_BuildValue("(si)", module, set_argv0);
  if (runargs == NULL) {
    fprintf(stderr,
            "Could not create arguments for runpy._run_module_as_main\n");
    Py_DECREF(runpy);
    Py_DECREF(runmodule);
    return -1;
  }
  result = PyObject_Call(runmodule, runargs, NULL);
  if (result == NULL) {
    PyErr_Print();
  }
  Py_DECREF(runpy);
  Py_DECREF(runmodule);
  Py_DECREF(runargs);
  if (result == NULL) {
    return -1;
  }
  Py_DECREF(result);
  return 0;
}

std::string GetEntryPointFilePath(const char *launcher_path) {
  PyObject *files;
  files = read_directory(launcher_path);
  if (files == NULL) {
    return std::string();
  }
  PyObject *toc_entry;
  // Return value: Borrowed reference.
  toc_entry = PyDict_GetItemString(files, ENTRYPOINT_FILE);
  if (toc_entry == NULL) {
    Py_DECREF(files);
    return std::string();
  }
  PyObject *py_data;
  py_data = get_data(launcher_path, toc_entry);
  if (py_data == NULL) {
    Py_DECREF(files);
    return std::string();
  }
  // PyString_AsString returns a NUL-terminated representation of the "py_data",
  // "data" must not be modified in any way. And it must not be deallocated.
  char *data = PyString_AsString(py_data);
  if (data == NULL) {
    Py_DECREF(py_data);
    Py_DECREF(files);
    return std::string();
  }

  char *res = strdup(data); /* deep copy of data */
  Py_DECREF(py_data);
  Py_DECREF(files);

  int i = 0;
  /* Strip newline and other trailing whitespace. */
  for (i = strlen(res) - 1; i >= 0 && isspace(res[i]); i--) {
    res[i] = '\0';
  }
  /* Check for the file extension. */
  i = strlen(res);
  if (i > 3 && strcmp(res + i - 3, ".py") == 0) {
    res[i - 3] = '\0';
  } else {
    PyErr_Format(PyExc_ValueError, "Invalid entrypoint in %s: %s",
                 ENTRYPOINT_FILE, res);
    return std::string();
  }
  return std::string(res);
}

int RunModuleNameFromEntryPoint(const char *launcher_path, std::string entrypoint) {
  if (entrypoint.empty()) {
    return -1;
  }
  // Has to pass to free to avoid a memory leak after use.
  char *arr = strdup(entrypoint.c_str());
  // Replace file system path seperator with Python package/module seperator.
  char *ch;
  for (ch = arr; *ch; ch++) {
    if (*ch == '/') {
      *ch = '.';
    }
  }

  if (AddPathToPythonSysPath(launcher_path) < 0) {
    free(arr);
    return -1;
  }
  // Calculate the runfiles path size. Extra space for '\0'.
  size_t size = snprintf(nullptr, 0, "%s/%s", launcher_path, RUNFILES) + 1;
  char runfiles_path[size];
  snprintf(runfiles_path, size, "%s/%s", launcher_path, RUNFILES);
  if (AddPathToPythonSysPath(runfiles_path) < 0) {
    free(arr);
    return -1;
  }
  int ret =  RunModule(arr, 0);
  free(arr);
  return ret;
}

int AddPathToPythonSysPath(const char *path) {
  if (path == NULL) {
    return -1;
  }
  PyObject *py_path;
  py_path = PyString_FromString(path);
  if (py_path == NULL) {
    return -1;
  }
  PyObject *sys_path;
  // Return value: Borrowed reference.
  sys_path = PySys_GetObject(const_cast<char*>("path"));
  if (sys_path == NULL) {
    Py_DECREF(py_path);
    return -1;
  }
  PyList_Insert(sys_path, 0, py_path);
  Py_DECREF(py_path);
  return 0;
}

int RunMainFromImporter(const char *launcher_path) {
  PyObject *py_launcher_path, *importer;
  py_launcher_path = PyString_FromString(launcher_path);
  if (py_launcher_path == NULL) {
    return -1;
  }
  importer = PyImport_GetImporter(py_launcher_path);
  if (importer == NULL) {
    Py_DECREF(py_launcher_path);
    return -1;
  }
  if (importer != Py_None && importer->ob_type != &PyNullImporter_Type) {
    /* Launcher path is usable as an import source, so
       put it in sys.path[0] and import __main__ */
    if (AddPathToPythonSysPath(launcher_path) < 0) {
      Py_DECREF(importer);
      Py_DECREF(py_launcher_path);
      return -1;
    }
  }
  Py_DECREF(importer);
  Py_DECREF(py_launcher_path);
  return RunModule("__main__", 0);
}
}  // namespace internal

int RunEntryPointOrMainModule(const char *launcher_path) {
  std::string entrypoint = internal::GetEntryPointFilePath(launcher_path);
  if (entrypoint.empty()) {
    // If entry point can not be found or can not be executed, we try to
    // run __main__.py within the .par file.
    fprintf(stderr, "Cannot find valid entry point to execute par file!\n");
    fprintf(stdout, "Start trying to run __main__ module within par file.\n");
    return internal::RunMainFromImporter(launcher_path);
  }
  return internal::RunModuleNameFromEntryPoint(launcher_path, entrypoint);
}
}  // namespace python_launcher
}  // namespace cpython2
}  // namespace android
