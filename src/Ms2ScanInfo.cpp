#include "Ms2ScanInfo.h"

namespace ov {

Ms2ScanInfo::Ms2ScanInfo(qreal scanTime, qreal precursorMz, const FragmentationSpectrumId &scanId)
    : scanTime(scanTime), precursorMz(precursorMz), scanId(scanId)
{

}

} // namespace ov
