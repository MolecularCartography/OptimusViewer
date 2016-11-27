#ifndef MS2_SCAN_INFO_H
#define MS2_SCAN_INFO_H

#include "Globals.h"

namespace ov {

struct Ms2ScanInfo {
    Ms2ScanInfo(qreal scanTime, qreal precursorMz, qreal precursorIntensity, const FragmentationSpectrumId &spectrumId);

    qreal scanTime;
    qreal precursorMz;
    qreal precursorIntensity;
    FragmentationSpectrumId spectrumId;
};

} // namespace ov

#endif // MS2_SCAN_INFO_H
