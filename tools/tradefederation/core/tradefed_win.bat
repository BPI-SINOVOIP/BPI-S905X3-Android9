@echo off

:: Copyright (C) 2014 The Android Open Source Project

:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at

::     http://www.apache.org/licenses/LICENSE-2.0

:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

:: A helper script that launches TradeFederation from the current build
:: environment.

setlocal EnableDelayedExpansion
call:checkCommand adb
call:checkCommand java

:: check java version

%JAVA% -version 2>&1 | findstr /R "version\ \"1*\.*[89].*\"$" || (
    echo "Wrong java version. 1.8 or 9 is required."
    exit /B
)

:: check debug flag and set up remote debugging
if not "%TF_DEBUG%"=="" (
    if "%TF_DEBUG_PORT%" == "" (
        set TF_DEBUG_PORT=10088
    )
    set RDBG_FLAG=-agentlib:jdwp=transport=dt_socket,server=y,suspend=y,address=!TF_DEBUG_PORT!
)

:: first try to find TF jars in same dir as this script
set CUR_DIR=%CD%

if exist "%CUR_DIR%\tradefed.jar" (
    set tf_path="%CUR_DIR%\*"
) else (
    if not "%ANDROID_HOST_OUT%" == "" (
        if exist "%ANDROID_HOST_OUT%\tradefed\tradefed.jar" (
            set tf_path="%ANDROID_HOST_OUT%\tradefed\*"
        )
    )
)

if "%tf_path%" == "" (
    echo "ERROR: Could not find tradefed jar files"
    exit /B
)

:: set any host specific options
:: file format for file at $TRADEFED_OPTS_FILE is one line per host with the following format:
:: <hostname>=<options>
:: for example:
:: hostname.domain.com=-Djava.io.tmpdir=/location/on/disk -Danother=false ...
:: hostname2.domain.com=-Djava.io.tmpdir=/different/location -Danother=true ...
if exist "%TRADEFED_OPTS_FILE%" (
    call:commandResult "hostname" HOST_NAME
    call:commandResult "findstr /i /b "%HOST_NAME%" "%TRADEFED_OPTS_FILE%"" TRADEFED_OPTS
:: delete the hostname part
    set TRADEFED_OPTS=!TRADEFED_OPTS:%HOST_NAME%=!
:: delete the first =
    set TRADEFED_OPTS=!TRADEFED_OPTS:~1!
)

java %RDBG_FLAG% -XX:+HeapDumpOnOutOfMemoryError ^
-XX:-OmitStackTraceInFastThrow %TRADEFED_OPTS% -cp %tf_path% com.android.tradefed.command.Console %*

endlocal
::end of file
goto:eof

:: check command exist or not
:: if command not exist, exit
:checkCommand
for /f "delims=" %%i in ('where %~1') do (
    if %%i == "" (
        echo %~1 not exist
        exit /B
    )
    goto:eof
)
goto:eof

:: get the command result
:: usage: call:commandResult "command" result
:commandResult
for /f "delims=" %%i in ('%~1') do (
    set %~2=%%i
    goto:eof
)
goto:eof
