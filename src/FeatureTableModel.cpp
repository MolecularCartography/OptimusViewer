#include "FeatureDataSource.h"

#include "FeatureTableModel.h"

const int DEFAULT_TABLE_SIZE = 0;
const int SAMPLE_COLUMNS_OFFSET = 4;
const QVariant TABLE_DEFAULT_VALUE = QVariant("0");

namespace ov {

FeatureTableModel::FeatureTableModel(QObject *parent, FeatureDataSource *dataSource)
    : QAbstractTableModel(parent), rowNumber(DEFAULT_TABLE_SIZE), columnNumber(DEFAULT_TABLE_SIZE), dataSource(dataSource),
    totalFeatureObservationCount(0)
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

void FeatureTableModel::updateFeatureCount()
{
    featureObservationCount.clear();
    totalFeatureObservationCount = 0;

    QSqlQuery sampleFeatureCountQuery("SELECT feature_id, COUNT(*) FROM SampleFeature WHERE feature_id IN "
        "(SELECT DISTINCT feature_id FROM SampleFeature) GROUP BY feature_id");
    while (sampleFeatureCountQuery.next()) {
        const qint64 featureCount = sampleFeatureCountQuery.value(1).toLongLong();
        featureObservationCount[sampleFeatureCountQuery.value(0).value<FeatureId>()] = featureCount;
        totalFeatureObservationCount += featureCount;
    }
}

void FeatureTableModel::reset()
{
    beginResetModel();

    updateRowNumber();
    updateColumnNumber();
    updateFeatureCount();

    consensusFeatureFetcher = QSqlQuery("SELECT id, consensus_mz, consensus_rt, consensus_charge FROM Feature ORDER BY consensus_mz");
    intensityFetcher = QSqlQuery("SELECT feature_id, sample_id, intensity FROM SampleFeature ORDER BY feature_id, sample_id");

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

qint64 FeatureTableModel::findFirstQueryRecordIndex(QSqlQuery &query, qint64 startIndex, qint64 endIndex, int fieldNumber, qint64 value)
{
    qint64 left = startIndex;
    qint64 right = endIndex;

    while (right - left > 1) {
        qint64 middle = left + (right - left) / 2;
        query.seek(middle);
        if (query.value(fieldNumber).toLongLong() < value) {
            left = middle;
        } else {
            right = middle;
        }
    }
    query.seek(left);
    if (query.value(fieldNumber).toLongLong() == value) {
        return left;
    } else {
        query.seek(right);
        return right;
    }
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

    if (index.column() < SAMPLE_COLUMNS_OFFSET) {
        consensusFeatureFetcher.seek(index.row());
        result = consensusFeatureFetcher.value(index.column());
    } else {
        const FeatureId rowId = dataSource->getFeatureIdByNumber(index.row());
        const SampleId columnId = dataSource->getSampleIdByNumber(index.column() - SAMPLE_COLUMNS_OFFSET);

        const qint64 firstFeatureRecord = findFirstQueryRecordIndex(intensityFetcher, 0, totalFeatureObservationCount - 1, 0, rowId);
        if (intensityFetcher.value(0).value<FeatureId>() != rowId) {
            result = TABLE_DEFAULT_VALUE;
        } else {
            const qint64 endRecordIndex = firstFeatureRecord + featureObservationCount[rowId] - 1;
            const qint64 targetFeatureRecord = findFirstQueryRecordIndex(intensityFetcher, firstFeatureRecord, endRecordIndex, 1, columnId);

            result = intensityFetcher.value(1).value<SampleId>() == columnId ? intensityFetcher.value(2) : TABLE_DEFAULT_VALUE;
        }
    }
    return result;
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
