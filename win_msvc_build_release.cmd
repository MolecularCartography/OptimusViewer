@echo off
rmdir /s /q _debug _release
qmake -tp vc -r ov.pro CONFIG+=x64
rmdir /s /q debug release
msbuild /p:Configuration=Release /t:Clean,Build
windeployqt _release
