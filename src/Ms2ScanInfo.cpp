#include "Ms2ScanInfo.h"

namespace ov {

Ms2ScanInfo::Ms2ScanInfo(qreal scanTime, qreal precursorMz, qreal precursorIntensity, const FragmentationSpectrumId &spectrumId)
    : scanTime(scanTime), precursorMz(precursorMz), precursorIntensity(precursorIntensity), spectrumId(spectrumId)
{

}

} // namespace ov
