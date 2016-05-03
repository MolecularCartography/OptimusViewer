#ifndef APPVIEW_H
#define APPVIEW_H

#include <QMainWindow>
#include <QMap>

#include "Globals.h"

class QAction;
class QItemSelection;
class QWebView;

namespace Ui {

class AppViewUi;

}

namespace qm {

class FeatureTableModel;

class AppView : public QMainWindow
{
    Q_OBJECT

public:
    explicit AppView(QWidget *parent = NULL);
    ~AppView();

    void initViews(FeatureTableModel *model);

signals:
    void open();
    void exit();
    void about();
    void exportToCsv(const QVector<int> &visibleColumns);

    void graphViewAboutToLoad(QWebView *view);
    void featureSelectionChanged(const QMultiHash<SampleId, FeatureId> &newSelection, const QMap<FeatureId, qreal> &featureMzs);

public slots:
    void samplesChanged();
    void resetSelection();
    void setFeatureTableIndexWidget(const QModelIndex &index, QWidget *w);

private slots:
    void graphViewLoaded(bool ok);
    void featureTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void featureTableHeaderContextMenu(const QPoint &p);
    void hideColumnTriggered();
    void showHideColumnsTriggered();
    void exportToCsvTriggered();

private:
    void setDefaultSplitterSize();
    void initGraphView();
    void initFeatureTable(FeatureTableModel *model);
    void initActions();
    void connectGuiSignals();
    void setShortcuts();
    FeatureTableModel * getFeatureTableModel() const;

    bool graphViewInited;
    QAction *hideColumnAction;
    QAction *showHideColumnsAction;
    int lastReferredLogicalColumn;

    Ui::AppViewUi *ui;
};

} // namespace qm

#endif // APPVIEW_H
