#include "cursor.h"
#include "plot.h"
#include "plotmodel.h"
#include "curve.h"
#include "algorithms.h"
#include <QMenu>
#include <QClipboard>
#include "app.h"

Cursor::Cursor(Type type, Style style, Plot *plot)
    : m_type{type}, m_style{style}, m_plot{plot}
{
    m_snapToValues = App->getSetting("cursorSnapToValues", true).toBool();
    m_showValues = App->getSetting("cursorShowYValues", false).toBool();
    m_digits = App->getSetting("cursorDigits", 2).toInt();
    m_harmonics = App->getSetting("cursorHarmonics", 10).toInt();
    m_format = App->getSetting("cursorFormat", "fixed").toString()=="fixed"?Format::Fixed:Format::Scientific;
    if (m_type==Type::Double || m_type==Type::DoubleReject)
        m_info = static_cast<Info>(App->getSetting("cursorInfo", 0).toInt());
}

void Cursor::copyValues() const
{
    QStringList list;

    list << dataHeader(false).join('\t');

    auto curves = m_plot->model()->curves();
    for (int i=0; i<curves.size(); ++i) {
        auto d = data(i,false);
        QStringList l;
        for (auto v: d) {
            l << QString::number(v, m_format==Format::Fixed?'f':'e', m_digits);
        }
        list << l.join('\t');
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(list.join("\n"));
}

void Cursor::setSnapToValues(bool snap)
{
    m_snapToValues = snap;
    if (snap) updatePos();
}

QPointF Cursor::correctedPos(QPointF oldPos, int deltaX, int deltaY) const
{
    if (!m_plot) return oldPos;

    const auto list = m_plot->model()->curves();
    if (list.isEmpty()) return oldPos;

    //ищем минимальный шаг по оси X
    Curve *first = m_plot->model()->selectedCurve();
    if (!first) {
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
    oldPos.setX(closest(first->channel, oldPos.x(), true, deltaX));
    if (first->type == Curve::Type::Spectrogram)
        oldPos.setY(closest(first->channel, oldPos.y(), false, deltaY));

    return oldPos;
}

void Cursor::setShowValues(bool show)
{
    if (m_showValues != show) {
        m_showValues = show;
        update();
    }
}

void Cursor::setDigits(int digits)
{
    if (m_digits != digits) {
        m_digits = digits;
        update();
    }
}

void Cursor::setHarmonics(int harmonics)
{
    if (m_harmonics != harmonics) {
        m_harmonics = harmonics;
        update();
    }
}

void Cursor::setFormat(Cursor::Format format)
{
    if (m_format != format) {
        m_format = format;
        update();
    }
}

void Cursor::setInfo(Info info)
{
    if (m_info != info) {
        m_info = info;
        emit dataChanged();
    }
}

void Cursor::addRejectCursor(Cursor *c)
{
    rejectCursors.append(c);
}

void Cursor::removeRejectCursor(Cursor *c)
{
    rejectCursors.removeOne(c);
}
