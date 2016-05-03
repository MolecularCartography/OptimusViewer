#ifndef FEATURE_TABLE_MODEL_H
#define FEATURE_TABLE_MODEL_H

#include <QAbstractTableModel>
#include <QSqlError>
#include <QSqlQuery>

#include "Globals.h"

namespace qm {

class FeatureDataSource;

class FeatureTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    FeatureTableModel(QObject *parent, FeatureDataSource *dataSource);

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    void reset();
    QSqlError lastError() const;

    SampleId FeatureTableModel::getSampleIdByColumnNumber(int column) const;

signals:
    void setIndexWidget(const QModelIndex &index, QWidget *w);

private:
    void updateRowNumber();
    void updateColumnNumber();
    void updateFeatureCount();
    void updateFeatureAnnotationRows();
    void invalidateCache();
    void cacheValue(int row, int column, const QVariant &val);
    QVariant getCachedValue(int row, int column) const;
    QVariant dataInternal(const QModelIndex &index, int role);
    QVariant compoundIdColumnData(const QModelIndex &index);
    static qint64 findFirstQueryRecordIndex(QSqlQuery &query, qint64 startIndex, qint64 endIndex, int fieldNumber, qint64 value);

    qint64 rowNumber;
    qint64 columnNumber;
    FeatureDataSource *dataSource;

    QSqlQuery consensusFeatureFetcher;
    QSqlQuery intensityFetcher;
    QSqlQuery annotationFetcher;

    QMap<FeatureId, QPair<qint64, int> > featureAnnotationRows;
    QMap<FeatureId, qint64> featureObservationCount;
    qint64 totalFeatureObservationCount;
    QVector<QVariant> cachedCells;

    QSqlError error;
};

} // namespace qm

#endif // FEATURE_TABLE_MODEL_H
