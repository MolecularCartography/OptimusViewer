#ifndef CSV_WRITING_UTILS_H
#define CSV_WRITING_UTILS_H

#include <QStringList>

namespace qm {

namespace CsvWritingUtils {

QList<QStringList> createEmptyTable(int nRows, int nColumns);

bool saveTableToFile(const QList<QStringList> &table, const QString &path);

} // namespace CsvWritingUtils

} // namespace qm

#endif // CSV_WRITING_UTILS_H
