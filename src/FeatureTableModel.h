#ifndef FEATURE_TABLE_MODEL_H
#define FEATURE_TABLE_MODEL_H

#include <QAbstractTableModel>
#include <QSqlError>
#include <QSqlQuery>

#include "Globals.h"

namespace ov {

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

    void reset();
    QSqlError lastError() const;

    SampleId FeatureTableModel::getSampleIdByColumnNumber(int column) const;

private:
    void updateRowNumber();
    void updateColumnNumber();
    QVariant dataInternal(const QModelIndex &index, int role);
    QVariant getQueryResultValue(QSqlQuery &query, const QVector<QVariant> &parameters, int fieldNumber);

    int rowNumber;
    int columnNumber;
    FeatureDataSource *dataSource;

    QSqlQuery consensusFeatureFetcher;
    QSqlQuery intensityFetcher;

    QSqlError error;
};

} // namespace ov

#endif // FEATURE_TABLE_MODEL_H
