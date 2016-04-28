#include <functional>

#include "FeatureData.h"

namespace ov {

FeatureData::FeatureData()
    : sampleId(-1), featureId(-1), featureStart(-1), featureEnd(-1)
{

}

FeatureData::FeatureData(const SampleId &sampleId, const FeatureId &featureId, const QList<QList<QVector3D> > &massTraces, qreal featureStart, qreal featureEnd)
    : sampleId(sampleId), featureId(featureId), massTraces(massTraces), featureStart(featureStart), featureEnd(featureEnd)
{

}

namespace {

void addPointToGraph(const QPointF &point, QList<QPointF> &graph, const std::function<bool(const QPointF &, const QPointF &)> &lessThan)
{
    QList<QPointF>::iterator insertPos = std::upper_bound(graph.begin(), graph.end(), point, lessThan);
    QList<QPointF>::iterator prevPos = insertPos - 1;
    if (insertPos != graph.begin() && prevPos->x() == point.x()) {
        prevPos->setY(prevPos->y() + point.y());
    } else {
        graph.insert(insertPos, point);
    }
}

QList<QPointF> projectMassTraces2D(const QList<QList<QVector3D> > &massTraces, const std::function<QPointF(const QVector3D &)> &project)
{
    QList<QPointF> result;
    foreach(const QList<QVector3D> &massTrace, massTraces) {
        foreach(const QVector3D &point, massTrace) {
            addPointToGraph(project(point), result, [](const QPointF &p1, const QPointF &p2) { return p1.x() < p2.x(); });
        }
    }
    return result;
}

}

QList<QPointF> FeatureData::getXic() const
{
    return projectMassTraces2D(massTraces, [] (const QVector3D &p) { return QPointF(p.y(), p.z()); });
}

QList<QPointF> FeatureData::getMassPeaks() const
{
    return projectMassTraces2D(massTraces, [](const QVector3D &p) { return QPointF(p.x(), p.z()); });
}

} // namespace ov
