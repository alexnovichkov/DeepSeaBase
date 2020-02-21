#include "dataiodevice.h"

#include "filedescriptor.h"
#include "logging.h"
#include "dfdfiledescriptor.h"

DataIODevice::DataIODevice(Channel *channel, QObject *parent)
    : QIODevice(parent), m_channel(channel)
{DDD;

}


bool DataIODevice::isSequential() const
{DDD;
    return false;
}

bool DataIODevice::open(QIODevice::OpenMode mode)
{DDD;
    qDebug()<<mode;
    return QIODevice::open(mode);
}

void DataIODevice::close()
{DDD;
    QIODevice::close();
}

double DataIODevice::positionSec() const
{
    return double(m_pos) * m_channel->xStep();
}

qint64 DataIODevice::pos() const
{DDD;
    return m_pos * sizeof(double);
}

qint64 DataIODevice::size() const
{DDD;
    return m_channel->samplesCount() * sizeof(double);
}

bool DataIODevice::seek(qint64 pos)
{DDD;
    QIODevice::seek(pos);
    m_pos = pos / sizeof(double);
    return (m_pos <= m_channel->samplesCount());
}

bool DataIODevice::atEnd() const
{DD;
    return (m_pos >= m_channel->samplesCount());
}

//bool DataIODevice::reset()
//{DDD;
//    m_pos = 0;
//    return true;
//}

//qint64 DataIODevice::bytesAvailable() const
//{DDD;
//    return (m_data->samplesCount() - m_pos)*sizeof(double) + QIODevice::bytesAvailable();
//}

//qint64 DataIODevice::bytesToWrite() const
//{DDD;
//    return QIODevice::bytesToWrite();
//}

bool DataIODevice::canReadLine() const
{DD;
    return false;
}

qint64 DataIODevice::readData(char *data, qint64 maxlen)
{DD;
    memset(data, 0, maxlen);

    QByteArray b;
    if (DfdChannel *raw = dynamic_cast<DfdChannel*>(m_channel)) {
        QVector<double> mid = raw->yValues().mid(m_pos, qint64(maxlen/sizeof(double)));

        QDataStream buff(&b, QIODevice::WriteOnly);
        buff.setByteOrder(QDataStream::LittleEndian);
        for (int i = 0; i < mid.length(); ++i) {
            double v = raw->preprocess(mid[i]);
            quint16 vv = (quint16)v;
            if (vv > 32768) vv -= 32768;
            else vv += 32768;
            buff << vv;
        }
        memcpy(data, b.data(), b.length());
        m_pos += mid.length();
    }
    return b.length();
}

qint64 DataIODevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    return len;
}
