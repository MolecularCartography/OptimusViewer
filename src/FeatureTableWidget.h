#ifndef FEATURETABLEWIDGET_H
#define FEATURETABLEWIDGET_H

#include <QTableView>

class QAction;

namespace ov {

class FeatureTableWidget : public QTableView {
    Q_OBJECT

public:
    FeatureTableWidget(QAbstractItemModel *model, int countOfFrozenColumns, QWidget *parent);
    ~FeatureTableWidget();

    void setColumnHidden(int column, bool hide);
    void resetColumnHiddenState();

protected:
    void resizeEvent(QResizeEvent *event);
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);

private slots:
    void updateSectionWidth(int logicalIndex, int oldSize, int newSize);
    void updateSectionHeight(int logicalIndex, int oldSize, int newSize);
    void headerContextMenu(const QPoint &p);
    void headerSortIndicatorChanged(int logicalIndex, Qt::SortOrder order);
    void frozenColumnResized(int logicalIndex, int oldSize, int newSize);
    void hideColumnTriggered();
    void showHideColumnsTriggered();

private:
    void connectGuiSignals();
    void initActions();
    void initFrozenView();
    void updateFrozenTableGeometry();

    QAction *hideColumnAction;
    QAction *showHideColumnsAction;
    QTableView *frozenTableView;
    int lastReferredLogicalColumn;
    int countOfFrozenColumns;
    int currentSortedColumn;
};

} // namespace ov

#endif // FEATURETABLEWIDGET_H
