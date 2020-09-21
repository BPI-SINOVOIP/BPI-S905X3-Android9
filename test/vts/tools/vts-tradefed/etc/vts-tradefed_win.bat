:: Copyright (C) 2017 The Android Open Source Project
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::       http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

:: Launcher script for vts-tradefed harness
:: Can be used from an Android build environment, or a standalone VTS zip
:: Caution: Although this script can be used to run VTS on Windows,
:: Windows host is not yet officially supported.

@echo off
setlocal ENABLEDELAYEDEXPANSION

set ADB=adb.exe
set JAVA=java.exe
where %ADB% || (echo Unable to find %ADB% && goto:eof)
where %JAVA% || (echo Unable to find %JAVA% && goto:eof)

:: check java version
%JAVA% -version 2>&1 | findstr /R "version\ \"1*\.*[89].*\"$" || (
    echo Wrong java version. 1.8 or 9 is required.
    goto:eof
)

:: check debug flag and set up remote debugging
if not [%TF_DEBUG%] == [] (
    if [%TF_DEBUG_PORT%] == [] (
        set TF_DEBUG_PORT=10088
    )
    set RDBG_FLAG=-agentlib:jdwp=transport=dt_socket,server=y,suspend=y,address=!TF_DEBUG_PORT!
)

:: assume built on Linux; running on Windows.
:: find VTS_ROOT directory by location of this script
echo %~dp0 | findstr /R \\out\\host\\windows-x86\\bin && (
    set VTS_ROOT=%~dp0\..\..\linux-x86\vts
)

if [%VTS_ROOT%] == [] (
    :: assume in an extracted VTS installation package
    set VTS_ROOT=%~dp0\..\..
)
echo VTS_ROOT=%VTS_ROOT%

:: java classpath
set JAR_DIR=%VTS_ROOT%\android-vts\tools

:: tradefed.jar
set TRADEFED_JAR=%JAR_DIR%\tradefed.jar
if not exist "%TRADEFED_JAR%" (
    echo Unable to locate %TRADEFED_JAR%. Try prebuilt jar.
    set TRADEFED_JAR=%JAR_DIR%\tradefed-prebuilt.jar
)
if not exist "%TRADEFED_JAR%" (
    echo Unable to locate %TRADEFED_JAR%
    goto:eof
)
set JAR_PATH=%TRADEFED_JAR%

:: other required jars
set JARS=^
  loganalysis^
  hosttestlib^
  vts-tradefed^
  vts-tradefed-tests^
  compatibility-host-util
for %%J in (%JARS%) do (
    set JAR=%JAR_DIR%\%%J.jar
    if not exist "!JAR!" ( echo Unable to locate !JAR! && goto:eof )
    set JAR_PATH=!JAR_PATH!;!JAR!
)

:: to run in the lab.
set OPTIONAL_JARS=^
  android-vts\tools\google-tradefed-vts-prebuilt^
  google-tradefed-prebuilt^
  google-tradefed-tests^
  google-tf-prod-tests

for %%J in (%OPTIONAL_JARS%) do (
    set JAR=%VTS_ROOT%\%%J.jar
    if exist "!JAR!" (
        echo Including optional JAR: !JAR!
        set JAR_PATH=!JAR_PATH!;!JAR!
    ) else (
        echo Optional JAR not found: !JAR!
    )
)

:: skip loading shared libraries for host-side executables

:: include any host-side test jars
set JAR_PATH=%JAR_PATH%;%VTS_ROOT%\android-vts\testcases\*
echo JAR_PATH=%JAR_PATH%

cd %VTS_ROOT%/android-vts/testcases
%JAVA% %RDBG_FLAG% -cp "%JAR_PATH%" "-DVTS_ROOT=%VTS_ROOT%" com.android.compatibility.common.tradefed.command.CompatibilityConsole %*

