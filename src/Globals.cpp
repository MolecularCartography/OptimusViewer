#include "Globals.h"

namespace ov {

namespace GraphIds {

const GraphId XIC_ID = "XIC";
const GraphId MASS_PEAK_ID = "MassPeak";

QString getHtmlContainerIdForGraph(const GraphId &id)
{
    if (id == XIC_ID) {
        return "xic_div";
    } else if (id == MASS_PEAK_ID) {
        return "mass_peak_div";
    } else {
        Q_ASSERT(false);
        return QString();
    }
}

}

namespace ExportFormats {

const QList<FormatId> lossyImageFormats = QList<FormatId>() << "PNG" << "JPG";
const QList<FormatId> losslessImageFormats = QList<FormatId>() << "BMP";
const QList<FormatId> vectorImageFormats = QList<FormatId>() << "PDF" << "SVG";
const QList<FormatId> dataFormats = QList<FormatId>() << "CSV";

}

} // namespace
