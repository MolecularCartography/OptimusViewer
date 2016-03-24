include (ov.pri)

HEADERS += src/AppController.h \
           src/AppView.h \
           src/FeatureDataSource.h \
           src/FeatureTableModel.h \
           src/FeatureTableVisibilityDialog.h \
           src/Globals.h \
           src/GraphDataController.h \
           src/GraphExporter.h \
           src/GraphPoint.h \
           src/SaveGraphDialog.h

FORMS += src/ui/AppView.ui \
         src/ui/FeatureTableVisibilityDialog.ui

SOURCES += src/AppController.cpp \
           src/AppView.cpp \
           src/FeatureDataSource.cpp \
           src/FeatureTableModel.cpp \
           src/FeatureTableVisibilityDialog.cpp \
           src/Globals.cpp \
           src/GraphDataController.cpp \
           src/GraphPoint.cpp \
           src/GraphExporter.cpp \
           src/Main.cpp \
           src/SaveGraphDialog.cpp

RESOURCES += ov.qrc

