@echo off
setlocal enabledelayedexpansion

set PROJECT=atmega128-test
set PROGRAMMER=stk500v2
set PORT=COM5

REM Build the project
echo Building the project...
cd build
make

REM Upload the project
avrdude -c %PROGRAMMER% -P %PORT% -p m128 -U flash:w:%PROJECT%.hex

endlocal
