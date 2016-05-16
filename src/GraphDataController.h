#ifndef GRAPHDATACONTROLLER_H
#define GRAPHDATACONTROLLER_H

#include <QMultiHash>
#include <QObject>
#include <QVariantMap>

#include "Ms2ScanInfo.h"

namespace ov {

class FeatureDataSource;
struct Ms1GraphDescriptor;
struct MsnGraphDescriptor;
struct XicGraphDescriptor;

class GraphDataController: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString xFieldKey READ getXFieldKey)
    Q_PROPERTY(QString yFieldKey READ getYFieldKey)
    Q_PROPERTY(QString graphIdKey READ getGraphIdKey)

    Q_PROPERTY(QString precursorMzKey READ getPrecursorMzKey)
    Q_PROPERTY(QString spectrumIdKey READ getSpectrumIdKey)
    Q_PROPERTY(QString featureIdKey READ getFeatureIdKey)

    Q_PROPERTY(QString sampleNameGraphKey READ getSampleNameGraphKey)
    Q_PROPERTY(QString consensusMzGraphKey READ getConsensusMzGraphKey)

    Q_PROPERTY(QString featureStartGraphKey READ getFeatureStartGraphKey)
    Q_PROPERTY(QString featureEndGraphKey READ getFeatureEndGraphKey)

    Q_PROPERTY(QString xicGraphDescKey READ getXicGraphDescKey)
    Q_PROPERTY(QString ms1GraphDescKey READ getMs1GraphDescKey)
    Q_PROPERTY(QString msnGraphDescKey READ getMsnGraphDescKey)

    Q_PROPERTY(QString xicGraphDataKey READ getXicGraphDataKey)
    Q_PROPERTY(QString ms1GraphDataKey READ getMs1GraphDataKey)
    Q_PROPERTY(QString msnGraphDataKey READ getMsnGraphDataKey)

public:
    explicit GraphDataController(FeatureDataSource *dataSource);

    Q_INVOKABLE QVariantMap getMs2Spectra(const QVariantList &spectraIds) const;

    QString getXFieldKey() const;
    QString getYFieldKey() const;
    QString getGraphIdKey() const;

    QString getPrecursorMzKey() const;
    QString getSpectrumIdKey() const;
    QString getFeatureIdKey() const;

    QString getSampleNameGraphKey() const;
    QString getConsensusMzGraphKey() const;

    QString getFeatureStartGraphKey() const;
    QString getFeatureEndGraphKey() const;

    QString getXicGraphDescKey() const;
    QString getMs1GraphDescKey() const;
    QString getMsnGraphDescKey() const;

    QString getXicGraphDataKey() const;
    QString getMs1GraphDataKey() const;
    QString getMsnGraphDataKey() const;

signals:
    void updatePlot(const QVariantMap &data);
    void resetActiveFeatures();

public slots:
    void samplesChanged();
    void featureSelectionChanged(const QMultiHash<SampleId, FeatureId> &newSelection, const QMap<FeatureId, qreal> &featureMzs);

private:
    QVariantMap ms1graphDescriptionToMap(const Ms1GraphDescriptor &graphDescription) const;
    QVariantMap xicGraphDescriptionToMap(const XicGraphDescriptor &graphDescription) const;
    QVariantMap msngraphDescriptionToMap(const MsnGraphDescriptor &graphDescription) const;
    void addMs2ScanPointToGraph(const QPointF &prevXicPoint, const QPointF &nextXicPoint, bool isBeforeMs1, bool isAfterMs1, const QList<Ms2ScanInfo> &ms2ScanPoints,
        const Ms1GraphDescriptor &graphDescription, int &nextMs2Index, bool &moreMs2Points, QVariantList &xicGraph);

    QMultiHash<SampleId, FeatureId> currentFeatures;

    FeatureDataSource *dataSource;
};

} // namespace qm

#endif // GRAPHDATACONTROLLER_H
