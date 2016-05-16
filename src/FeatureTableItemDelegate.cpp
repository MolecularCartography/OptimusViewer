#include <QAbstractItemView>

#include "FeatureTableItemDelegate.h"

namespace ov {

FeatureTableItemDelegate::FeatureTableItemDelegate(QAbstractItemView *view)
    : QStyledItemDelegate(view), view(view)
{

}

void FeatureTableItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (NULL == view->indexWidget(index)) {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

} // namespace qm
