@ECHO OFF

REM Start-up script for the GUI of ProGuard -- free class file shrinker,
REM optimizer, obfuscator, and preverifier for Java bytecode.

rem Change current directory and drive to where the script is, to avoid
rem issues with directories containing whitespaces.
cd /d %~dp0

IF EXIST "%PROGUARD_HOME%" GOTO home
SET PROGUARD_HOME=..
:home

set java_exe=
call %PROGUARD_HOME%\..\lib\find_java.bat

call %java_exe% -jar "%PROGUARD_HOME%"\lib\proguardgui.jar %1 %2 %3 %4 %5 %6 %7 %8 %9
