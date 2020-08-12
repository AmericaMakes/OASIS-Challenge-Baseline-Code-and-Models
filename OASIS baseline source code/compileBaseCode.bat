REM This batch file compiles the OASIS baseline scanpath code
REM using Microsoft MSbuild tools, assuming they are installed in default location
REM
REM Change line 22, which calls vcvarsall.bat, to the location of this file on your machine.
REM The x64 switch as well as Release/x64 switches on msbuild are required because this
REM Visual Studio solution is only set up with correct paths under that configuration
REM
REM USAGE:
REM 	Send in the code directory as a parameter in double-quotes when executing this file from command line
REM For example, on the author's machine this would be
REM 	compileBaseCode.bat "C:\Users\200008936\Documents\GitHub\OASIS-Challenge-Baseline-Code-and-Models\OASIS baseline source code\OASIS baseline source"
REM
REM The solution file should appear in that folder.  The solution filename, configuration
REM and platform are hard coded in the msbuild command
 
@ECHO OFF
CLS
TITLE OASIS Challenge Baseline File Compilation
ECHO ***OASIS Challenge Baseline File Compilation***

REM Start msbuild tools by assuming default location and x64 configuration
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\"vcvarsall.bat x64

REM Change to solution folder input by user
REM use %~f1 to extract the full path from parameter #1, or %1 to use it as-is
CD %1

REM Compile the solution
msbuild OASIS_baseline.sln /property:Configuration=Release /property:Platform=x64
