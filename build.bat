@echo off

setlocal

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" > nul

cl /Zi /FC /nologo main.cpp /link User32.lib kernel32.lib

endlocal