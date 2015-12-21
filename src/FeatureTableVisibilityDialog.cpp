#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include "ui_FeatureTableVisibilityDialog.h"

#include "FeatureTableVisibilityDialog.h"

namespace ov {

FeatureTableVisibilityDialog::FeatureTableVisibilityDialog(const QList<QPair<QString, bool> > &headers, QWidget *parent)
    : QDialog(parent), ui(new Ui::FeatureTableVisibilityDialogUi)
{
    ui->setupUi(this);
    connectGui();
    initHeaderList(headers);
}

FeatureTableVisibilityDialog::~FeatureTableVisibilityDialog()
{
    delete ui;
}

QStandardItemModel * FeatureTableVisibilityDialog::getHeaderListModel() const
{
    return dynamic_cast<QStandardItemModel *>(dynamic_cast<QSortFilterProxyModel *>(ui->headerListView->model())->sourceModel());
}

void FeatureTableVisibilityDialog::initHeaderList(const QList<QPair<QString, bool> > &headers)
{
    QStandardItemModel *listModel = new QStandardItemModel(headers.length(), 1, this);
    for (int i = 0, n = headers.length(); i < n; ++i) {
        const QPair<QString, bool> &header = headers[i];
        QStandardItem *item = new QStandardItem(header.first);
        item->setCheckable(true);
        item->setCheckState(header.second ? Qt::Checked : Qt::Unchecked);
        listModel->setItem(i, item);
    }

    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(listModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    connect(ui->filterLineEdit, &QLineEdit::textChanged, proxyModel, &QSortFilterProxyModel::setFilterFixedString);

    ui->headerListView->setModel(proxyModel);
}

void FeatureTableVisibilityDialog::connectGui()
{
    connect(ui->selectAllButton, &QPushButton::clicked, this, &FeatureTableVisibilityDialog::selectAllClicked);
    connect(ui->clearSelectionButton, &QPushButton::clicked, this, &FeatureTableVisibilityDialog::clearClicked);
}

QBitArray FeatureTableVisibilityDialog::getHeaderVisibility() const
{
    QStandardItemModel *model = getHeaderListModel();
    const int rowCount = model->rowCount();
    QBitArray result(rowCount);
    for (int i = 0; i < rowCount; ++i) {
        result.setBit(i, model->item(i)->checkState() == Qt::Checked);
    }
    return result;
}

void FeatureTableVisibilityDialog::setListItemsCheckState(Qt::CheckState state)
{
    QStandardItemModel *model = getHeaderListModel();
    const int rowCount = model->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        model->item(i)->setCheckState(state);
    }
}

void FeatureTableVisibilityDialog::selectAllClicked()
{
    setListItemsCheckState(Qt::Checked);
}

void FeatureTableVisibilityDialog::clearClicked()
{
    setListItemsCheckState(Qt::Unchecked);
}

void FeatureTableVisibilityDialog::accept()
{
    QStandardItemModel *model = getHeaderListModel();
    const int rowCount = model->rowCount();
    bool selectionExists = false;
    for (int i = 0; i < rowCount; ++i) {
        if (Qt::Checked == model->item(i)->checkState()) {
            selectionExists = true;
            break;
        }
    }
    if (!selectionExists) {
        QMessageBox::warning(this, tr("Warning"), tr("Please, select at least one header."));
    } else {
        QDialog::accept();
    }
}

} // namespace ov
