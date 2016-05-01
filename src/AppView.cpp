#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QTextStream>
#include <QWebFrame>
#include <QWebPage>
#include <QWebSecurityOrigin>

#include "ui_AppView.h"

#include "FeatureTableModel.h"
#include "FeatureTableVisibilityDialog.h"

#include "AppView.h"

namespace ov {

AppView::AppView(QWidget *parent)
    : QMainWindow(parent), graphViewInited(false), hideColumnAction(NULL), showHideColumnsAction(NULL), lastReferredLogicalColumn(-1),
    ui(new Ui::AppViewUi)
{
    ui->setupUi(this);
    initActions();
    connectGuiSignals();
    setShortcuts();
    showMaximized();

    setDefaultSplitterSize();
}

void AppView::initViews(FeatureTableModel *model)
{
    initGraphView();
    initFeatureTable(model);
}

AppView::~AppView()
{
    delete ui;
}

void AppView::initActions()
{
    ui->actionExportToCsv->setEnabled(false);

    hideColumnAction = new QAction(tr("Hide this column"), this);
    connect(hideColumnAction, &QAction::triggered, this, &AppView::hideColumnTriggered);

    showHideColumnsAction = new QAction(tr("Select columns to show..."), this);
    connect(showHideColumnsAction, &QAction::triggered, this, &AppView::showHideColumnsTriggered);
}

void AppView::hideColumnTriggered()
{
    Q_ASSERT(-1 != lastReferredLogicalColumn);

    ui->featureTableView->hideColumn(lastReferredLogicalColumn);
}

void AppView::showHideColumnsTriggered()
{
    Q_ASSERT(-1 != lastReferredLogicalColumn);

    FeatureTableModel *model = getFeatureTableModel();
    const int columnCount = model->columnCount();

    QList<QPair<QString, bool> > headers;
    for (int i = 1; i < columnCount; ++i) {
        headers.append(QPair<QString, bool>(model->headerData(i, Qt::Horizontal).toString(), !ui->featureTableView->isColumnHidden(i)));
    }

    FeatureTableVisibilityDialog d(headers, this);
    if (QDialog::Accepted == d.exec()) {
        const QBitArray updatedHeaders = d.getHeaderVisibility();
        Q_ASSERT(updatedHeaders.size() == headers.size());
        for (int i = 1; i < columnCount; ++i) {
            ui->featureTableView->setColumnHidden(i, !updatedHeaders.testBit(i - 1));
        }
    }
}

void AppView::exportToCsvTriggered()
{
    QVector<int> visibleColumns;
    FeatureTableModel *model = getFeatureTableModel();
    const int columnCount = model->columnCount();
    for (int i = 1; i < columnCount; ++i) {
        if (!ui->featureTableView->isColumnHidden(i)) {
            visibleColumns.append(i);
        }
    }
    emit exportToCsv(visibleColumns);
}

void AppView::connectGuiSignals()
{
    connect(ui->actionAbout, &QAction::triggered, this, &AppView::about);
    connect(ui->actionExit, &QAction::triggered, this, &AppView::exit);
    connect(ui->actionOpen, &QAction::triggered, this, &AppView::open);
    connect(ui->actionExportToCsv, &QAction::triggered, this, &AppView::exportToCsvTriggered);
}

void AppView::featureTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);

    QMultiHash<SampleId, FeatureId> currentSelection;
    QMap<FeatureId, qreal> featureMzs;

    QSortFilterProxyModel *proxyModel = dynamic_cast<QSortFilterProxyModel *>(ui->featureTableView->model());
    Q_ASSERT(NULL != proxyModel);
    FeatureTableModel *model = getFeatureTableModel();
    Q_ASSERT(NULL != model);
    foreach (const QModelIndex &index, ui->featureTableView->selectionModel()->selectedIndexes()) {
        if (index.column() > 3) {
            const FeatureId featureId = model->data(model->index(proxyModel->mapToSource(index).row(), 0)).value<FeatureId>(); // hidden 0th column contains feature id
            const SampleId sampleId = model->getSampleIdByColumnNumber(proxyModel->mapToSource(index).column());
            if (!featureMzs.contains(featureId)) {
                featureMzs[featureId] = model->data(model->index(proxyModel->mapToSource(index).row(), 1)).toReal(); // the 1st column contains mz value
            }
            currentSelection.insert(sampleId, featureId);
        }
    }

    emit featureSelectionChanged(currentSelection, featureMzs);
}

void AppView::setShortcuts()
{
    ui->actionOpen->setShortcut(QKeySequence::Open);
    ui->actionExit->setShortcut(QKeySequence::Quit);
    ui->actionAbout->setShortcut(QKeySequence::HelpContents);
}

void AppView::setDefaultSplitterSize()
{
    const int wholeSize = ui->mainSplitter->height();
    ui->mainSplitter->setSizes(QList<int>() << 3 * wholeSize / 5 << 2 * wholeSize / 5);
}

void AppView::initGraphView()
{
    QWebPage *webPage = ui->graphView->page();
    webPage->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
#ifndef _DEBUG
    ui->graphView->setContextMenuPolicy(Qt::NoContextMenu);
#endif

    QFile file(":/ov/src/html/GraphView.html");
    bool opened = file.open(QIODevice::ReadOnly);
    Q_ASSERT(opened);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    QString html = stream.readAll();
    file.close();

    connect(ui->graphView, &QWebView::loadFinished, this, &AppView::graphViewLoaded);
    emit graphViewAboutToLoad(ui->graphView);
    webPage->mainFrame()->setHtml(html);
}

void AppView::initFeatureTable(FeatureTableModel *model)
{
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(ui->featureTableView);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSourceModel(model);
    ui->featureTableView->setModel(proxyModel);

    connect(ui->featureTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AppView::featureTableSelectionChanged);

    QHeaderView *headerView = ui->featureTableView->horizontalHeader();
    headerView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(headerView, &QHeaderView::customContextMenuRequested, this, &AppView::featureTableHeaderContextMenu);
}

void AppView::featureTableHeaderContextMenu(const QPoint &p)
{
    QHeaderView *headerView = ui->featureTableView->horizontalHeader();
    lastReferredLogicalColumn = headerView->logicalIndexAt(p);

    if (-1 == lastReferredLogicalColumn) {
        return;
    }

    QMenu *menu = new QMenu(this);
    if (headerView->hiddenSectionCount() < headerView->count() - 1) {
        menu->addAction(hideColumnAction);
    }
    menu->addAction(showHideColumnsAction);
    menu->popup(headerView->viewport()->mapToGlobal(p));
}

FeatureTableModel * AppView::getFeatureTableModel() const
{
    QSortFilterProxyModel *proxyModel = dynamic_cast<QSortFilterProxyModel *>(ui->featureTableView->model());
    Q_ASSERT(NULL != proxyModel);
    FeatureTableModel *model = dynamic_cast<FeatureTableModel *>(proxyModel->sourceModel());
    Q_ASSERT(NULL != model);
    return model;
}

void AppView::samplesChanged()
{
    FeatureTableModel *model = getFeatureTableModel();
    model->reset();
    if (QSqlError::NoError != model->lastError().type()) {
        QMessageBox::critical(this, tr("Error"), model->lastError().text());
    } else {
        ui->featureTableView->hideColumn(0);
        for (int column = 1; column < model->columnCount(); ++column) {
            ui->featureTableView->resizeColumnToContents(column);
        }

        ui->actionExportToCsv->setEnabled(true);
    }
}

void AppView::graphViewLoaded(bool ok)
{
    disconnect(ui->graphView, &QWebView::loadFinished, this, &AppView::graphViewLoaded);
    Q_ASSERT(ok);
    graphViewInited = true;
}

void AppView::resetSelection()
{
    ui->featureTableView->selectionModel()->clearSelection();
}

} // namespace ov
