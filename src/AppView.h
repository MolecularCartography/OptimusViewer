#ifndef APPVIEW_H
#define APPVIEW_H

#include <QMainWindow>
#include <QMap>

#include "Globals.h"

class QAction;
class QItemSelection;
class QSqlQueryModel;
class QWebView;

namespace Ui {

class AppViewUi;

}

namespace ov {

class AppView : public QMainWindow
{
    Q_OBJECT

public:
    explicit AppView(QWidget *parent = NULL);
    ~AppView();

    void initViews();

signals:
    void open();
    void exit();
    void about();

    void graphViewAboutToLoad(QWebView *view);
    void featureSelectionChanged(const QMultiHash<SampleId, FeatureId> &newSelection, const QMap<FeatureId, qreal> &featureMzs);

public slots:
    void samplesChanged(const QMap<SampleId, QString> &sampleNameById);

private slots:
    void graphViewLoaded(bool ok);
    void featureTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void featureTableHeaderContextMenu(const QPoint &p);
    void hideColumnTriggered();
    void showHideColumnsTriggered();

private:
    void setDefaultSplitterSize();
    void initGraphView();
    void initFeatureTable();
    void initActions();
    void connectGuiSignals();
    void setShortcuts();
    QSqlQueryModel * getFeatureTableModel() const;

    bool graphViewInited;
    QMap<int, SampleId> sampleIdByColumn;
    QAction *hideColumnAction;
    QAction *showHideColumnsAction;
    int lastReferredLogicalColumn;

    Ui::AppViewUi *ui;
};

} // namespace ov

#endif // APPVIEW_H
