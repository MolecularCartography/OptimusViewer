include (ov.pri)

HEADERS += src/AppController.h \
           src/AppView.h \
           src/CsvWritingUtils.h \
           src/FeatureDataSource.h \
           src/FeatureTableExporter.h \
           src/FeatureTableModel.h \
           src/FeatureTableVisibilityDialog.h \
           src/Globals.h \
           src/GraphDataController.h \
           src/GraphExporter.h \
           src/GraphPoint.h \
           src/ProgressIndicator.h \
           src/SaveGraphDialog.h

FORMS += src/ui/AppView.ui \
         src/ui/FeatureTableVisibilityDialog.ui \
         src/ui/ProgressIndicator.ui

SOURCES += src/AppController.cpp \
           src/AppView.cpp \
           src/CsvWritingUtils.cpp \
           src/FeatureDataSource.cpp \
           src/FeatureTableExporter.cpp \
           src/FeatureTableModel.cpp \
           src/FeatureTableVisibilityDialog.cpp \
           src/Globals.cpp \
           src/GraphDataController.cpp \
           src/GraphPoint.cpp \
           src/GraphExporter.cpp \
           src/Main.cpp \
           src/ProgressIndicator.cpp \
           src/SaveGraphDialog.cpp

RESOURCES += ov.qrc

