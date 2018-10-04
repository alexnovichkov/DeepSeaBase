#include "abstractmethod.h"

QDebug operator<<(QDebug debug, const Parameters &p)
{
    QDebugStateSaver saver(debug);
    debug << "sampleRate"<< p.sampleRate<<endl;
    debug <<  "averagingType"<< p.averagingType<<endl;
    debug <<  "windowType"<< p.windowType<<endl;
    debug <<  "bufferSize"<< p.bufferSize<<endl;
    debug <<  "bandWidth"<< p.bandWidth<<endl;
    debug <<  "initialBandStripNumber"<< p.initialBandStripNumber<<endl;
    debug <<  "bandStrip"<< p.bandStrip<<endl;
    debug <<  "overlap"<< p.overlap<<endl;
    debug <<  "scaleType"<< p.scaleType<<endl;
    debug <<  "threshold"<< p.threshold<<endl;
    debug << "averagesCount"<< p.averagesCount<<endl;
    debug <<"baseChannel"<<   p.baseChannel<<endl;
    debug <<"fCount"<<   p.fCount<<endl;
    debug <<"useDeepSea"<<   p.useDeepSea<<endl;
    debug << "saveAsComplex"<< p.saveAsComplex<<endl;
      return debug;
}
