rm -rf _debug _release
qmake ov.pro -r -spec macx-clang CONFIG+=x86_64
rm -rf debug release
make clean
make
macdeployqt _release/OptimusViewer.app
