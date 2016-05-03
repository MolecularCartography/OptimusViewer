#ifndef GRAPHPOINT_H
#define GRAPHPOINT_H

#include <QPointF>
#include <QVariant>

#include "Globals.h"

namespace qm {

class GraphPoint: public QPointF
{
public:
    enum Attribute {
        PRECURSOR_MZ_ATTR,
        SCAN_START_TIME
    };

    GraphPoint(const SampleId &sampleId, const FeatureId &featureId, qreal x, qreal y, const QMap<Attribute, QVariant> &attrs = QMap<Attribute, QVariant>());

    const SampleId & getSampleId() const;
    const FeatureId & getFeatureId() const;

    bool hasAttribute(Attribute attr) const;
    QVariant getAttribute(Attribute attr) const;

private:
    SampleId sampleId;
    FeatureId featureId;

    QMap<Attribute, QVariant> attrs;
};

} // namespace qm

#endif // GRAPHPOINT_H
