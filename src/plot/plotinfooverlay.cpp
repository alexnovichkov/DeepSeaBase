#include "plotinfooverlay.h"
#include <QFont>
#include <QColor>
#include <QBrush>
#include "logging.h"

PlotInfoOverlay::PlotInfoOverlay(QwtPlot *parent) : QwtPlotTextLabel(), parent{parent}
{DDD;
    attach(parent);

    QwtText title("- Перетащите сюда каналы, чтобы построить их графики\n"
                  "- Если зажать Ctrl, то будут построены графики для всех\n"
                  "  выделенных файлов");
    title.setRenderFlags(Qt::AlignHCenter | Qt::AlignVCenter);

    QFont font;
    //font.setBold(true);
    font.setPointSize(12);
    title.setFont(font);
    title.setColor(QColor(150,150,150,150));
    title.setBackgroundBrush(QColor(255,255,255,150));
    setText(title);
}
