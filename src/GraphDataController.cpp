#include <QRegExp>

#include "FeatureDataSource.h"

#include "GraphDataController.h"

namespace ov {

GraphDataController::GraphDataController(FeatureDataSource *dataSource)
    : dataSource(dataSource)
{
    Q_ASSERT(NULL != dataSource);
}

namespace {

const QString MS2_SCAN_KEY_START = "rt_";
const QString MS2_SCAN_KEY_SEPARATOR = "_";

QString createMs2ScanKey(const SampleId &sampleId, const FeatureId &featureId)
{
    return QString("%1%2%3%4").arg(MS2_SCAN_KEY_START).arg(sampleId).arg(MS2_SCAN_KEY_SEPARATOR).arg(featureId);
}

bool isMs2ScanKeyValid(const QString &key)
{
    static QRegExp validator(QString("%1.+%2.+").arg(MS2_SCAN_KEY_START, MS2_SCAN_KEY_SEPARATOR));
    return validator.exactMatch(key);
}

bool parseMs2ScanKey(const QString &key, SampleId &sampleId, FeatureId &featureId)
{
    if (0 != key.indexOf(MS2_SCAN_KEY_START)) {
        return false;
    }

    const int prefixLen = MS2_SCAN_KEY_START.length();
    const int sepPos = key.indexOf(MS2_SCAN_KEY_SEPARATOR, prefixLen);
    if (-1 == sepPos) {
        return false;
    }

    sampleId = key.mid(prefixLen, sepPos - prefixLen).toLongLong();
    featureId = key.mid(sepPos + 1).toLongLong();

    return true;
}

const QString MS2_INT_KEY_START = "Sample: ";
const QString MS2_INT_KEY_SEPARATOR = "<br>Consensus mz: ";
const QString MS2_INT_KEY_END = "<br>";

QString createMs2IntKey(const QString &sampleName, qreal consensusMz)
{
    return QString("%1%2%3%4%5").arg(MS2_INT_KEY_START, sampleName, MS2_INT_KEY_SEPARATOR).arg(consensusMz).arg(MS2_INT_KEY_END);
}

bool isMs2IntKeyValid(const QString &key)
{
    static QRegExp validator(QString("%1.+%2.+%3").arg(MS2_INT_KEY_START, MS2_INT_KEY_SEPARATOR, MS2_INT_KEY_END));
    return validator.exactMatch(key);
}

bool parseMs2IntKey(const QString &key, QString &sampleName, qreal &consensusMz)
{
    if (0 != key.indexOf(MS2_INT_KEY_START)) {
        return false;
    }
    const int keyEndStartPos = key.length() - MS2_INT_KEY_END.length();
    if (keyEndStartPos != key.lastIndexOf(MS2_INT_KEY_END)) {
        return false;
    }

    const int prefixLen = MS2_INT_KEY_START.length();
    const int sepPos = key.indexOf(MS2_INT_KEY_SEPARATOR, prefixLen);
    if (-1 == sepPos) {
        return false;
    }

    sampleName = key.mid(prefixLen, sepPos - prefixLen);
    consensusMz = key.mid(sepPos + 1, keyEndStartPos - sepPos - 1).toDouble();

    return true;
}

}

void GraphDataController::featureSelectionChanged(const QMultiHash<SampleId, FeatureId> &newSelection, const QMap<FeatureId, qreal> &featureMzs)
{
    if (newSelection == currentFeatures) {
        return;
    }

    currentFeatures = newSelection;

    QVariantList xicGraph;
    const QList<GraphPoint> xicPoints = dataSource->getXicData(newSelection);
    const QList<GraphPoint> ms2Points = dataSource->getMs2ScanData(newSelection);
    int nextMs2Index = 0;
    bool moreMs2Points = !ms2Points.isEmpty();
    qreal lastIntensity = 0.0;
    QMap<SampleId, QMap<FeatureId, int> > lastIndexes;
    for (int xicIndex = 0, xicCount = xicPoints.length(); xicIndex < xicCount; ++xicIndex) {
        const GraphPoint &xicPoint = xicPoints[xicIndex];

        if (moreMs2Points && ms2Points[nextMs2Index].x() < xicPoint.x()) {
            GraphPoint ms2Point = ms2Points[nextMs2Index];
            do {
                const SampleId ms2Sample = ms2Point.getSampleId();
                const FeatureId ms2Feature = ms2Point.getFeatureId();
                const bool firstScan = !lastIndexes.contains(ms2Sample) || !lastIndexes[ms2Sample].contains(ms2Feature);
                const GraphPoint &prevXicPoint = firstScan ? GraphPoint(ms2Sample, ms2Feature, 0.0, 0.0) : xicPoints[lastIndexes[ms2Sample][ms2Feature]];

                QVariantMap variantPlotData;
                const QString rtKey = createMs2ScanKey(ms2Sample, ms2Feature);
                variantPlotData[rtKey] = ms2Point.x();
                const QString intKey = createMs2IntKey(sampleNameById[ms2Sample], featureMzs[ms2Feature]);

                GraphPoint nextXicPoint(ms2Sample, ms2Feature, std::numeric_limits<qreal>::max(), 0.0);
                for (int pos = xicIndex; pos < xicCount; ++pos) {
                    const GraphPoint &point = xicPoints[pos];
                    if (point.getSampleId() == ms2Sample && point.getFeatureId() == ms2Feature) {
                        nextXicPoint = point;
                        break;
                    }
                }

                const qreal intensityDelta = firstScan ? 0.0
                    : (nextXicPoint.y() - prevXicPoint.y()) * (ms2Point.x() - prevXicPoint.x()) / (nextXicPoint.x() - prevXicPoint.x());
                variantPlotData[intKey] = prevXicPoint.y() + intensityDelta;
                Q_ASSERT(ms2Point.hasAttribute(GraphPoint::PRECURSOR_MZ_ATTR));
                variantPlotData["precursorMz"] = ms2Point.getAttribute(GraphPoint::PRECURSOR_MZ_ATTR);
                xicGraph.append(variantPlotData);

                if (nextMs2Index < ms2Points.length() - 1) {
                    ms2Point = ms2Points[++nextMs2Index];
                } else {
                    moreMs2Points = false;
                }
            } while (moreMs2Points && ms2Point.x() < xicPoint.x());
        }

        QVariantMap variantPlotData;
        const QString rtKey = QString("rt_%1_%2").arg(xicPoint.getSampleId()).arg(xicPoint.getFeatureId());
        variantPlotData[rtKey] = xicPoint.x();
        const QString intKey = QString("Sample: %1<br>Consensus mz: %2<br>")
            .arg(sampleNameById[xicPoint.getSampleId()]).arg(featureMzs[xicPoint.getFeatureId()]);
        variantPlotData[intKey] = xicPoint.y();
        lastIndexes[xicPoint.getSampleId()][xicPoint.getFeatureId()] = xicIndex;
        xicGraph.append(variantPlotData);
    }

    QVariantList massPeakGraph;
    const QList<GraphPoint> isotopicPatternPoints = dataSource->getIsotopicPatternData(newSelection);
    foreach (const GraphPoint &point, isotopicPatternPoints) {
        QVariantMap variantPlotData;
        const QString mzKey = QString("mz_%1_%2").arg(point.getSampleId()).arg(point.getFeatureId());
        variantPlotData[mzKey] = point.x();
        const QString intKey = QString("Sample: %1<br>Consensus mz: %2<br>")
            .arg(sampleNameById[point.getSampleId()]).arg(featureMzs[point.getFeatureId()]);
        variantPlotData[intKey] = point.y();
        massPeakGraph.append(variantPlotData);
    }

    QVariantMap data;
    data["xic"] = xicGraph;
    data["isotopicPattern"] = massPeakGraph;
    emit updatePlot(data);
}

QVariantList GraphDataController::getMs2Spectra(const QVariantList &scanTimesByScanKey) const
{
    QVariantList result;
    QHash<SampleId, QMultiHash<FeatureId, qreal> > featureScansBySample;
    QHash<SampleId, QHash<FeatureId, QString> > intKeys;
    foreach (const QVariant &var, scanTimesByScanKey) {
        const QVariantMap scansForFeatureSample = var.toMap();
        const QString scanKey = scansForFeatureSample["scanKey"].toString();
        Q_ASSERT(isMs2ScanKeyValid(scanKey));

        SampleId sampleId = 0;
        FeatureId featureId = 0;
        if (!parseMs2ScanKey(scanKey, sampleId, featureId)) {
            Q_ASSERT(false);
            return result;
        }

        intKeys[sampleId][featureId] = scansForFeatureSample["intKey"].toString();
        foreach (const QVariant &var, scansForFeatureSample["scanTimes"].toList()) {
            featureScansBySample[sampleId].insert(featureId, var.toReal());
        }
    }

    const QList<GraphPoint> graphPoints = dataSource->getMs2SpectraData(featureScansBySample);
    foreach(const GraphPoint &point, graphPoints) {
        QVariantMap variantPlotData;
        Q_ASSERT(point.hasAttribute(GraphPoint::SCAN_START_TIME));
        variantPlotData["scanStartTime"] = point.getAttribute(GraphPoint::SCAN_START_TIME);
        Q_ASSERT(point.hasAttribute(GraphPoint::PRECURSOR_MZ_ATTR));
        variantPlotData["precursorMz"] = point.getAttribute(GraphPoint::PRECURSOR_MZ_ATTR);
        const QString mzKey = QString("mz_%1_%2_%3").arg(point.getSampleId(), point.getFeatureId()).arg(variantPlotData["scanStartTime"].toString());
        variantPlotData[mzKey] = point.x();
        const QString intKey = intKeys[point.getSampleId()][point.getFeatureId()];
        variantPlotData[intKey] = point.y();
        result.append(variantPlotData);
    }

    return result;
}

void GraphDataController::samplesChanged(const QMap<SampleId, QString> &nameById)
{
    currentFeatures.clear();
    sampleNameById = nameById;
    emit updatePlot(QVariantMap());
}

} // namespace ov
