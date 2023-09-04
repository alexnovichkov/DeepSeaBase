#ifndef DFDIO_H
#define DFDIO_H

#include "fileio.h"
#include "filedescriptor.h"

class DfdIO : public FileIO
{
    Q_OBJECT
public:
    DfdIO(const QVector<Channel*> &source,
          const QString &fileName, QObject *parent = nullptr,
          const QMap<QString, QVariant> & parameters = {});
    //Используется для создания нового файла или дозаписи каналов в файл,
    //если заранее не известно количество каналов
    DfdIO(const DataDescription &description, const QString &fileName, QObject *parent = nullptr,
          const QMap<QString, QVariant> & parameters = {});

    // FileIO interface
public:
    virtual void addChannel(DataDescription *description, DataHolder *data) override;
    virtual void finalize() override;
private:
    int m_channelsCount = 0;
    QString m_rawFileName;
    bool m_fileExists = false;
    DataDescription m_fileDescription;
};

#endif // DFDIO_H
