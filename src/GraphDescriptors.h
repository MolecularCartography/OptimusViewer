#ifndef GRAPH_DESCRIPTORS_H
#define GRAPH_DESCRIPTORS_H

#include "Globals.h"

namespace ov {

//////////////////////////////////////////////////////////////////////////
/// BaseGraphDescriptor
//////////////////////////////////////////////////////////////////////////

struct BaseGraphDescriptor
{
    BaseGraphDescriptor(const QString &graphId);

    virtual QString getXField() const;
    virtual QString getYField() const;

    const QString graphId;
};

//////////////////////////////////////////////////////////////////////////
/// Ms1GraphDescriptor
//////////////////////////////////////////////////////////////////////////

struct Ms1GraphDescriptor: public BaseGraphDescriptor
{
    Ms1GraphDescriptor(const SampleId &sampleId, const FeatureId &featureId, const QString &sampleName, qreal consensusMz, const QStringList &compoundIds);

    const SampleId sampleId;
    const FeatureId featureId;
    const QString sampleName;
    qreal consensusMz;
    const QStringList compoundIds;
};

//////////////////////////////////////////////////////////////////////////
/// XicGraphDescriptor
//////////////////////////////////////////////////////////////////////////

struct XicGraphDescriptor : public Ms1GraphDescriptor
{
    XicGraphDescriptor(const SampleId &sampleId, const FeatureId &featureId, const QString &sampleName,
        qreal consensusMz, const QStringList &compoundIds, qreal rtStart, qreal rtEnd);

    const qreal rtStart;
    const qreal rtEnd;
};

//////////////////////////////////////////////////////////////////////////
/// MsnGraphDescriptor
//////////////////////////////////////////////////////////////////////////

struct MsnGraphDescriptor: public BaseGraphDescriptor
{
    MsnGraphDescriptor(const FragmentationSpectrumId &specId);

    const FragmentationSpectrumId specId;
};

} // namespace ov

#endif // GRAPH_DESCRIPTORS_H
