#include "GraphPoint.h"

namespace qm {

GraphPoint::GraphPoint(const SampleId &sampleId, const FeatureId &featureId, qreal x, qreal y, const QMap<GraphPoint::Attribute, QVariant> &attrs)
    : QPointF(x, y), sampleId(sampleId), featureId(featureId), attrs(attrs)
{

}

const SampleId & GraphPoint::getSampleId() const
{
    return sampleId;
}

const FeatureId & GraphPoint::getFeatureId() const
{
    return featureId;
}

bool GraphPoint::hasAttribute(GraphPoint::Attribute attr) const
{
    return attrs.contains(attr);
}

QVariant GraphPoint::getAttribute(GraphPoint::Attribute attr) const
{
    return attrs.value(attr);
}

} // namespace qm
