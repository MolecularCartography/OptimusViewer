#ifndef GRAPHDATACONTROLLER_H
#define GRAPHDATACONTROLLER_H

#include <QMultiHash>
#include <QObject>
#include <QVariantMap>

#include "Globals.h"

namespace ov {

class FeatureDataSource;

class GraphDataController: public QObject
{
    Q_OBJECT

public:
    explicit GraphDataController(FeatureDataSource *dataSource);

    Q_INVOKABLE QVariantList getMs2Spectra(const QVariantList &scanTimesByScanKey) const;

signals:
    void updatePlot(const QVariantMap &data);

public slots:
    void samplesChanged();
    void featureSelectionChanged(const QMultiHash<SampleId, FeatureId> &newSelection, const QMap<FeatureId, qreal> &featureMzs);

private:
    QMultiHash<SampleId, FeatureId> currentFeatures;

    FeatureDataSource *dataSource;
};

} // namespace ov

#endif // GRAPHDATACONTROLLER_H
