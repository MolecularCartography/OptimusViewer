#ifndef FEATURE_TABLE_ITEM_DELEGATE_H
#define FEATURE_TABLE_ITEM_DELEGATE_H

#include <QStyledItemDelegate>

namespace ov {

class FeatureTableItemDelegate : public QStyledItemDelegate
{
public:
    FeatureTableItemDelegate(QAbstractItemView *view);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    QAbstractItemView *view;
};

} // namespace qm

#endif // FEATURE_TABLE_ITEM_DELEGATE_H
