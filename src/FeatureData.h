#ifndef FEATURE_DATA_H
#define FEATURE_DATA_H

#include <QPointF>
#include <QVector3D>

#include "Globals.h"

namespace ov {

struct FeatureData {
    FeatureData();
    FeatureData(const SampleId &sampleId, const FeatureId &featureId, const QList<QList<QVector3D> > &massTraces, qreal featureStart, qreal featureEnd);

    QList<QPointF> getXic() const; // Point: (RT, intensity)
    QList<QPointF> getMassPeaks() const; // Point: (mz, intensity)

    SampleId sampleId;
    FeatureId featureId;

    QList<QList<QVector3D> > massTraces;
    qreal featureStart;
    qreal featureEnd;
};

} // namespace qm

#endif // FEATURE_DATA_H
