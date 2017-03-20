#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QScrollBar>

#include "FeatureTableItemDelegate.h"
#include "FeatureTableVisibilityDialog.h"

#include "FeatureTableWidget.h"

namespace ov {

FeatureTableWidget::FeatureTableWidget(QAbstractItemModel *model, int countOfFrozenColumns, QWidget *parent)
    : QTableView(parent), hideColumnAction(NULL), showHideColumnsAction(NULL), lastReferredLogicalColumn(-1),
    countOfFrozenColumns(countOfFrozenColumns), currentSortedColumn(-1)
{
    setModel(model);
    frozenTableView = new QTableView(this);

    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sp.setHorizontalStretch(0);
    sp.setVerticalStretch(0);
    sp.setHeightForWidth(sizePolicy().hasHeightForWidth());
    setSizePolicy(sp);
    setSortingEnabled(true);
    verticalHeader()->setVisible(false);

    initFrozenView();
    resetColumnHiddenState();
    initActions();
    connectGuiSignals();

    setItemDelegate(new FeatureTableItemDelegate(this));
}

FeatureTableWidget::~FeatureTableWidget()
{
    delete frozenTableView;
}

void FeatureTableWidget::initActions()
{
    hideColumnAction = new QAction(tr("Hide this column"), this);
    connect(hideColumnAction, &QAction::triggered, this, &FeatureTableWidget::hideColumnTriggered);

    showHideColumnsAction = new QAction(tr("Select columns to show..."), this);
    connect(showHideColumnsAction, &QAction::triggered, this, &FeatureTableWidget::showHideColumnsTriggered);
}

void FeatureTableWidget::initFrozenView()
{
    frozenTableView->setModel(model());
    frozenTableView->setSortingEnabled(true);
    frozenTableView->verticalHeader()->hide();

    viewport()->stackUnder(frozenTableView);

    frozenTableView->setSelectionModel(selectionModel());

    frozenTableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    frozenTableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    frozenTableView->show();

    updateFrozenTableGeometry();

    setHorizontalScrollMode(ScrollPerPixel);
    setVerticalScrollMode(ScrollPerPixel);
    frozenTableView->setVerticalScrollMode(ScrollPerPixel);
}

void FeatureTableWidget::connectGuiSignals()
{
    connect(horizontalHeader(), &QHeaderView::sectionResized, this, &FeatureTableWidget::updateSectionWidth);
    connect(verticalHeader(), &QHeaderView::sectionResized, this, &FeatureTableWidget::updateSectionHeight);

    connect(frozenTableView->verticalScrollBar(), &QAbstractSlider::valueChanged, verticalScrollBar(), &QAbstractSlider::setValue);
    connect(verticalScrollBar(), &QAbstractSlider::valueChanged, frozenTableView->verticalScrollBar(), &QAbstractSlider::setValue);

    QHeaderView *headerView = horizontalHeader();
    headerView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(headerView, &QHeaderView::customContextMenuRequested, this, &FeatureTableWidget::headerContextMenu);
    connect(headerView, &QHeaderView::sortIndicatorChanged, this, &FeatureTableWidget::headerSortIndicatorChanged);

    QHeaderView *frozenHeaderView = frozenTableView->horizontalHeader();
    frozenHeaderView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(frozenHeaderView, &QHeaderView::customContextMenuRequested, this, &FeatureTableWidget::headerContextMenu);
    connect(frozenHeaderView, &QHeaderView::sortIndicatorChanged, this, &FeatureTableWidget::headerSortIndicatorChanged);
    connect(frozenHeaderView, &QHeaderView::sectionResized, this, &FeatureTableWidget::frozenColumnResized);
}

void FeatureTableWidget::frozenColumnResized(int logicalIndex, int oldSize, int newSize)
{
    if (logicalIndex < countOfFrozenColumns && !frozenTableView->horizontalHeader()->isSectionHidden(logicalIndex)) {
        horizontalHeader()->resizeSection(logicalIndex, newSize);
    }
}

void FeatureTableWidget::headerSortIndicatorChanged(int logicalIndex, Qt::SortOrder order)
{
    if (currentSortedColumn != -1) {
        if (currentSortedColumn < countOfFrozenColumns) {
            frozenTableView->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        } else {
            horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        }
    }
    if (logicalIndex < countOfFrozenColumns) {
        frozenTableView->horizontalHeader()->setSortIndicator(logicalIndex, order);
    } else {
        horizontalHeader()->setSortIndicator(logicalIndex, order);
    }
    currentSortedColumn = logicalIndex;
}

void FeatureTableWidget::hideColumnTriggered()
{
    Q_ASSERT(-1 != lastReferredLogicalColumn);
    setColumnHidden(lastReferredLogicalColumn, true);
}

void FeatureTableWidget::showHideColumnsTriggered()
{
    Q_ASSERT(-1 != lastReferredLogicalColumn);

    QAbstractItemModel *m = model();
    const int columnCount = m->columnCount();

    QList<QPair<QString, bool> > headers;
    for (int i = 0; i < columnCount; ++i) {
        headers.append(QPair<QString, bool>(m->headerData(i, Qt::Horizontal).toString(), !isColumnHidden(i)));
    }

    FeatureTableVisibilityDialog d(headers, this);
    if (QDialog::Accepted == d.exec()) {
        const QBitArray updatedHeaders = d.getHeaderVisibility();
        Q_ASSERT(updatedHeaders.size() == headers.size());
        for (int i = 0; i < columnCount; ++i) {
            if (i < countOfFrozenColumns) {
                frozenTableView->setColumnHidden(i, !updatedHeaders.testBit(i));
            }
            setColumnHidden(i, !updatedHeaders.testBit(i));
        }
    }
}

void FeatureTableWidget::headerContextMenu(const QPoint &p)
{
    QHeaderView *headerView = horizontalHeader();
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

void FeatureTableWidget::setColumnHidden(int column, bool hide)
{
    if (column < countOfFrozenColumns) {
        frozenTableView->setColumnHidden(column, hide);
    }
    QTableView::setColumnHidden(column, hide);
}

void FeatureTableWidget::resetColumnHiddenState()
{
    const int modelColumnCount = model()->columnCount();
    for (int column = 0; column < modelColumnCount; ++column) {
        setColumnHidden(column, false);
        if (column >= countOfFrozenColumns) {
            resizeColumnToContents(column);
        }
    }

    for (int column = 0; column < modelColumnCount; ++column) {
        if (column < countOfFrozenColumns) {
            frozenTableView->setColumnHidden(column, false);
            //frozenTableView->resizeColumnToContents(column);
        } else {
            frozenTableView->setColumnHidden(column, true);
        }
    }
}

void FeatureTableWidget::updateSectionWidth(int logicalIndex, int /* oldSize */, int newSize)
{
    if (logicalIndex < countOfFrozenColumns) {
        frozenTableView->setColumnWidth(logicalIndex, newSize);
        updateFrozenTableGeometry();
    }
}

void FeatureTableWidget::updateSectionHeight(int logicalIndex, int /* oldSize */, int newSize)
{
    frozenTableView->setRowHeight(logicalIndex, newSize);
}

void FeatureTableWidget::resizeEvent(QResizeEvent * event)
{
    QTableView::resizeEvent(event);
    updateFrozenTableGeometry();
}

QModelIndex FeatureTableWidget::moveCursor(CursorAction cursorAction,
    Qt::KeyboardModifiers modifiers)
{
    QModelIndex current = QTableView::moveCursor(cursorAction, modifiers);
    int frozenWidth = 0;
    for (int i = 0; i < countOfFrozenColumns; ++i) {
        frozenWidth += frozenTableView->columnWidth(i);
    }
    if (cursorAction == MoveLeft && current.column() > countOfFrozenColumns && visualRect(current).topLeft().x() < frozenTableView->columnWidth(0)) {
        const int newValue = horizontalScrollBar()->value() + visualRect(current).topLeft().x() - frozenWidth;
        horizontalScrollBar()->setValue(newValue);
    }
    return current;
}

void FeatureTableWidget::scrollTo(const QModelIndex & index, ScrollHint hint){
    if (index.column() > countOfFrozenColumns) {
        QTableView::scrollTo(index, hint);
    }
}

void FeatureTableWidget::updateFrozenTableGeometry()
{
    int frozenWidth = 0;
    for (int i = 0; i < countOfFrozenColumns; ++i) {
        frozenWidth += frozenTableView->columnWidth(i);
    }
    frozenTableView->setGeometry(verticalHeader()->width() + frameWidth(), frameWidth(), frozenWidth, viewport()->height() + horizontalHeader()->height());
}

} // namespace ov
