@echo off
if not exist build mkdir build
if not exist temp mkdir temp

REM del build\*.pdb > NUL 2> NUL

xcopy deps\bin\x64\*.dll build /Y /S /E /Q /D >NUL
call tools\build\reflect --input:src\ --template:src\main\reflection.template --exclude:src\engine\std.h --output:src\main\reflection.cpp --inc_base:src\
call fbuild

call tools\build\ShaderCompiler src/shaders data/shaders
