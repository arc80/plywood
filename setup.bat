@echo off
cl /Zi /Fd"bulk.pdb" /DPLY_WITH_ASSERTS=1 /I"base\src\platform" /I"data\build\crowbar\codegen\ply-platform" /I"base\src\runtime" /I"base\src\reflect" /I"base\src\build\common" /I"base\src\build\target" /I"data\build\crowbar\codegen\ply-build-target" /I"base\src\pylon\pylon" /I"base\src\pylon\reflect" /I"base\src\build\provider" /I"base\src\cpp" /I"base\src\build\repo" /I"base\src\build\folder" /I"base\src\apps\crowbar\crowbar-client" /I"base\src\buildSteps" /I"base\src\biscuit" /I"base\src\build\repository" /I"base\src\apps\crowbar\crowbar" base\scripts\bulk.cpp /link shell32.lib ws2_32.lib winhttp.lib ole32.lib /incremental:no /out:crowbar-new.exe
del bulk.obj
del bulk.pdb
