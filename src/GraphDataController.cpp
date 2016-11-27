#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS // std::transform call cause a compiler warning on MSVC
#endif

#include <QApplication>
#include <QRegExp>
#include <QMessageBox>

#include "FeatureDataSource.h"
#include "GraphDescriptors.h"

#include "GraphDataController.h"

namespace ov {

GraphDataController::GraphDataController(FeatureDataSource *dataSource)
    : dataSource(dataSource)
{
    Q_ASSERT(NULL != dataSource);
}

QVariantMap GraphDataController::ms1graphDescriptionToMap(const Ms1GraphDescriptor &graphDescription) const
{
    QVariantMap result;
    result[getXFieldKey()] = graphDescription.getXField();
    result[getYFieldKey()] = graphDescription.getYField();
    result[getSampleNameGraphKey()] = graphDescription.sampleName;
    result[getConsensusMzGraphKey()] = graphDescription.consensusMz;
    result[getFeatureIdKey()] = graphDescription.featureId;
    result[getCompoundIdGraphKey()] = graphDescription.compoundIds;
    return result;
}

QVariantMap GraphDataController::xicGraphDescriptionToMap(const XicGraphDescriptor &graphDescription) const
{
    QVariantMap result = ms1graphDescriptionToMap(graphDescription);
    result[getFeatureStartGraphKey()] = graphDescription.rtStart;
    result[getFeatureEndGraphKey()] = graphDescription.rtEnd;
    return result;
}

QVariantMap GraphDataController::msngraphDescriptionToMap(const MsnGraphDescriptor &graphDescription) const
{
    QVariantMap result;
    result[getXFieldKey()] = graphDescription.getXField();
    result[getYFieldKey()] = graphDescription.getYField();
    return result;
}

void GraphDataController::addMs2ScanPointToGraph(const QPointF &nextXicPoint, const QList<Ms2ScanInfo> &ms2ScanPoints,
    const Ms1GraphDescriptor &graphDescription, int &nextMs2Index, bool &moreMs2Points, QVariantList &xicGraph)
{
    // Check if ms2 scan happened before @xicPoint, and if it did, add it to the plot
    if (moreMs2Points && ms2ScanPoints[nextMs2Index].scanTime < nextXicPoint.x()) {
        const Ms2ScanInfo *ms2Point = &ms2ScanPoints[nextMs2Index];
        do {
            QVariantMap variantPlotData;
            variantPlotData[graphDescription.getXField()] = ms2Point->scanTime;
            variantPlotData[graphDescription.getYField()] = ms2Point->precursorIntensity;
            variantPlotData[getPrecursorMzKey()] = ms2Point->precursorMz;
            variantPlotData[getSpectrumIdKey()] = ms2Point->spectrumId;
            variantPlotData[getGraphIdKey()] = graphDescription.graphId;

            xicGraph.append(variantPlotData);

            if (nextMs2Index < ms2ScanPoints.length() - 1) {
                ms2Point = &ms2ScanPoints[++nextMs2Index];
            } else {
                moreMs2Points = false;
            }
        } while (moreMs2Points && ms2Point->scanTime < nextXicPoint.x());
    }
}

void GraphDataController::featureSelectionChanged(const QMultiHash<SampleId, FeatureId> &newSelection, const QMap<FeatureId, qreal> &featureMzs)
{
    if (newSelection == currentFeatures) {
        return;
    }

    currentFeatures = newSelection;

    if (!dataSource->setActiveFeatures(newSelection)) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Too many features are selected simultaneously."));
    }

    const QList<FeatureData> &features = dataSource->getMs1Data();
    const QHash<SampleId, QHash<FeatureId, QList<Ms2ScanInfo> > > ms2ScanData = dataSource->getMs2ScanData();

    QVariantMap xicGraphDescriptions;
    QVariantList xicGraph;
    QVariantList ms1Graph;
    QVariantMap ms1GraphDescriptions;
    const QHash<FeatureId, QStringList> featureAnnotations = dataSource->getFeatureCompoundIds(newSelection.values().toSet());
    foreach (const FeatureData &fd, features) {
        // add XIC graph info
        XicGraphDescriptor xicGraphDescription(fd.sampleId, fd.featureId, dataSource->getSampleNameById(fd.sampleId), featureMzs[fd.featureId],
            featureAnnotations[fd.featureId], fd.featureStart, fd.featureEnd);
        xicGraphDescriptions[xicGraphDescription.graphId] = xicGraphDescriptionToMap(xicGraphDescription);

        const QList<QPointF> xicPoints = fd.getXic();
        const QList<Ms2ScanInfo> &ms2ScanPoints = ms2ScanData[fd.sampleId][fd.featureId];

        int nextMs2Index = 0;
        bool moreMs2Points = !ms2ScanPoints.isEmpty();
        for (int xicIndex = 0, xicCount = xicPoints.length(); xicIndex < xicCount; ++xicIndex) {
            const QPointF &xicPoint = xicPoints[xicIndex];

            addMs2ScanPointToGraph(xicPoint, ms2ScanPoints, xicGraphDescription, nextMs2Index, moreMs2Points, xicGraph);

            QVariantMap variantPlotData;
            variantPlotData[xicGraphDescription.getXField()] = xicPoint.x();
            variantPlotData[xicGraphDescription.getYField()] = xicPoint.y();
            variantPlotData[getGraphIdKey()] = xicGraphDescription.graphId;
            xicGraph.append(variantPlotData);
        }
        // ms2 scans after ms1 finished for this feature
        if (moreMs2Points) {
            const QPointF infinityPoint = QPointF(std::numeric_limits<qreal>::max(), 0.0);
            addMs2ScanPointToGraph(infinityPoint, ms2ScanPoints, xicGraphDescription, nextMs2Index, moreMs2Points, xicGraph);
        }

        // add mass peak graph info
        Ms1GraphDescriptor massGraphDescription(fd.sampleId, fd.featureId, dataSource->getSampleNameById(fd.sampleId),
            featureMzs[fd.featureId], featureAnnotations[fd.featureId]);
        ms1GraphDescriptions[massGraphDescription.graphId] = ms1graphDescriptionToMap(massGraphDescription);

        foreach (const QPointF &ms1Point, fd.getMassPeaks()) {
            QVariantMap variantPlotData;
            variantPlotData[massGraphDescription.getXField()] = ms1Point.x();
            variantPlotData[massGraphDescription.getYField()] = ms1Point.y();
            variantPlotData[getGraphIdKey()] = massGraphDescription.graphId;
            ms1Graph.append(variantPlotData);
        }
    }

    QVariantMap data;
    data[getXicGraphDescKey()] = xicGraphDescriptions;
    data[getXicGraphDataKey()] = xicGraph;
    data[getMs1GraphDescKey()] = ms1GraphDescriptions;
    data[getMs1GraphDataKey()] = ms1Graph;
    emit updatePlot(data);
}

QVariantMap GraphDataController::getMs2Spectra(const QVariantList &spectraIds) const
{
    QVector<FragmentationSpectrumId> tmpIds(spectraIds.size());
    std::transform(spectraIds.constBegin(), spectraIds.constEnd(), tmpIds.begin(), [] (const QVariant &v) { return v.value<FragmentationSpectrumId>(); });
    const QList<FragmentationSpectrumId> realIds = tmpIds.toList();

    QVariantList msnGraph;
    QVariantMap msnGraphDescriptions;
    const QHash<FragmentationSpectrumId, QList<QPointF> > graphPoints = dataSource->getMs2SpectraData(realIds);
    foreach (const FragmentationSpectrumId &specId, graphPoints.keys()) {
        MsnGraphDescriptor graphDescription(specId);
        msnGraphDescriptions[graphDescription.graphId] = msngraphDescriptionToMap(graphDescription);
        foreach (const QPointF &point, graphPoints[specId]) {
            QVariantMap variantPlotData;
            variantPlotData[graphDescription.getXField()] = point.x();
            variantPlotData[graphDescription.getYField()] = point.y();
            variantPlotData[getGraphIdKey()] = graphDescription.graphId;
            msnGraph.append(variantPlotData);
        }
    }

    QVariantMap data;
    data[getMsnGraphDescKey()] = msnGraphDescriptions;
    data[getMsnGraphDataKey()] = msnGraph;
    return data;
}

QString GraphDataController::getXFieldKey() const
{
    return "x";
}

QString GraphDataController::getYFieldKey() const
{
    return "y";
}

QString GraphDataController::getGraphIdKey() const
{
    return "graph_id";
}

QString GraphDataController::getPrecursorMzKey() const
{
    return "precursor_mz";
}

QString GraphDataController::getSpectrumIdKey() const
{
    return "spectrum_id";
}

QString GraphDataController::getFeatureIdKey() const
{
    return "feature_id";
}

QString GraphDataController::getSampleNameGraphKey() const
{
    return "sample_name";
}

QString GraphDataController::getConsensusMzGraphKey() const
{
    return "consensus_mz";
}

QString GraphDataController::getCompoundIdGraphKey() const
{
    return "compound_id";
}

QString GraphDataController::getFeatureStartGraphKey() const
{
    return "feature_start";
}

QString GraphDataController::getFeatureEndGraphKey() const
{
    return "feature_end";
}

QString GraphDataController::getXicGraphDescKey() const
{
    return "xic_graph_desc";
}

QString GraphDataController::getMs1GraphDescKey() const
{
    return "ms1_graph_desc";
}

QString GraphDataController::getMsnGraphDescKey() const
{
    return "msn_graph_desc";
}

QString GraphDataController::getXicGraphDataKey() const
{
    return "xic_graph_data";
}

QString GraphDataController::getMs1GraphDataKey() const
{
    return "ms1_graph_data";
}

Q_INVOKABLE QString GraphDataController::getMsnGraphDataKey() const
{
    return "msn_graph_data";
}

void GraphDataController::samplesChanged()
{
    currentFeatures.clear();
    QVariantMap emptyData;
    emptyData[getXicGraphDescKey()] = QVariantMap();
    emptyData[getXicGraphDataKey()] = QVariantList();
    emptyData[getMs1GraphDescKey()] = QVariantMap();
    emptyData[getMs1GraphDataKey()] = QVariantList();
    emit updatePlot(emptyData);
}

} // namespace ov
