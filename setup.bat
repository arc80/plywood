@echo off
cl /Zi /Fd"bulk_crowbar.pdb" /DPLY_WITH_ASSERTS=1 /I"base\src\platform" /I"data\build\crowbar\codegen\ply-platform" /I"base\src\runtime" /I"base\src\pylon\pylon" /I"base\src\reflect" /I"base\src\pylon\reflect" /I"base\src\build\common" /I"base\src\build\folder" /I"base\src\cpp" /I"base\src\apps\crowbar\crowbar-client" /I"base\src\buildSteps" /I"base\src\biscuit" /I"base\src\build\repository" /I"base\src\apps\crowbar\crowbar" base\scripts\bulk_crowbar.cpp /link shell32.lib ws2_32.lib winhttp.lib ole32.lib /incremental:no /out:crowbar-new.exe
del bulk_crowbar.obj
del bulk_crowbar.pdb
