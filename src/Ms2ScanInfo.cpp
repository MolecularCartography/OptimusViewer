#include "Ms2ScanInfo.h"

namespace ov {

Ms2ScanInfo::Ms2ScanInfo(qreal scanTime, qreal precursorMz, const FragmentationSpectrumId &spectrumId)
    : scanTime(scanTime), precursorMz(precursorMz), spectrumId(spectrumId)
{

}

} // namespace ov
