#include "Ms2ScanInfo.h"

namespace qm {

Ms2ScanInfo::Ms2ScanInfo(qreal scanTime, qreal precursorMz, const FragmentationSpectrumId &spectrumId, const QString &scanId)
    : scanTime(scanTime), precursorMz(precursorMz), spectrumId(spectrumId), scanId(scanId)
{

}

} // namespace qm
