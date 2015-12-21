#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include "FeatureDataSource.h"
#include "AppView.h"
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
    AppView view;
    GraphDataController xicDataController;
    GraphExporter graphExporter;
};

} // namespace ov

#endif // APPCONTROLLER_H
