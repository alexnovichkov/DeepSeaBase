#include "framecutter.h"
#include "logging.h"

FrameCutter::FrameCutter()
{DD;

}

int FrameCutter::type() const
{DD;
    return param.type;
}

void FrameCutter::setType(int type)
{DD;
    param.type = type;
}

int FrameCutter::blockSize() const
{DD;
    return param.blockSize;
}

void FrameCutter::setBlockSize(int blockSize)
{DD;
    param.blockSize = blockSize;
}

void FrameCutter::setSource(const QVector<double> &data)
{DD;
    this->data = data;
}

void FrameCutter::setTriggerSource(const QVector<double> &triggerData)
{DD;
    this->triggerData = triggerData;
}

QVector<double> FrameCutter::get(bool *ok)
{DD;
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

QVector<double> FrameCutter::getAll()
{DD;
    QVector<double> result;
    bool ok = true;
    while (ok) {
        auto d = get(&ok);
        result.append(d);
    }
    return result;
}

void FrameCutter::reset(bool clearData)
{DD;
    if (clearData) {
        data.clear();
        triggerData.clear();
    }

    firstTriggerSearched = false;
    currentSample = 0;
}

int FrameCutter::getBlocksCount() const
{DD;
    switch (param.type) {
        case Continuous: return getBlocksCountSimple();
        case Overlap: return getBlocksCountWithOverlap();
        case Delta: return getBlocksCountWithDelta();
        case Trigger: return getBlocksCountWithTrigger();
        default:
            return 1;
    }
    return 1;
}

QVector<double> FrameCutter::getSimple(bool *ok)
{DD;
    Q_ASSERT(currentSample >= 0);
    QVector<double> output;
    if (currentSample < data.size()) {
        output = data.mid(currentSample, param.blockSize);
        output.resize(param.blockSize);
        currentSample += param.blockSize;
        if (ok) *ok = true;
    }
    else if (ok) *ok=false;

    return output;
}

int FrameCutter::getBlocksCountSimple() const
{DD;
    return qCeil(double(data.size()) / param.blockSize);
}

QVector<double> FrameCutter::getWithOverlap(bool *ok)
{DD;
    Q_ASSERT(currentSample >= 0);

    double delta = getDelta();

    QVector<double> output;
    if (currentSample < data.size()) {
        output = data.mid(currentSample, param.blockSize);
        output.resize(param.blockSize);
        currentSample += param.blockSize - delta;
        if (ok) *ok = true;
    }
    else if (ok) *ok=false;
    return output;
}

int FrameCutter::getBlocksCountWithOverlap() const
{DD;
    int count = 0;
    for (int i=0; i<data.size(); ) {
        i += (param.blockSize - getDelta());
        count++;
    }
    return count;
}

QVector<double> FrameCutter::getWithDelta(bool *ok)
{DD;
    //Delta может быть задана тремя способами:
    //* percent double [1..99], в % от размера блока
    //* deltaTime double [s] [0..inf]
    //* deltaBlock int [0..]
    // если в параметры записано несколько значений, то будет использована длина в секундах

    Q_ASSERT(currentSample >= 0);

    QVector<double> output;
    if (currentSample < data.size()) {
        output = data.mid(currentSample, param.blockSize);
        output.resize(param.blockSize);
        currentSample += param.blockSize + getDelta();
        if (ok) *ok = true;
    }
    else if (ok) *ok = false;
    return output;
}

int FrameCutter::getBlocksCountWithDelta() const
{DD;
    int count = 0;
    for (int i=0; i<data.size(); ) {
        i += (param.blockSize + getDelta());
        count++;
    }
    return count;
}

QVector<double> FrameCutter::getWithTrigger(bool *ok)
{DD;
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

int FrameCutter::getBlocksCountWithTrigger() const
{DD;
    //TODO: сделать подсчет блоков для триггера - сейчас стоит заглушка
    return getBlocksCountSimple();
}

int FrameCutter::searchTrigger(const int pos)
{DD;
    int current = pos;
    if (firstTriggerSearched) {
        current += param.blockSize;
        current += getDelta();
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
{DD;
    return param.xStep;
}

void FrameCutter::setXStep(double value)
{DD;
    param.xStep = value;
}

int FrameCutter::getDelta() const
{DD;
    if (param.deltaType == DeltaPercent) return int(1.0 * param.blockSize * param.deltaValue / 100);
    if (param.deltaType == DeltaTime && !qFuzzyIsNull(param.xStep)) return int(param.deltaValue / param.xStep);
    return 0;
}

void FrameCutter::setDelta(double value, DeltaType type)
{DD;
    param.deltaValue = value;
    param.deltaType = type;
}

int FrameCutter::getChannel() const
{DD;
    return param.channel;
}

void FrameCutter::setChannel(int value)
{DD;
    param.channel = value;
}

double FrameCutter::getLevel() const
{DD;
    return param.level;
}

void FrameCutter::setLevel(double value)
{DD;
    param.level = value;
}

int FrameCutter::getMode() const
{DD;
    return param.mode;
}

void FrameCutter::setMode(int value)
{DD;
    param.mode = value;
}

int FrameCutter::getPretrigger() const
{DD;
    return param.pretrigger;
}

void FrameCutter::setPretrigger(int value)
{DD;
    param.pretrigger = value;
}
