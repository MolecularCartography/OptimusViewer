@echo off
rmdir /s /q _debug _release
qmake -tp vc -r ov.pro
rmdir /s /q debug release
msbuild /p:Configuration=Release /t:Clean,Build
windeployqt _release
