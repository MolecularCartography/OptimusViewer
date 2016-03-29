#include <QApplication>
#include <QMessageBox>
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
    << ExportFormats::losslessImageFormats << ExportFormats::vectorImageFormats;
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

QString GraphExporter::getXicGraphId() const
{
    return GraphIds::XIC_ID;
}

QString GraphExporter::getMassPeakGraphId() const
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
    return graphView->page()->mainFrame()->findFirstElement(QString("#%1 svg").arg(GraphIds::getHtmlContainerIdForGraph(id)));
}

void GraphExporter::saveGraphAsImage(const GraphId &id, const FormatId &formatId, const QString &path, int quality, double scale) const
{
    QWebElement graphElement = getGraphWebElement(id);
    const QRect graphGeometry = graphElement.geometry();

    QImage image(graphGeometry.width() * scale, graphGeometry.height() * scale, QImage::Format_ARGB32_Premultiplied);

    image.fill(Qt::white);
    QPainter painter(&image);
    painter.scale(scale, scale);
    graphElement.render(&painter);

    if (!image.save(path, formatId.toLocal8Bit().constData(), quality)) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Unable to save the file. Perhaps, it is being used by another process."));
    }
}

void GraphExporter::saveGraphAsSvg(const GraphId &id, const QString &path, double scale) const
{
    QWebElement graphElement = getGraphWebElement(id);

    QSvgGenerator generator;
    generator.setFileName(path);
    generator.setSize(graphElement.geometry().size());

    generator.setTitle(tr("OptimusViewer SVG Generator"));
    generator.setDescription(tr("An image of a plot created by OptimusViewer software, LC-MS data visualization tool."));

    QPainter painter(&generator);
    painter.scale(scale, scale);
    graphElement.render(&painter);
}

void GraphExporter::saveGraphAsPdf(const GraphId &id, const QString &path, double scale) const
{
    QWebElement graphElement = getGraphWebElement(id);

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPrinter::A4);
    printer.setOutputFileName(path);

    QPainter painter(&printer);
    if (QPrinter::Error == printer.printerState()) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Unable to save the file. Perhaps, it is being used by another process."));
        return;
    }

    painter.scale(scale, scale);
    graphElement.render(&painter);
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
    foreach (const QVariant &varPoint, graphPoints) {
        const QVariantMap mapPoint = varPoint.toMap();
        const QString yField = mapPoint["y"].toString();

        if (!columnNumberByName.contains(yField)) {
            columnNumberByName[yField] = numberOfNextValueColumn++;
        }
    }

    QList<QStringList> resultTable = CsvWritingUtils::createEmptyTable(graphPoints.length() + 1, numberOfNextValueColumn);
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
    foreach (const QVariant &varPoint, graphPoints) {
        const QVariantMap mapPoint = varPoint.toMap();
        const QString xField = mapPoint["x"].toString();
        const QString yField = mapPoint["y"].toString();
        const int columnNumber = columnNumberByName[yField];

        const QString xValue = mapPoint[xField].toString();

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
            saveGraphAsPdf(graphId, path, saveDialog->getScale());
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
