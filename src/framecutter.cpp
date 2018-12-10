#include "framecutter.h"

FrameCutter::FrameCutter()
{

}

int FrameCutter::type() const
{
    return param.type;
}

void FrameCutter::setType(int type)
{
    param.type = type;
}

int FrameCutter::blockSize() const
{
    return param.blockSize;
}

void FrameCutter::setBlockSize(int blockSize)
{
    param.blockSize = blockSize;
}

void FrameCutter::setSource(const QVector<double> &data)
{
    this->data = data;
}

void FrameCutter::setTriggerSource(const QVector<double> &triggerData)
{
    this->triggerData = triggerData;
}

QVector<double> FrameCutter::get(bool *ok)
{
    QVector<double> result;
    switch (param.type) {
        case Continuous: return getSimple(ok);
        case Overlap: return getWithOverlap(ok);
        case Delta: return getWithDelta(ok);
        case Trigger: return getWithTrigger(ok);
        default:
            return result;
    }
    return result;
}

void FrameCutter::reset()
{
    data.clear();
    triggerData.clear();
    firstTriggerSearched = false;
    currentSample = 0;
}

QVector<double> FrameCutter::getSimple(bool *ok)
{
    Q_ASSERT(currentSample >= 0);

    QVector<double> output(param.blockSize);
    if (currentSample < data.size()) {
        output = data.mid(currentSample, param.blockSize);
        output.resize(param.blockSize);
        currentSample += param.blockSize;
        if (ok) *ok = true;
    }
    else if (ok) *ok=false;
    return output;
}

QVector<double> FrameCutter::getWithOverlap(bool *ok)
{
    Q_ASSERT(currentSample >= 0);

    QVector<double> output(param.blockSize);
    if (currentSample < data.size()) {
        output = data.mid(currentSample, param.blockSize);
        output.resize(param.blockSize);
        currentSample += param.blockSize - param.delta;
        if (ok) *ok = true;
    }
    else if (ok) *ok=false;
    return output;
}

QVector<double> FrameCutter::getWithDelta(bool *ok)
{
    //Delta может быть задана тремя способами:
    //* percent double [1..99], в % от размера блока
    //* deltaTime double [s] [0..inf]
    //* deltaBlock int [0..]
    // если в параметры записано несколько значений, то будет использована длина в секундах

    Q_ASSERT(currentSample >= 0);

    QVector<double> output(param.blockSize);
    if (currentSample < data.size()) {
        output = data.mid(currentSample, param.blockSize);
        output.resize(param.blockSize);
        currentSample += param.blockSize + param.delta;
        if (ok) *ok = true;
    }
    else if (ok) *ok=false;
    return output;
}

QVector<double> FrameCutter::getWithTrigger(bool *ok)
{
    currentSample = searchTrigger(currentSample);

    Q_ASSERT(currentSample >= 0);

    QVector<double> output;
    if (currentSample < data.size()) {
        int pos = currentSample - param.pretrigger;
        if (pos < 0) pos = 0;
        output = data.mid(pos, param.blockSize);
        output.resize(param.blockSize);
        if (ok) *ok = true;
    }
    else if (ok) *ok=false;
    return output;
}

int FrameCutter::searchTrigger(const int pos)
{
    int current = pos;
    if (firstTriggerSearched) {
        current += param.blockSize;
        current += param.delta;
    }
    bool found = false;
    for (int i=current; i<triggerData.size(); ++i) {
        if (param.mode == GreaterThan && triggerData.at(i) > param.level) {
            current = i;
            found = true;
            break;
        }
        if (param.mode == LessThan && triggerData.at(i) < param.level) {
            current = i;
            found = true;
            break;
        }
    }
    if (!found) current = data.size();
    if (current < 0) current = 0;

    firstTriggerSearched = true;

    return current;
}

double FrameCutter::getXStep() const
{
    return param.xStep;
}

void FrameCutter::setXStep(double value)
{
    param.xStep = value;
}

int FrameCutter::getDelta() const
{
    return param.delta;
}

void FrameCutter::setDelta(int value)
{
    param.delta = value;
}

int FrameCutter::getChannel() const
{
    return param.channel;
}

void FrameCutter::setChannel(int value)
{
    param.channel = value;
}

double FrameCutter::getLevel() const
{
    return param.level;
}

void FrameCutter::setLevel(double value)
{
    param.level = value;
}

int FrameCutter::getMode() const
{
    return param.mode;
}

void FrameCutter::setMode(int value)
{
    param.mode = value;
}

int FrameCutter::getPretrigger() const
{
    return param.pretrigger;
}

void FrameCutter::setPretrigger(int value)
{
    param.pretrigger = value;
}
