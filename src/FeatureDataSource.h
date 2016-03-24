#ifndef FEATUREDATASOURCE_H
#define FEATUREDATASOURCE_H

#include <QObject>
#include <QSqlDatabase>
#include <QVector>

#include "Globals.h"
#include "GraphPoint.h"

namespace ov {

class FeatureDataSource: public QObject
{
    Q_OBJECT

public:
    FeatureDataSource();

    bool isValid() const;
    QList<GraphPoint> getXicData(const QMultiHash<SampleId, FeatureId> &featuresBySample) const;
    QList<GraphPoint> getMs2ScanData(const QMultiHash<SampleId, FeatureId> &featuresBySample) const;
    QList<GraphPoint> getMs2SpectraData(const QHash<SampleId, QMultiHash<FeatureId, qreal> > &featureScansBySample) const;
    QList<GraphPoint> getIsotopicPatternData(const QMultiHash<SampleId, FeatureId> &featuresBySample) const;

    SampleId getSampleIdByNumber(int number) const;
    QString getSampleNameById(const SampleId &id) const;
    int getSampleCount() const;

signals:
    void samplesChanged();

public slots:
    void selectDataSource();

private:
    bool setDataSource(const DataSourceId &dataSourceId);
    DataSourceId currentDataSourceId() const;
    void updateSamplesInfo();

    static QString getInputFileFilter();

    QMap<SampleId, QString> sampleNameById;
    QVector<SampleId> sampleIds;

    QSqlDatabase db;
};

} // namespace ov

#endif // FEATUREDATASOURCE_H
