#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>

#include "CsvWritingUtils.h"
#include "FeatureTableModel.h"

#include "FeatureTableExporter.h"

namespace qm {

FeatureTableExporter::FeatureTableExporter(const FeatureTableModel &model)
    : model(model)
{

}

void FeatureTableExporter::exportFeatures(const QVector<int> &visibleColumns)
{
    const QString path = QFileDialog::getSaveFileName(QApplication::activeWindow(), tr("Export Feature Table"), QString(), tr("CSV File (*.csv)"));

    if (path.isEmpty()) {
        return;
    }

    const int columnCount = visibleColumns.size();
    const int rowCount = model.rowCount() + 1; // +1 header row
    QList<QStringList> table = CsvWritingUtils::createEmptyTable(rowCount, columnCount);

    for (int column = 0; column < columnCount; ++column) {
        table[0][column] = model.headerData(visibleColumns[column], Qt::Horizontal).toString();
    }

    for (int row = 1; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            table[row][column] = model.data(model.index(row - 1, visibleColumns[column])).toString();
        }
    }

    if (!CsvWritingUtils::saveTableToFile(table, path)) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Unable to save file: %1").arg(path));
    }
}

} // namespace qm
