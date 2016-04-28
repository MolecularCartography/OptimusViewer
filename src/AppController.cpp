#include <QApplication>
#include <QWebFrame>
#include <QWebSettings>
#include <QWebView>

#include "AppController.h"

namespace ov {

bool AppController::staticInitializationDone = false;

AppController::AppController()
    : featureModel(NULL, &dataSource), graphDataController(&dataSource), featureTableExporter(featureModel)
{
    initStatic();
    connectSingals();

    view.initViews(&featureModel);
    view.show();
}

void AppController::initStatic()
{
    if (!staticInitializationDone) {
        registerMetatypes();
        setWebSettings();
        staticInitializationDone = true;
    }
}

void AppController::registerMetatypes()
{
    qRegisterMetaType<GraphId>("GraphId");
    qRegisterMetaType<FormatId>("FormatId");
}

void AppController::setWebSettings()
{
    QWebSettings *webSettings = QWebSettings::globalSettings();
    webSettings->setAttribute(QWebSettings::Accelerated2dCanvasEnabled, true);
    webSettings->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, false);
    webSettings->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
#ifdef _DEBUG
    webSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
#endif
}

void AppController::connectSingals()
{
    connect(&view, &AppView::open, &dataSource, &FeatureDataSource::selectDataSource);
    connect(&view, &AppView::exportToCsv, &featureTableExporter, &FeatureTableExporter::exportFeatures);
    connect(&view, &AppView::about, &QApplication::aboutQt);
    connect(&view, &AppView::exit, &QCoreApplication::quit);
    connect(&view, &AppView::graphViewAboutToLoad, this, &AppController::graphViewAboutToLoad);
    connect(&view, &AppView::featureSelectionChanged, &graphDataController, &GraphDataController::featureSelectionChanged);

    connect(&dataSource, &FeatureDataSource::samplesChanged, &view, &AppView::samplesChanged);
    connect(&dataSource, &FeatureDataSource::samplesChanged, &graphDataController, &GraphDataController::samplesChanged);

    connect(&graphDataController, &GraphDataController::resetActiveFeatures, &view, &AppView::resetSelection);
}

void AppController::graphDataControllerRequested()
{
    QWebFrame *frame = dynamic_cast<QWebFrame *>(sender());
    frame->addToJavaScriptWindowObject("dataController", &graphDataController);
    frame->addToJavaScriptWindowObject("graphExporter", &graphExporter);
}

void AppController::graphViewAboutToLoad(QWebView *view)
{
    Q_ASSERT(NULL != view);
    graphExporter.setGraphView(view);
    connect(view->page()->mainFrame(), &QWebFrame::javaScriptWindowObjectCleared, this, &AppController::graphDataControllerRequested);
}

} // namespace ov
