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

//qint64 DataIODevice::pos() const
//{DDD;
//    return m_pos*sizeof(double);
//}

//qint64 DataIODevice::size() const
//{DDD;
//    return m_data->samplesCount()*sizeof(double);
//}

//bool DataIODevice::seek(qint64 pos)
//{DDD;
//    QIODevice::seek(pos);
//    m_pos = pos / sizeof(double);
//    return (m_pos <= m_data->samplesCount());
//}

//bool DataIODevice::atEnd() const
//{DDD;
//    return (m_pos >= m_data->samplesCount());
//}

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

//bool DataIODevice::canReadLine() const
//{DDD;
//    return false;
//}

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
            buff << (quint16)v;
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
