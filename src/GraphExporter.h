#ifndef GRAPHEXPORTER_H
#define GRAPHEXPORTER_H

#include <QImage>
#include <QObject>
#include <QVariantList>
#include <QWebElement>

#include "Globals.h"

class QWebView;

namespace qm {

class GraphExporter : public QObject
{
    Q_OBJECT

    Q_PROPERTY(GraphId xicChartId READ getXicChartId)
    Q_PROPERTY(GraphId massPeakChartId READ getMassPeakChartId)
    Q_PROPERTY(QVariantList supportedImageFormatIds READ getSupportedImageFormatIds)
    Q_PROPERTY(QVariantList supportedDataFormatIds READ getSupportedDataFormatIds)

public:
    explicit GraphExporter(QWebView *graphView = NULL);

    GraphId getXicChartId() const;
    GraphId getMassPeakChartId() const;

    QVariantList getSupportedImageFormatIds() const;
    QVariantList getSupportedDataFormatIds() const;

    Q_INVOKABLE void exportGraph(const GraphId &graphId, const FormatId &formatId, const QVariantList &graphPoints);

    void setGraphView(QWebView *view);

private:
    QWebView *graphView;

    QWebElement getGraphWebElement(const GraphId &id) const;
    QWebElement getLegendWebElement(const GraphId &id) const;
    void saveGraphAsImage(const GraphId &id, const FormatId &formatId, const QString &path, int quality, double scale) const;
    void saveGraphAsSvg(const GraphId &id, const QString &path, double scale) const;
    void saveGraphAsPdf(const GraphId &id, const QString &path, double scale) const;
    void saveGraphAsCsv(const GraphId &id, const QString &path, const QVariantList &graphPoints) const;

    static bool isDataFormat(const QString &id);
    static bool isImageFormat(const QString &id);

    static const QList<FormatId> supportedImageFormatIds;
    static const QList<FormatId> supportedDataFormatIds;
};

} // namespace qm

#endif // GRAPHEXPORTER_H
