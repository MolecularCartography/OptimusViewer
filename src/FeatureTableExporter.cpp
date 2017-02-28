#include <QAbstractItemModel>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>

#include "AppView.h"
#include "CsvWritingUtils.h"

#include "FeatureTableExporter.h"

namespace ov {

FeatureTableExporter::FeatureTableExporter(const AppView &appView)
    : appView(appView)
{

}

void FeatureTableExporter::exportFeatures(const QVector<int> &visibleColumns)
{
    const QString path = QFileDialog::getSaveFileName(QApplication::activeWindow(), tr("Export Feature Table"), QString(), tr("CSV File (*.csv)"));

    if (path.isEmpty()) {
        return;
    }

    const int columnCount = visibleColumns.size();
    const QAbstractItemModel *featureTableModel = appView.getTableModel();
    Q_ASSERT(NULL != featureTableModel);
    const int rowCount = featureTableModel->rowCount() + 1; // +1 header row
    QList<QStringList> table = CsvWritingUtils::createEmptyTable(rowCount, columnCount);

    for (int column = 0; column < columnCount; ++column) {
        table[0][column] = featureTableModel->headerData(visibleColumns[column], Qt::Horizontal).toString();
    }

    for (int row = 1; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            table[row][column] = featureTableModel->data(featureTableModel->index(row - 1, visibleColumns[column])).toString();
        }
    }

    if (!CsvWritingUtils::saveTableToFile(table, path)) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Unable to save file: %1").arg(path));
    }
}

} // namespace ov
