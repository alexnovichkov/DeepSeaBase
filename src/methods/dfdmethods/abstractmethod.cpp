#include "abstractmethod.h"
#include "fileformats/dfdfiledescriptor.h"

QDebug operator<<(QDebug debug, const Parameters &p)
{
    QDebugStateSaver saver(debug);
    debug << "sampleRate"<< p.sampleRate<<endl;
    debug <<  "averagingType"<< p.averagingType<<endl;
    debug <<  "windowType"<< p.windowType<<endl;
    debug <<  "windowPercent"<< p.windowPercent<<endl;
    debug <<  "forceWindowType"<< p.forceWindowType<<endl;
    debug <<  "forceWindowPercent"<< p.forceWindowPercent<<endl;
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

/*DfdFileDescriptor *AbstractMethod::createNewDfdFile(const QString &fileName, FileDescriptor *dfd, Parameters &p)
{
    DfdFileDescriptor *newDfd = DfdFileDescriptor::newFile(fileName, DfdDataType(dataType()));
    newDfd->BlockSize = 0;

    // [DataDescription]
    newDfd->setDataDescription(dfd->dataDescription());
    QMap<QString, QString> info; //= dfd->info();
    //newDfd->DescriptionFormat = info.value("descriptionFormat");

    // [Sources]
//    newDfd->source = new Source();
//    QStringList l; for (int i=1; i<=dfd->channelsCount(); ++i) l << QString::number(i);
//    newDfd->source->sFile = dfd->fileName()+"["+l.join(",")+"]"+info.value("guid");

    // [Process]
//    newDfd->process = new Process();
//    newDfd->process->data = processData(p);

    return newDfd;
}*/
