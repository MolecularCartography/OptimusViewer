#include <QFile>
#include <QTextStream>

#include "CsvWritingUtils.h"

namespace ov {

namespace CsvWritingUtils {

QList<QStringList> createEmptyTable(int nRows, int nColumns)
{
    Q_ASSERT(nRows > 0 && nColumns > 0);
    QList<QStringList> result;
    result.reserve(nRows);
    for (int i = 0; i < nRows; ++i) {
        QStringList row;
        row.reserve(nColumns);
        for (int j = 0; j < nColumns; ++j) {
            row.append(QString());
        }
        result.append(row);
    }
    return result;
}

bool saveTableToFile(const QList<QStringList> &table, const QString &path)
{
    QFile file(path);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QStringList csvRows;
        foreach (QStringList row, table) {
            for (int i = 0; i < row.length(); ++i) {
                if (row[i].contains(",")) {
                    row[i] = QString("\"%1\"").arg(row[i]);
                }
            }
            csvRows.append(row.join(","));
        }

        QTextStream output(&file);
        output << csvRows.join("\n");
        return true;
    } else {
        return false;
    }
}

} // namespace CsvWritingUtils

} // namespace ov
