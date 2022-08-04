#ifndef CALCULATIONS_H
#define CALCULATIONS_H

class FileDescriptor;
class Channel;
class Averaging;

#include <QList>


void calculateMean(FileDescriptor *file, const QList<Channel*> &channels);
void calculateMovingAvg(FileDescriptor *file, const QList<Channel *> &channels, int windowSize);
void calculateThirdOctave(FileDescriptor *file, FileDescriptor *source);
QString saveTimeSegment(FileDescriptor *file, double from, double to);
void saveSpectre(FileDescriptor *file, Channel* channel, double zValue);

Averaging *averageChannels(const QList<QPair<FileDescriptor *, int> > &toMean);

#endif // CALCULATIONS_H
