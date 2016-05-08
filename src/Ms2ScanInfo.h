#ifndef MS2_SCAN_INFO_H
#define MS2_SCAN_INFO_H

#include "Globals.h"

namespace qm {

struct Ms2ScanInfo {
    Ms2ScanInfo(qreal scanTime, qreal precursorMz, const FragmentationSpectrumId &spectrumId);

    qreal scanTime;
    qreal precursorMz;
    FragmentationSpectrumId spectrumId;
};

} // namespace qm

#endif // MS2_SCAN_INFO_H
