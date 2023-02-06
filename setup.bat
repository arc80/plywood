@echo off
cl /Zi /Od /Fd"crowbar_bulk.pdb" /DPLY_WITH_ASSERTS=1 /I"base\src\platform" /I"data\build\crowbar\codegen\ply-platform" /I"base\src\runtime" /I"base\src\reflect" /I"base\src\cpp" /I"base\src\pylon\pylon" /I"base\src\pylon\reflect" /I"base\src\build\steps" /I"base\src\biscuit" /I"base\src\build\repo" /I"base\src\apps\crowbar\crowbar" base\scripts\crowbar_bulk.cpp /link /DEBUG shell32.lib ws2_32.lib winhttp.lib ole32.lib /incremental:no /out:crowbar.exe
del crowbar_bulk.obj
del crowbar_bulk.pdb
