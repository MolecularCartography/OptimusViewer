#include <QFile>
#include <QMessageBox>
#include <QSqlError>
#include <QTextStream>
#include <QWebFrame>
#include <QWebPage>
#include <QWebSecurityOrigin>

#include "ui_AppView.h"

#include "FeatureTableModel.h"
#include "FeatureTableProxyModel.h"
#include "FeatureTableWidget.h"

#include "AppView.h"

namespace ov {

AppView::AppView(QWidget *parent)
    : QMainWindow(parent), graphViewInited(false), ui(new Ui::AppViewUi)
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

    filterTableAction = new QAction(tr("Filter feature table..."), this);
    connect(filterTableAction, &QAction::triggered, this, &AppView::filterTableTriggered);
    addAction(filterTableAction);
}

void AppView::exportToCsvTriggered()
{
    QVector<int> visibleColumns;
    FeatureTableModel *model = getFeatureTableModel();
    const int columnCount = model->columnCount();
    for (int i = 0; i < columnCount; ++i) {
        if (!featureTableView->isColumnHidden(i)) {
            visibleColumns.append(i);
        }
    }
    emit exportToCsv(visibleColumns);
}

void AppView::aboutTriggered()
{
    QFile file(":/ov/src/html/AboutApp.html");
    bool opened = file.open(QIODevice::ReadOnly);
    Q_ASSERT(opened);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    QString info = stream.readAll();
    file.close();

    info.prepend(QString("OptimusViewer version %1<br><br>").arg(CURRENT_OPTIMUS_VERSION));

    QMessageBox::information(this, tr("About OptimusViewer"), info);
}

void AppView::connectGuiSignals()
{
    connect(ui->actionAbout, &QAction::triggered, this, &AppView::aboutTriggered);
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

    FeatureTableProxyModel *proxyModel = dynamic_cast<FeatureTableProxyModel *>(featureTableView->model());
    Q_ASSERT(NULL != proxyModel);
    FeatureTableModel *model = getFeatureTableModel();
    Q_ASSERT(NULL != model);

    foreach (const QModelIndex &index, featureTableView->selectionModel()->selectedIndexes()) {
        if (index.column() > 4) {
            const QModelIndex sourceIndex = proxyModel->mapToSource(index);
            const FeatureId featureId = model->data(model->index(sourceIndex.row(), 0)).value<FeatureId>(); // the 0th column contains feature id
            const SampleId sampleId = model->getSampleIdByColumnNumber(sourceIndex.column());
            if (!featureMzs.contains(featureId)) {
                featureMzs[featureId] = model->data(model->index(sourceIndex.row(), 1)).toReal(); // the 1st column contains mz value
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
    filterTableAction->setShortcut(QKeySequence::Find);
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
    connect(model, &FeatureTableModel::setIndexWidget, this, &AppView::setFeatureTableIndexWidget);

    FeatureTableProxyModel *proxyModel = new FeatureTableProxyModel(model);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    connect(ui->filterEdit, &QLineEdit::textChanged, proxyModel, &FeatureTableProxyModel::setFilterFixedString);

    featureTableView = new FeatureTableWidget(proxyModel, model->countOfGeneralDataColumns(), ui->layoutWidget);
    featureTableView->setObjectName("featureTableView");
    ui->verticalLayout->addWidget(featureTableView);

    connect(featureTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AppView::featureTableSelectionChanged);
}

FeatureTableModel * AppView::getFeatureTableModel() const
{
    FeatureTableProxyModel *proxyModel = dynamic_cast<FeatureTableProxyModel *>(featureTableView->model());
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
        featureTableView->resetColumnHiddenState();
        ui->actionExportToCsv->setEnabled(true);
    }
}

void AppView::filterTableTriggered()
{
    ui->filterEdit->setFocus(Qt::MouseFocusReason);
}

void AppView::graphViewLoaded(bool ok)
{
    disconnect(ui->graphView, &QWebView::loadFinished, this, &AppView::graphViewLoaded);
    Q_ASSERT(ok);
    graphViewInited = true;
}

void AppView::resetSelection()
{
    featureTableView->selectionModel()->clearSelection();
}

void AppView::setFeatureTableIndexWidget(const QModelIndex &index, QWidget *w)
{
    FeatureTableProxyModel *proxyModel = dynamic_cast<FeatureTableProxyModel *>(featureTableView->model());
    Q_ASSERT(NULL != proxyModel);

    featureTableView->setIndexWidget(proxyModel->mapFromSource(index), w);
}

const QAbstractItemModel * AppView::getTableModel() const
{
    return featureTableView->model();
}

} // namespace ov
