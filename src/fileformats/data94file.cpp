#include "data94file.h"

Data94File::Data94File(const QString &fileName) : FileDescriptor(fileName)
{

}

Data94File::Data94File(const Data94File &d) : FileDescriptor(d.fileName())
{
    this->description = d.description;
    foreach (Data94Channel *f, d.channels) {
        this->channels << new Data94Channel(*f);
    }
//    foreach (Data94Channel *f, channels) {
//        f->parent = this;
    //    }
}

Data94File::Data94File(const FileDescriptor &other) : FileDescriptor(other.fileName())
{
    //TODO: доделать создание файла
}


void Data94File::fillPreliminary(Descriptor::DataType)
{
}

void Data94File::fillRest()
{
}

void Data94File::read()
{
}

void Data94File::write()
{
}

void Data94File::writeRawFile()
{
}

void Data94File::updateDateTimeGUID()
{
}

Descriptor::DataType Data94File::type() const
{
}

QString Data94File::typeDisplay() const
{
}

DescriptionList Data94File::dataDescriptor() const
{
}

void Data94File::setDataDescriptor(const DescriptionList &data)
{
}

QString Data94File::dataDescriptorAsString() const
{
}

QDateTime Data94File::dateTime() const
{
}

void Data94File::deleteChannels(const QVector<int> &channelsToDelete)
{
}

void Data94File::copyChannelsFrom(FileDescriptor *, const QVector<int> &)
{
}

void Data94File::calculateMean(const QList<QPair<FileDescriptor *, int> > &channels)
{
}

QString Data94File::calculateThirdOctave()
{
}

void Data94File::calculateMovingAvg(const QList<QPair<FileDescriptor *, int> > &channels, int windowSize)
{
}

QString Data94File::saveTimeSegment(double from, double to)
{
}

void Data94File::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{
}

bool Data94File::hasAttachedFile() const
{
}

QString Data94File::attachedFileName() const
{
}

void Data94File::setAttachedFileName(const QString &name)
{
}

QVariant Data94File::channelHeader(int column) const
{
}

int Data94File::columnsCount() const
{
}

Channel *Data94File::channel(int index) const
{
}

QString Data94File::legend() const
{
}

bool Data94File::setLegend(const QString &legend)
{
}

double Data94File::xStep() const
{

}

void Data94File::setXStep(const double xStep)
{
}

int Data94File::samplesCount() const
{
}

void Data94File::setSamplesCount(int count)
{
}

QString Data94File::xName() const
{
}

bool Data94File::setDateTime(QDateTime dt)
{
}

bool Data94File::dataTypeEquals(FileDescriptor *other) const
{
}

QString Data94File::fileFilters() const
{
    return "Файлы Data94 (*.d94)";
}


QVariant Data94Channel::info(int column, bool edit) const
{
}

int Data94Channel::columnsCount() const
{
}

QVariant Data94Channel::channelHeader(int column) const
{
}

Descriptor::DataType Data94Channel::type() const
{
}

Descriptor::OrdinateFormat Data94Channel::yFormat() const
{
}

bool Data94Channel::populated() const
{
}

void Data94Channel::setPopulated(bool populated)
{
}

void Data94Channel::populate()
{
}

QString Data94Channel::name() const
{
}

void Data94Channel::setName(const QString &name)
{
}

QString Data94Channel::description() const
{
}

void Data94Channel::setDescription(const QString &description)
{
}

QString Data94Channel::xName() const
{
}

QString Data94Channel::yName() const
{
}

void Data94Channel::setYName(const QString &yName)
{
}

QString Data94Channel::legendName() const
{
}

FileDescriptor *Data94Channel::descriptor()
{
}

int Data94Channel::index() const
{
}

QString Data94Channel::correction() const
{
}

void Data94Channel::setCorrection(const QString &s)
{
}
