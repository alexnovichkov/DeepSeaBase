#include "cursor.h"
#include "plot.h"
#include "plotmodel.h"
#include "curve.h"
#include "algorithms.h"
#include <QMenu>
#include <QClipboard>
#include "settings.h"
#include "app.h"
#include "logging.h"

Cursor::Cursor(Type type, Style style, Plot *plot)
    : m_type{type}, m_style{style}, m_plot{plot}
{DD;
    m_snapToValues = Settings::getSetting("cursorSnapToValues", true).toBool();
    m_showValues = Settings::getSetting("cursorShowYValues", false).toBool();
    m_digits = Settings::getSetting("cursorDigits", m_digits).toInt();
    m_harmonics = Settings::getSetting("cursorHarmonics", m_harmonics).toInt();
    m_format = Settings::getSetting("cursorFormat", "fixed").toString()=="fixed"?Format::Fixed:Format::Scientific;
    if (m_type==Type::Double || m_type==Type::DoubleReject)
        m_info = static_cast<Info>(Settings::getSetting("cursorInfo", NoInfo).toInt());
}

void Cursor::saveSpectrum()
{DD;

}

void Cursor::saveSlice()
{DD;

}

void Cursor::copyValues() const
{DD;
    QStringList list;

    list << ""<< dataHeader(false).join('\t');

    auto curves = m_plot->model()->curves();
    for (int i=0; i<curves.size(); ++i) {
        auto d = data(i,false);
        QStringList l;
        l << curves.at(i)->title();
        for (auto v: d) {
            l << QLocale(QLocale::Russian).toString(v);
        }
        list << l.join('\t');
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(list.join("\n"));
}

void Cursor::setSnapToValues(bool snap)
{DD;
    m_snapToValues = snap;
    if (snap) updatePos();
}

QPointF Cursor::correctedPos(QPointF oldPos, int deltaX, int deltaY) const
{DD;
    if (!m_plot) return oldPos;

    const auto list = m_plot->model()->curves();
    if (list.isEmpty()) return oldPos;

    //ищем минимальный шаг по оси X
    //определяем шаг по выделенной кривой
    Curve *first = m_plot->model()->selectedCurve();
    if (!first) {
        //нет выделенной кривой - ищем первую кривую с ненулевым шагом
        for (auto c: list) if (c->channel->data()->xValuesFormat()==DataHolder::XValuesUniform) {
            first = c;
            break;
        }
        if (first) {//есть канал с ненулевым шагом
            for (auto c: list) {
                if (c->channel->data()->xStep() < first->channel->data()->xStep()
                    && c->channel->data()->xValuesFormat()==DataHolder::XValuesUniform)
                    first = c;
            }
        }
        else {//нет канала с ненулевым шагом,
            first = list.first();
        }
    }
    auto newPos = oldPos;

    if (deltaX != 0) newPos.setX(closest(first->channel, oldPos.x(), true, deltaX));
    if (deltaY != 0) newPos.setY(closest(first->channel, oldPos.y(), false, deltaY));

    return newPos;
}

void Cursor::setShowValues(bool show)
{DD;
    if (m_showValues != show) {
        m_showValues = show;
        update();
    }
}

void Cursor::setShowPeaksInfo(bool show)
{
    if (m_showPeaksInfo != show) {
        m_showPeaksInfo = show;
        update();
    }
}

void Cursor::setDigits(int digits)
{DD;
    if (m_digits != digits) {
        m_digits = digits;
        update();
    }
}

void Cursor::setHarmonics(int harmonics)
{DD;
    if (m_harmonics != harmonics) {
        m_harmonics = harmonics;
        update();
    }
}

void Cursor::setFormat(Cursor::Format format)
{DD;
    if (m_format != format) {
        m_format = format;
        update();
    }
}

void Cursor::setInfo(Info info)
{DD;
    if (m_info != info) {
        m_info = info;
        emit dataChanged();
    }
}

void Cursor::addRejectCursor(Cursor *c)
{DD;
    rejectCursors.append(c);
}

void Cursor::removeRejectCursor(Cursor *c)
{DD;
    rejectCursors.removeOne(c);
}
