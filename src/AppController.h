#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include "FeatureDataSource.h"
#include "AppView.h"
#include "FeatureTableExporter.h"
#include "FeatureTableModel.h"
#include "GraphDataController.h"
#include "GraphExporter.h"

class QWebView;

namespace ov {

class AppController : public QObject
{
    Q_OBJECT

public:
    AppController();

private slots:
    void graphViewAboutToLoad(QWebView *view);
    void graphDataControllerRequested();

private:
    void connectSingals();
    void initStatic();
    static void setWebSettings();
    static void registerMetatypes();

    static bool staticInitializationDone;
    FeatureDataSource dataSource;
    FeatureTableModel featureModel;
    AppView view;
    GraphDataController graphDataController;
    GraphExporter graphExporter;
    FeatureTableExporter featureTableExporter;
};

} // namespace qm

#endif // APPCONTROLLER_H
