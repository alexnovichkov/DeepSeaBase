#ifndef SPECTRE94_H
#define SPECTRE94_H

#include <QtCore>

class Spectre94
{
public:
    explicit Spectre94();
    explicit Spectre94(const QString fileName);
    explicit Spectre94(const Spectre94 &file);

    void read();

    bool write();

    template <typename T>
    void populate();

    QString description() const;

private:
    QString m_description;
};

#endif // SPECTRE94_H
