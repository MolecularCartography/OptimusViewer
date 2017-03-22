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
        const int row = index.row();
        bool intersectsSelection = false;
        foreach (const QModelIndex &selectedIndex, view->selectionModel()->selectedIndexes()) {
            if (selectedIndex.row() == row) {
                intersectsSelection = true;
                break;
            }
        }
        QStyleOptionViewItem customOption = option;
        customOption.font.setBold(intersectsSelection);
        QStyledItemDelegate::paint(painter, customOption, index);
    }
}

} // namespace ov
