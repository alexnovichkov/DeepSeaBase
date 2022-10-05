#include "data3d.h"

#include "dataholder.h"

Data3D::Data3D(DataHolder *data) : mData(data)
{

}

int Data3D::keySize() const
{
    if (mData) return mData->samplesCount();
    return 0;
}

int Data3D::valueSize() const
{
    if (mData) return mData->blocksCount();
    return 0;
}

QCPRange Data3D::keyRange() const
{
    if (mData) return QCPRange(mData->xMin(), mData->xMax());
    return QCPRange();
}

QCPRange Data3D::valueRange() const
{
    if (mData) return QCPRange(mData->zMin(), mData->zMax());
    return QCPRange();
}

QCPRange Data3D::dataBounds() const
{
    return QCPRange(mData->yMin(-1), mData->yMax(-1));
}

double Data3D::data(double key, double value) const
{
    if ( !( keyRange().contains(key) && valueRange().contains(value) ) )
        return qQNaN();

    int j = mData->nearestZ(value);
    if (j < 0) return qQNaN();

    int i = mData->nearest(key);
    if (i < 0) return qQNaN();

    return mData->yValue(i, j);
}

double Data3D::cell(int keyIndex, int valueIndex) const
{
    if (keyIndex >= 0 && keyIndex < mData->samplesCount() && valueIndex >=0 && valueIndex < mData->blocksCount())
        return mData->yValue(keyIndex, valueIndex);
    return qQNaN();
}

unsigned char Data3D::alpha(int keyIndex, int valueIndex) const
{
    Q_UNUSED(keyIndex);
    Q_UNUSED(valueIndex);
    return 255;
}

void Data3D::setSize(int keySize, int valueSize)
{
    Q_UNUSED(keySize);
    Q_UNUSED(valueSize);
}

void Data3D::setKeySize(int keySize)
{
    Q_UNUSED(keySize);
}

void Data3D::setValueSize(int valueSize)
{
    Q_UNUSED(valueSize);
}

void Data3D::setRange(const QCPRange &keyRange, const QCPRange &valueRange)
{
    Q_UNUSED(keyRange);
    Q_UNUSED(valueRange);
}

void Data3D::setKeyRange(const QCPRange &keyRange)
{
    Q_UNUSED(keyRange);
}

void Data3D::setValueRange(const QCPRange &valueRange)
{
    Q_UNUSED(valueRange);
}

void Data3D::setData(double key, double value, double z)
{
    if ( !( keyRange().contains(key) && valueRange().contains(value) ) )
        return;

    int j = mData->nearestZ(value);
    if (j < 0) return;

    int i = mData->nearest(key);
    if (i < 0) return;

    mData->setYValue(i, z, j);
    mDataModified = true;
}

void Data3D::setCell(int keyIndex, int valueIndex, double z)
{
    if (keyIndex >= 0 && keyIndex < mData->samplesCount() && valueIndex >=0 && valueIndex < mData->blocksCount()) {
        mData->setYValue(keyIndex, z, valueIndex);
        mDataModified = true;
    }
}

void Data3D::setAlpha(int keyIndex, int valueIndex, unsigned char alpha)
{
    Q_UNUSED(keyIndex);
    Q_UNUSED(valueIndex);
    Q_UNUSED(alpha);
}

void Data3D::recalculateDataBounds()
{

}

void Data3D::clear()
{
}

void Data3D::clearAlpha()
{
    //No-OP
}

void Data3D::fill(double z)
{
    Q_UNUSED(z);
}

void Data3D::fillAlpha(unsigned char alpha)
{
    Q_UNUSED(alpha);
}

bool Data3D::isEmpty() const
{
    return mData->samplesCount() * mData->blocksCount() == 0;
}

void Data3D::coordToCell(double key, double value, int *keyIndex, int *valueIndex) const
{
    if ( !( keyRange().contains(key) && valueRange().contains(value) ) )
        return;

    int j = mData->nearestZ(value);
    if (j < 0) return;
    if (valueIndex) *valueIndex = j;

    int i = mData->nearest(key);
    if (i < 0) return;
    if (keyIndex) *keyIndex = i;
}

void Data3D::cellToCoord(int keyIndex, int valueIndex, double *key, double *value) const
{
    if (key) *key = mData->xValue(keyIndex);
    if (value) *value = mData->zValue(valueIndex);
}
