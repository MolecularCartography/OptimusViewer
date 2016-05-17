QT += core gui printsupport sql svg webkit webkitwidgets
TEMPLATE = app
CONFIG += debug_and_release

CONFIG(debug, debug|release) {
    TARGET = OptimusViewerd
    DEFINES += _DEBUG
    CONFIG += console
    DESTDIR = _debug
    MOC_DIR = _tmp/moc/debug
    OBJECTS_DIR = _tmp/obj/debug
}

CONFIG(release, debug|release) {
    TARGET = OptimusViewer
    DEFINES += NDEBUG
    DESTDIR = _release
    MOC_DIR = _tmp/moc/release
    OBJECTS_DIR = _tmp/obj/release
}

UI_DIR = _tmp/ui
RCC_DIR = _tmp/rcc

win32 {
    QMAKE_CXXFLAGS_WARN_ON = -W3
    QMAKE_CFLAGS_WARN_ON = -W3
}

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
    CONFIG += c++11
}

DEFINES += MIN_COMPATIBLE_OPTIMUS_VERSION='"0.1"'
DEFINES += CURRENT_OPTIMUS_VERSION='"0.1"'
