#ifndef SAMPLING_H
#define SAMPLING_H

#include <QtCore>

class FrameCutter
{
public:
    enum SamplingType {
        Continuous = 0,
        Overlap,
        Delta,
        Trigger,
        SamplingTypeCount
    };
    enum TriggerMode {
        GreaterThan = 0,
        LessThan
    };


    explicit FrameCutter();

    int type() const;
    void setType(int type);

    int blockSize() const;
    void setBlockSize(int blockSize);

    double getXStep() const;
    void setXStep(double value);

    int getDelta() const;
    void setDelta(int value);

    int getChannel() const;
    void setChannel(int value);

    double getLevel() const;
    void setLevel(double value);

    int getMode() const;
    void setMode(int mode);

    int getPretrigger() const;
    void setPretrigger(int value);

    bool isEmpty() const {return data.isEmpty();}

    void setSource(const QVector<double> &data);
    void setTriggerSource(const QVector<double> &triggerData);

    QVector<double> get(bool *ok=0);
    void reset();

    int getBlocksCount() const;
private:
    struct Parameters {
        int blockSize = 1024;
        int type = Continuous;
        double xStep = 1.0;
        int delta = 0;
        int channel = 0;
        double level = 0.0;
        int mode = GreaterThan;
        int pretrigger = 0;
    };

    //возвращает блок последовательно
    QVector<double> getSimple(bool *ok);
    //возвращает блок с перекрытием, то есть с отступом назад
    QVector<double> getWithOverlap(bool *ok);
    //возвращает блок с отступом вперед
    QVector<double> getWithDelta(bool *ok);
    //возвращает блок с отступом по триггеру
    QVector<double> getWithTrigger(bool *ok);

    int getBlocksCountSimple() const;
    //возвращает блок с перекрытием, то есть с отступом назад
    int getBlocksCountWithOverlap() const;
    //возвращает блок с отступом вперед
    int getBlocksCountWithDelta() const;
    //возвращает блок с отступом по триггеру
    int getBlocksCountWithTrigger() const;

    int searchTrigger(const int pos);

    Parameters param;
    QVector<double> data;
    QVector<double> triggerData;
    bool firstTriggerSearched = false;
    int currentSample = 0;
};

#endif // SAMPLING_H
