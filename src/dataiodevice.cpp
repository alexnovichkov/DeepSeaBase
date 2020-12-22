#include "dataiodevice.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

DataIODevice::DataIODevice(Channel *channel, QObject *parent)
    : QIODevice(parent), m_channel(channel)
{DD;

}


bool DataIODevice::isSequential() const
{DD;
    return false;
}

double DataIODevice::positionSec() const
{
    return pos()/sizeof(qint16) * m_channel->data()->xStep();
}

bool DataIODevice::seek(qint64 pos)
{DD;
    QIODevice::seek(pos);
    if (pos < 0) return false;

    m_pos = pos / sizeof(qint16);

    return (m_pos <= m_channel->data()->samplesCount());
}

bool DataIODevice::atEnd() const
{DD;
    return (m_pos >= m_channel->data()->samplesCount()-1);
}

bool DataIODevice::reset()
{DDD;
    QIODevice::reset();
    m_pos = 0;
    return true;
}

bool DataIODevice::canReadLine() const
{DD;
    return false || QIODevice::canReadLine();
}

qint64 DataIODevice::size() const
{
    return m_channel->data()->samplesCount() * sizeof(qint16);
}

qint64 DataIODevice::readData(char *data, qint64 maxlen)
{DD;
    //заполняем нулями
    memset(data, 0, maxlen);

    //что-то пошло не так в seek
    if (m_pos < 0) {
//        qDebug()<<"why m_pos < 0?";
        return 0;
    }

//    if (m_pos *2 != pos()) {
//        qDebug()<<"position mismatch: m_pos"<<m_pos<<"pos"<<pos();
//    }

    //получаем сырые данные, количество отсчетов равно буферу / sizeof(int16)
    QByteArray b = m_channel->wavData(m_pos, maxlen / sizeof(qint16));

    DebugPrint(m_pos);
    DebugPrint(maxlen);
    DebugPrint(pos());
    qDebug()<<b.mid(0,20);

    if (b.isEmpty()) return 0;

    memcpy(data, b.data(), b.length());
    m_pos += (b.length() / sizeof(qint16));

    // no more data available
    if (m_pos >= m_channel->data()->samplesCount()-1) return -1;

    return b.length();
}

qint64 DataIODevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    return len;
}
