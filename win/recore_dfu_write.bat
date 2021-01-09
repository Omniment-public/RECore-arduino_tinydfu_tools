@echo off
set adfu_path=%~dp0
set PORT=%~2
set FILEPATH=%~3
set ADDRESS=0x8000000

rem echo %PORT% %FILEPATH% %ADDRESS%
rem %adfu_path%tiny_dfu_writer.exe %PORT% %FILEPATH% %ADDRESS%

%adfu_path%tiny_dfu.exe %PORT% %FILEPATH% %ADDRESS%