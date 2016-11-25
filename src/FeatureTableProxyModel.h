#ifndef FEATURE_TABLE_PROXY_MODEL_H
#define FEATURE_TABLE_PROXY_MODEL_H

#include <QSortFilterProxyModel>

namespace ov {

class FeatureTableProxyModel : public QSortFilterProxyModel
{
public:
    FeatureTableProxyModel(QObject *parent);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
};

} // namespace ov

#endif // FEATURE_TABLE_PROXY_MODEL_H
