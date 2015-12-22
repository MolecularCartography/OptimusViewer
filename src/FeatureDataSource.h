#ifndef FEATUREDATASOURCE_H
#define FEATUREDATASOURCE_H

#include <QObject>
#include <QSqlDatabase>

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

signals:
    void samplesChanged(const QMap<SampleId, QString> &sampleNameById);

public slots:
    void selectDataSource();

private:
    bool setDataSource(const DataSourceId &dataSourceId);
    QMap<SampleId, QString> getSampleNameById();
    DataSourceId currentDataSourceId() const;

    static QString getInputFileFilter();

    QSqlDatabase db;
};

} // namespace ov

#endif // FEATUREDATASOURCE_H
