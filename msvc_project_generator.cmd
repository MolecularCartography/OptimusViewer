@echo off
qmake -tp vc -r quickmix.pro
rmdir /s /q "debug"
rmdir /s /q "release"
