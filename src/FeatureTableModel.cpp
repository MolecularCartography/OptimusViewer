#include <QLabel>

#include "FeatureDataSource.h"

#include "FeatureTableModel.h"

const int DEFAULT_TABLE_SIZE = 0;
const int SAMPLE_COLUMNS_OFFSET = 5;
const int ANNOTATION_COLUMN_OFFSET = 4;
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

int FeatureTableModel::countOfGeneralDataColumns() const
{
    return SAMPLE_COLUMNS_OFFSET;
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

void FeatureTableModel::updateFeatureAnnotationRows()
{
    featureAnnotationRows.clear();

    annotationFetcher = QSqlQuery("SELECT F.id, sub.comp_id, sub.link FROM Feature AS F, FeatureAnnotation AS FA, "
        "(SELECT A.id AS ann_id, A.compound_id AS comp_id, CWL.web_link AS link "
        "FROM Annotation AS A "
        "LEFT OUTER JOIN AnnotationWebLink AS AWL ON AWL.annotation_id = A.id "
        "LEFT OUTER JOIN CompoundWebLink AS CWL ON AWL.link_id = CWL.id) AS sub "
        "WHERE FA.feature_id = F.id AND FA.annotation_id = sub.ann_id ORDER BY F.id");

    FeatureId lastFeatureId = -1;
    qint64 recordNumber = 0;
    while (annotationFetcher.next()) {
        const FeatureId curFeatureId = annotationFetcher.value(0).value<FeatureId>();
        if (curFeatureId != lastFeatureId) {
            featureAnnotationRows[curFeatureId] = QPair<qint64, int>(recordNumber, 1);
            lastFeatureId = curFeatureId;
        } else {
            featureAnnotationRows[curFeatureId].second++;
        }
        recordNumber++;
    }
    annotationFetcher.seek(0);
}

void FeatureTableModel::invalidateCache()
{
    cachedCells = QVector<QVariant>(rowNumber * columnNumber);
}

void FeatureTableModel::reset()
{
    beginResetModel();

    updateRowNumber();
    updateColumnNumber();
    updateFeatureCount();
    invalidateCache();

    consensusFeatureFetcher = QSqlQuery("SELECT id, consensus_mz, consensus_rt, consensus_charge FROM Feature ORDER BY id");
    intensityFetcher = QSqlQuery("SELECT feature_id, sample_id, intensity FROM SampleFeature ORDER BY feature_id, sample_id");

    updateFeatureAnnotationRows();

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

Qt::ItemFlags FeatureTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QAbstractTableModel::flags(index);
    if (index.column() < SAMPLE_COLUMNS_OFFSET) {
        result &= ~Qt::ItemIsEnabled;
    }
    return result;
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

void FeatureTableModel::cacheValue(int row, int column, const QVariant &val)
{
    cachedCells[row * columnNumber + column] = val;
}

QVariant FeatureTableModel::getCachedValue(int row, int column) const
{
    return cachedCells[row * columnNumber + column];
}

typedef QPair<QString, QString> QStringPair;

QVariant FeatureTableModel::compoundIdColumnData(const QModelIndex &index)
{
    const int row = index.row();
    QVariant result;
    const FeatureId rowId = dataSource->getFeatureIdByNumber(row);
    if (featureAnnotationRows.contains(rowId)) {
        const qint64 annotationStartRecord = featureAnnotationRows[rowId].first;
        const int annotationCount = featureAnnotationRows[rowId].second;
        QList<QStringPair> compoundsAndLinks;
        for (int i = 0; i < annotationCount; ++i) {
            annotationFetcher.seek(annotationStartRecord + i);
            compoundsAndLinks.append(QStringPair(annotationFetcher.value(1).toString(), annotationFetcher.value(2).toString()));
        }
        QStringList resultAnns;
        QStringList compIds;
        bool linkExists = false;
        foreach(const QStringPair compAndLink, compoundsAndLinks) {
            if (!compAndLink.second.isEmpty()) {
                resultAnns.append(QString("<a href=\"%2\">%1</a>").arg(compAndLink.first, compAndLink.second));
                linkExists = true;
            } else {
                resultAnns.append(compAndLink.first);
            }
            compIds.append(compAndLink.first);
        }
        if (linkExists) {
            QLabel *label = new QLabel;
            label->setTextFormat(Qt::RichText);
            label->setText(resultAnns.join("; "));
            label->setOpenExternalLinks(true);
            emit setIndexWidget(index, label);
        }
        result = QString(compIds.join("; "));
    } else {
        result = tr("N/A");
    }
    return result;
}

QVariant FeatureTableModel::dataInternal(const QModelIndex &index, int role)
{
    QVariant result;
    const int row = index.row();
    const int column = index.column();

    if (!index.isValid() || (role & ~Qt::DisplayRole) || row >= rowNumber || column >= columnNumber) {
        return result;
    }

    const QVariant cachedVal = getCachedValue(row, column);
    if (cachedVal.isValid()) {
        return cachedVal;
    }

    if (column < ANNOTATION_COLUMN_OFFSET) {
        consensusFeatureFetcher.seek(row);
        result = consensusFeatureFetcher.value(column);
    } else if (column == ANNOTATION_COLUMN_OFFSET) {
        result = compoundIdColumnData(index);
    } else {
        const FeatureId rowId = dataSource->getFeatureIdByNumber(row);
        const SampleId columnId = dataSource->getSampleIdByNumber(column - SAMPLE_COLUMNS_OFFSET);

        const qint64 firstFeatureRecord = findFirstQueryRecordIndex(intensityFetcher, 0, totalFeatureObservationCount - 1, 0, rowId);
        if (intensityFetcher.value(0).value<FeatureId>() != rowId) {
            result = TABLE_DEFAULT_VALUE;
        } else {
            const qint64 endRecordIndex = firstFeatureRecord + featureObservationCount[rowId] - 1;
            findFirstQueryRecordIndex(intensityFetcher, firstFeatureRecord, endRecordIndex, 1, columnId);

            result = intensityFetcher.value(1).value<SampleId>() == columnId ? intensityFetcher.value(2) : TABLE_DEFAULT_VALUE;
        }
    }
    cacheValue(row, column, result);
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
            case 4:
                return tr("Compound ID");
            default:
                return QVariant(dataSource->getSampleNameById(dataSource->getSampleIdByNumber(section - SAMPLE_COLUMNS_OFFSET)));
        }
    } else {
        return QAbstractItemModel::headerData(section, orientation, role);
    }
}

} // namespace ov
