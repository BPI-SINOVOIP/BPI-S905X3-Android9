@echo off
if not defined HOST_PYTHON (
  if "%1" EQU "Debug" (
    shift
    set HOST_PYTHON=python_d.exe
    if not exist python36_d.dll exit 1
  ) ELSE (
    set HOST_PYTHON=python.exe
    if not exist python36.dll exit 1
  )
)
%HOST_PYTHON% "%~dp0prepare_ssl.py" %1
