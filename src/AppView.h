#ifndef APPVIEW_H
#define APPVIEW_H

#include <QMainWindow>
#include <QMap>

#include "Globals.h"

class QAbstractItemModel;
class QAction;
class QItemSelection;
class QWebView;

namespace Ui {

class AppViewUi;

}

namespace ov {

class FeatureTableModel;
class FeatureTableWidget;

class AppView : public QMainWindow
{
    Q_OBJECT

public:
    explicit AppView(QWidget *parent = NULL);
    ~AppView();

    void initViews(FeatureTableModel *model);
    const QAbstractItemModel * getTableModel() const;

signals:
    void open();
    void exit();
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
    void exportToCsvTriggered();
    void aboutTriggered();
    void filterTableTriggered();

private:
    void setDefaultSplitterSize();
    void initGraphView();
    void initFeatureTable(FeatureTableModel *model);
    void initActions();
    void connectGuiSignals();
    void setShortcuts();
    FeatureTableModel * getFeatureTableModel() const;

    bool graphViewInited;
    QAction *filterTableAction;

    FeatureTableWidget *featureTableView;
    Ui::AppViewUi *ui;
};

} // namespace ov

#endif // APPVIEW_H
