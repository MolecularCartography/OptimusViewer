#include "FeatureDataSource.h"

#include "FeatureTableModel.h"

const int DEFAULT_TABLE_SIZE = 0;
const int SAMPLE_COLUMNS_OFFSET = 4;
const QVariant TABLE_DEFAULT_VALUE = QVariant("0");

namespace ov {

FeatureTableModel::FeatureTableModel(QObject *parent, FeatureDataSource *dataSource)
    : QAbstractTableModel(parent), rowNumber(DEFAULT_TABLE_SIZE), columnNumber(DEFAULT_TABLE_SIZE), dataSource(dataSource)
{

}

void FeatureTableModel::updateRowNumber()
{
    rowNumber = dataSource->getFeatureCount();
}

void FeatureTableModel::updateColumnNumber()
{
    columnNumber = SAMPLE_COLUMNS_OFFSET + dataSource->getSampleCount();
}

void FeatureTableModel::reset()
{
    beginResetModel();

    updateRowNumber();
    updateColumnNumber();

    consensusFeatureFetcher = QSqlQuery();
    consensusFeatureFetcher.prepare("SELECT id, consensus_mz, consensus_rt, consensus_charge FROM Feature WHERE id = ?");
    intensityFetcher = QSqlQuery();
    intensityFetcher.prepare("SELECT intensity FROM SampleFeature WHERE feature_id = ? AND sample_id = ?");

    endResetModel();
}

QSqlError FeatureTableModel::lastError() const
{
    return error;
}

int FeatureTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : columnNumber;
}

int FeatureTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : rowNumber;
}

SampleId FeatureTableModel::getSampleIdByColumnNumber(int column) const
{
    if (column >= SAMPLE_COLUMNS_OFFSET && column < columnNumber) {
        return dataSource->getSampleIdByNumber(column - SAMPLE_COLUMNS_OFFSET);
    } else {
        Q_ASSERT(false);
        return -1;
    }
}

QVariant FeatureTableModel::getQueryResultValue(QSqlQuery &query, const QVector<QVariant> &parameters, int fieldNumber)
{
    QVariant result;

    foreach (const QVariant &parameter, parameters) {
        query.addBindValue(parameter);
    }

    if (!query.exec()) {
        error = query.lastError();
        printf(error.text().toLocal8Bit().data());
        return result;
    }
    if (query.next()) {
        result = query.value(fieldNumber);
    }
    query.finish();
    return result;
}

QVariant FeatureTableModel::data(const QModelIndex &index, int role) const
{
    return const_cast<FeatureTableModel *>(this)->dataInternal(index, role);
}

QVariant FeatureTableModel::dataInternal(const QModelIndex &index, int role)
{
    QVariant result;

    if (!index.isValid() || (role & ~Qt::DisplayRole) || index.row() >= rowNumber || index.column() >= columnNumber) {
        return result;
    }

    const FeatureId rowId = dataSource->getFeatureIdByNumber(index.row());
    if (index.column() < SAMPLE_COLUMNS_OFFSET) {
        result = getQueryResultValue(consensusFeatureFetcher, QVector<QVariant>() << rowId, index.column());
    } else {
        const SampleId columnId = dataSource->getSampleIdByNumber(index.column() - SAMPLE_COLUMNS_OFFSET);
        result = getQueryResultValue(intensityFetcher, QVector<QVariant>() << rowId << columnId, 0);
    }
    return result.isValid() ? result : TABLE_DEFAULT_VALUE;
}

QVariant FeatureTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 && section < columnNumber) {
        switch (section) {
            case 0:
                return tr("Feature ID");
            case 1:
                return tr("Consensus mz");
            case 2:
                return tr("Consensus RT");
            case 3:
                return tr("Consensus charge");
            default:
                return QVariant(dataSource->getSampleNameById(dataSource->getSampleIdByNumber(section - SAMPLE_COLUMNS_OFFSET)));
        }
    } else {
        return QAbstractItemModel::headerData(section, orientation, role);
    }
}

} // namespace ov
