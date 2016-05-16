#ifndef FEATUREDATASOURCE_H
#define FEATUREDATASOURCE_H

#include <QObject>
#include <QSqlDatabase>
#include <QVector>

#include "Globals.h"
#include "GraphPoint.h"
#include "FeatureData.h"
#include "Ms2ScanInfo.h"

namespace ov {

class FeatureDataSource: public QObject
{
    Q_OBJECT

public:
    FeatureDataSource();

    bool isValid() const;

    bool setActiveFeatures(const QMultiHash<SampleId, FeatureId> &featuresBySample);

    QList<FeatureData> getMs1Data() const;
    QHash<SampleId, QHash<FeatureId, QList<Ms2ScanInfo> > > getMs2ScanData() const;
    QHash<FragmentationSpectrumId, QList<QPointF> > getMs2SpectraData(const QList<FragmentationSpectrumId> &spectrumIds) const; // Point: (mz, intensity)

    SampleId getSampleIdByNumber(int number) const;
    QString getSampleNameById(const SampleId &id) const;
    qint64 getSampleCount() const;

    FeatureId getFeatureIdByNumber(int number) const;
    qint64 getFeatureCount() const;

signals:
    void samplesChanged();

public slots:
    void selectDataSource();

private:
    bool setDataSource(const DataSourceId &dataSourceId);
    DataSourceId currentDataSourceId() const;
    void updateSamplesInfo();
    void updateFeaturesInfo();
    QMultiHash<SampleId, FeatureId> getFeaturesToExtract(const QMultiHash<SampleId, FeatureId> &featuresBySample,
        QHash<SampleId, QHash<FeatureId, FeatureData> > &presentFeatures, QHash<SampleId, QHash<FeatureId, QList<Ms2ScanInfo> > > &presentMs2Scans);
    void fetchFeatures(const QMultiHash<SampleId, FeatureId> &featureIdsToExtract, QHash<SampleId, QHash<FeatureId, FeatureData> > &features);
    void fetchMs2Scans(const QMultiHash<SampleId, FeatureId> &featuresToExtract, QHash<SampleId, QHash<FeatureId, QList<Ms2ScanInfo> > > &ms2Scans);

    static QString getInputFileFilter();

    QMap<SampleId, QString> sampleNameById;
    QVector<SampleId> sampleIds;
    QVector<FeatureId> featureIds;
    QHash<SampleId, QHash<FeatureId, FeatureData> > currentFeatures;
    QHash<SampleId, QHash<FeatureId, QList<Ms2ScanInfo> > > currentMs2Scans;

    QSqlDatabase db;
};

} // namespace qm

#endif // FEATUREDATASOURCE_H
