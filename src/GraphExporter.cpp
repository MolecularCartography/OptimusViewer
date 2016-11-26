#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QMessageBox>
#include <QMimeData>
#include <QPrinter>
#include <QScopedPointer>
#include <QSvgGenerator>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>

#include "CsvWritingUtils.h"
#include "SaveGraphDialog.h"

#include "GraphExporter.h"

namespace ov {

const QList<FormatId> GraphExporter::supportedImageFormatIds = QStringList() << ExportFormats::lossyImageFormats
    << ExportFormats::losslessImageFormats << ExportFormats::resizableVectorImageFormats << ExportFormats::fixedSizeVectorImageFormats;
const QList<FormatId> GraphExporter::supportedDataFormatIds = QStringList() << ExportFormats::dataFormats;

namespace {

template<typename T>
QVariantList toVariantList(const QList<T> &l)
{
    QVariantList result;
    foreach(const T &i, l) {
        result.append(i);
    }
    return result;
}

}

GraphExporter::GraphExporter(QWebView *graphView)
    : graphView(graphView)
{

}

QString GraphExporter::getXicChartId() const
{
    return GraphIds::XIC_ID;
}

QString GraphExporter::getMassPeakChartId() const
{
    return GraphIds::MASS_PEAK_ID;
}

QVariantList GraphExporter::getSupportedImageFormatIds() const
{
    return toVariantList(supportedImageFormatIds);
}

QVariantList GraphExporter::getSupportedDataFormatIds() const
{
    return toVariantList(supportedDataFormatIds);
}

bool GraphExporter::isDataFormat(const QString &id)
{
    return supportedDataFormatIds.contains(id);
}

bool GraphExporter::isImageFormat(const QString &id)
{
    return supportedImageFormatIds.contains(id);
}

void GraphExporter::setGraphView(QWebView *view)
{
    Q_ASSERT(NULL != view);
    graphView = view;
}

QWebElement GraphExporter::getGraphWebElement(const GraphId &id) const
{
    Q_ASSERT(NULL != graphView);
    return graphView->page()->mainFrame()->findFirstElement(QString("#%1 .amcharts-chart-div svg").arg(GraphIds::getHtmlContainerIdForGraph(id)));
}

QWebElement GraphExporter::getLegendWebElement(const GraphId &id) const
{
    Q_ASSERT(NULL != graphView);
    QWebElement legendElement = graphView->page()->mainFrame()->findFirstElement(QString("#%1 .amcharts-legend-div svg").arg(GraphIds::getHtmlContainerIdForGraph(id)));
    if (legendElement.isNull()) {
        legendElement = graphView->page()->mainFrame()->findFirstElement(QString("#legend_container svg"));
        Q_ASSERT(!legendElement.isNull());
    }
    return legendElement;
}

void GraphExporter::saveGraphAsImage(const GraphId &id, const FormatId &formatId, const QString &path, int quality, double scale) const
{
    QWebElement graphElement = getGraphWebElement(id);
    QWebElement legendElement = getLegendWebElement(id);
    const QRect graphGeometry = graphElement.geometry();
    const QRect legendGeometry = legendElement.geometry();

    const double legendScalingFactor = double(graphGeometry.width()) / legendGeometry.width();
    QImage image(graphGeometry.width() * scale, (graphGeometry.height() + legendGeometry.height() * legendScalingFactor) * scale, QImage::Format_ARGB32_Premultiplied);

    image.fill(Qt::white);
    QPainter painter(&image);
    painter.scale(scale, scale);
    graphElement.render(&painter);
    painter.translate(QPoint(0, graphGeometry.height()));
    painter.scale(legendScalingFactor, legendScalingFactor);
    legendElement.render(&painter);

    if (!path.isEmpty() && !image.save(path, formatId.toLocal8Bit().constData(), quality)) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Unable to save the file. Perhaps, it is being used by another process."));
    } else if (path.isEmpty()) {
        QClipboard *clipboard = QApplication::clipboard();
        QMimeData *mimeData = new QMimeData();
        QByteArray data;
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        buffer.close();
        mimeData->setData("PNG", data);
        clipboard->setMimeData(mimeData);
    }
}

void GraphExporter::saveGraphAsSvg(const GraphId &id, const QString &path, double scale) const
{
    QWebElement graphElement = getGraphWebElement(id);
    QWebElement legendElement = getLegendWebElement(id);
    const QRect graphGeometry = graphElement.geometry();
    const QRect legendGeometry = legendElement.geometry();

    QSvgGenerator generator;
    generator.setFileName(path);
    const double legendScalingFactor = double(graphGeometry.width()) / legendGeometry.width();
    generator.setSize(QSize(graphGeometry.width() * scale,
        (graphGeometry.height() + legendGeometry.height() * legendScalingFactor) * scale));

    generator.setTitle(tr("Snapshot by OptimusViewer"));
    generator.setDescription(tr("An image of a plot created by OptimusViewer software, LC-MS data visualization tool."));

    QPainter painter(&generator);
    painter.scale(scale, scale);
    graphElement.render(&painter);
    painter.translate(QPoint(0, graphGeometry.height()));
    painter.scale(legendScalingFactor, legendScalingFactor);
    legendElement.render(&painter);
}

void GraphExporter::saveGraphAsPdf(const GraphId &id, const QString &path) const
{
    QWebElement graphElement = getGraphWebElement(id);
    QWebElement legendElement = getLegendWebElement(id);

    const QRect graphGeometry = graphElement.geometry();
    const QRect legendGeometry = legendElement.geometry();
    const double legendScalingFactor = double(graphGeometry.width()) / legendGeometry.width();

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPrinter::A4);
    printer.setOutputFileName(path);

    QPainter painter(&printer);
    if (QPrinter::Error == printer.printerState()) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Unable to save the file. Perhaps, it is being used by another process."));
        return;
    }

    const qreal pageWidth = printer.pageLayout().pageSize().rect(QPageSize::Inch).width();
    const qreal pageScale = pageWidth * printer.resolution() / graphGeometry.width();

    painter.scale(pageScale, pageScale);
    graphElement.render(&painter);
    painter.translate(QPoint(0, graphGeometry.height()));
    painter.scale(legendScalingFactor, legendScalingFactor);
    legendElement.render(&painter);
}

void GraphExporter::saveGraphAsCsv(const GraphId &id, const QString &path, const QVariantList &graphPoints) const
{
    if (graphPoints.isEmpty()) {
        return;
    }

    int numberOfNextValueColumn = 1;
    QString xFieldColumnName;
    if (id == GraphIds::XIC_ID) {
        xFieldColumnName = "RT";
    } else if (id == GraphIds::MASS_PEAK_ID) {
        xFieldColumnName = "m/z";
    } else {
        Q_ASSERT(false);
    }
    QMap<QString, int> columnNumberByName;
    QVariantList graphPointsSortedByX = graphPoints;
    std::sort(graphPointsSortedByX.begin(), graphPointsSortedByX.end(),
        [](const QVariant &first, const QVariant &second) { return first.toMap()["x"].toReal() < second.toMap()["x"].toReal(); });

    foreach (const QVariant &varPoint, graphPointsSortedByX) {
        const QVariantMap mapPoint = varPoint.toMap();
        const QString yField = mapPoint["y"].toString();

        if (!columnNumberByName.contains(yField)) {
            columnNumberByName[yField] = numberOfNextValueColumn++;
        }
    }

    QList<QStringList> resultTable = CsvWritingUtils::createEmptyTable(graphPointsSortedByX.length() + 1, numberOfNextValueColumn);
    resultTable[0][0] = xFieldColumnName;
    for (int i = 1; i < numberOfNextValueColumn; ++i) {
        QString columnName = columnNumberByName.key(i);
        if (columnName.contains(",")) {
            columnName = QString("\"%1\"").arg(columnName); // escape commas
        }
        resultTable[0][i] = columnName;
    }

    QMap<QString, int> rowNumberByXValue;
    int currentRow = 0;
    foreach (const QVariant &varPoint, graphPointsSortedByX) {
        const QVariantMap mapPoint = varPoint.toMap();
        const QString yField = mapPoint["y"].toString();
        const int columnNumber = columnNumberByName[yField];

        const QString xValue = mapPoint["x"].toString();

        int row = -1;
        if (!rowNumberByXValue.contains(xValue)) {
            rowNumberByXValue[xValue] = ++currentRow;
            row = currentRow;
            resultTable[row][0] = xValue;
        } else {
            row = rowNumberByXValue[xValue];
        }
        Q_ASSERT(-1 != row);
        resultTable[row][columnNumber] = mapPoint[yField].toString();
    }

    if (!CsvWritingUtils::saveTableToFile(resultTable, path)) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Unable to save file: %1").arg(path));
    }
}

void GraphExporter::exportGraph(const QString &graphId, const FormatId &initialFormatId, const QVariantList &graphPoints)
{
    if (initialFormatId == "Clipboard") {
        saveGraphAsImage(graphId, initialFormatId, QString(), 100, 1);
        return;
    }

    QScopedPointer<SaveGraphDialog> saveDialog(new SaveGraphDialog(QApplication::activeWindow(), initialFormatId));
    QString path;
    FormatId finalFormatId;
    if (saveDialog->exec()) {
        path = saveDialog->selectedFiles().first();
        finalFormatId = saveDialog->getSelectedFormat();
        if (QFileInfo(path).suffix().isEmpty()) {
            path += QString(".%1").arg(finalFormatId.toLower());
        }
    } else {
        return;
    }
    if (isImageFormat(finalFormatId)) {
        if (finalFormatId == "SVG") {
            saveGraphAsSvg(graphId, path, saveDialog->getScale());
        } else if (finalFormatId == "PDF") {
            saveGraphAsPdf(graphId, path);
        } else {
            saveGraphAsImage(graphId, finalFormatId, path, saveDialog->getQuality(), saveDialog->getScale());
        }
    } else if (isDataFormat(initialFormatId)) {
        if (finalFormatId == "CSV") {
            saveGraphAsCsv(graphId, path, graphPoints);
        } else {
            Q_ASSERT(false);
        }
    } else {
        Q_ASSERT(false);
    }
}


} // namespace ov
