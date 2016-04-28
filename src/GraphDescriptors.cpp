#include "GraphDescriptors.h"

namespace ov {

//////////////////////////////////////////////////////////////////////////
/// BaseGraphDescriptor
//////////////////////////////////////////////////////////////////////////

BaseGraphDescriptor::BaseGraphDescriptor(const QString &graphId)
    : graphId(graphId)
{
    Q_ASSERT(!graphId.isEmpty());
}

QString BaseGraphDescriptor::getXField() const
{
    return QString("x_%1").arg(graphId);
}

QString BaseGraphDescriptor::getYField() const
{
    return QString("y_%1").arg(graphId);
}

//////////////////////////////////////////////////////////////////////////
/// Ms1GraphDescriptor
//////////////////////////////////////////////////////////////////////////

Ms1GraphDescriptor::Ms1GraphDescriptor(const SampleId &sampleId, const FeatureId &featureId, const QString &sampleName, qreal consensusMz)
    : BaseGraphDescriptor(QString("%1_%2").arg(sampleId).arg(featureId)), sampleId(sampleId), featureId(featureId), sampleName(sampleName), consensusMz(consensusMz)
{

}

//////////////////////////////////////////////////////////////////////////
/// XicGraphDescriptor
//////////////////////////////////////////////////////////////////////////

XicGraphDescriptor::XicGraphDescriptor(const SampleId &sampleId, const FeatureId &featureId, const QString &sampleName, qreal consensusMz, qreal rtStart, qreal rtEnd)
    : Ms1GraphDescriptor(sampleId, featureId, sampleName, consensusMz), rtStart(rtStart), rtEnd(rtEnd)
{

}

//////////////////////////////////////////////////////////////////////////
/// MsnGraphDescriptor
//////////////////////////////////////////////////////////////////////////

MsnGraphDescriptor::MsnGraphDescriptor(const FragmentationSpectrumId &specId)
    : BaseGraphDescriptor(QString::number(specId)), specId(specId)
{

}

} // namespace ov
