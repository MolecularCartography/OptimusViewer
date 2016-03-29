#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>

#include "CsvWritingUtils.h"
#include "FeatureTableModel.h"

#include "FeatureTableExporter.h"

namespace ov {

FeatureTableExporter::FeatureTableExporter(const FeatureTableModel &model)
    : model(model)
{

}

void FeatureTableExporter::exportFeatures()
{
    const QString path = QFileDialog::getSaveFileName(QApplication::activeWindow(), tr("Export Feature Table"), QString(), tr("CSV File (*.csv)"));

    if (path.isEmpty()) {
        return;
    }

    const int columnCount = model.columnCount() - 1; // -1 feature ID column
    const int rowCount = model.rowCount() + 1; // +1 header row
    QList<QStringList> table = CsvWritingUtils::createEmptyTable(rowCount, columnCount);

    for (int header = 0; header < columnCount; ++header) {
        table[0][header] = model.headerData(header + 1, Qt::Horizontal).toString();
    }

    for (int row = 1; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            table[row][column] = model.data(model.index(row - 1, column + 1)).toString();
        }
    }

    if (!CsvWritingUtils::saveTableToFile(table, path)) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Unable to save file: %1").arg(path));
    }
}

} // namespace ov
