@echo off
qmake -tp vc -r ov.pro
rmdir /s /q "debug"
rmdir /s /q "release"
