@echo off

set CompilerFlags=-O0 -g -gcodeview -isystem ../libs/CSFML/include -DLUDUM_WINDOWS=1
set LinkerFlags=-L../libs/CSFML/lib -lcsfml-audio -lcsfml-graphics -lcsfml-window -lcsfml-system

IF NOT EXIST "..\build" (mkdir "..\build")

pushd "..\build" > NUL 2> NUL

clang++ %CompilerFlags% "../code/CSFML_Ludum.cpp" -o CSFML_Ludum.exe %LinkerFlags%

popd > NUL 2> NUL

