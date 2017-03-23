#include "FeatureTableProxyModel.h"

namespace ov {

FeatureTableProxyModel::FeatureTableProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

bool FeatureTableProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QString filter = filterRegExp().pattern();
    if (filter.isEmpty()) {
        return true;
    }

    QAbstractItemModel *originalModel = sourceModel();
    const int columnNum = originalModel->columnCount();
    for (int column = 0; column < columnNum; ++column) {
        if (originalModel->data(originalModel->index(sourceRow, column, sourceParent)).toString().contains(filter, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

} // namespace ov
