#ifndef DATA3D_H
#define DATA3D_H

class DataHolder;

#include "qcustomplot.h"

class Data3D
{
public:
    explicit Data3D(DataHolder *data);

private:
    DataHolder *mData = nullptr;
    bool mDataModified = false;

    friend class QCPSpectrogram;

public:
    int keySize() const;
    int valueSize() const;
    QCPRange keyRange() const;
    QCPRange keyRange(int key) const;
    QCPRange valueRange() const;
    QCPRange dataBounds() const;
    double data(double key, double value) const;
    double cell(int keyIndex, int valueIndex) const;
    unsigned char alpha(int keyIndex, int valueIndex) const;
    void setSize(int keySize, int valueSize);
    void setKeySize(int keySize);
    void setValueSize(int valueSize);
    void setRange(const QCPRange &keyRange, const QCPRange &valueRange);
    void setKeyRange(const QCPRange &keyRange);
    void setValueRange(const QCPRange &valueRange);
    void setData(double key, double value, double z);
    void setCell(int keyIndex, int valueIndex, double z);
    void setAlpha(int keyIndex, int valueIndex, unsigned char alpha);
    void recalculateDataBounds();
    void clear();
    void clearAlpha();
    void fill(double z);
    void fillAlpha(unsigned char alpha);
    bool isEmpty() const;
    void coordToCell(double key, double value, int *keyIndex, int *valueIndex) const;
    void cellToCoord(int keyIndex, int valueIndex, double *key, double *value) const;
};

#endif // DATA3D_H
