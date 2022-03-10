#include "cursor.h"
#include "plot.h"
#include "plotmodel.h"
#include "curve.h"
#include "algorithms.h"

Cursor::Cursor(Type type, Style style, Plot *plot)
    : type{type}, style{style}, plot{plot}
{
}

void Cursor::setSnapToValues(bool snap)
{
    snapToValues = snap;
    if (snap) updatePos();
}

QPointF Cursor::correctedPos(QPointF oldPos)
{
    if (!plot) return oldPos;

    const auto list = plot->model()->curves();
    if (list.isEmpty()) return oldPos;

    //ищем минимальный шаг по оси X
    Curve *first = plot->model()->selectedCurve();
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
    oldPos.setX(closest(first->channel, oldPos.x()));
    if (first->type == Curve::Type::Spectrogram)
        oldPos.setY(closest(first->channel, oldPos.y(), false));

    return oldPos;
}

