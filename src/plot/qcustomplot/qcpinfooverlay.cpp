#include "qcpinfooverlay.h"

QCPInfoOverlay::QCPInfoOverlay(QCustomPlot *parent) : QCPItemText(parent)
{
    QString title("- Перетащите сюда каналы, чтобы построить их графики\n"
                  "- Если зажать Ctrl, то будут построены графики для всех\n"
                  "  выделенных файлов");
    setText(title);
    position->setType(QCPItemPosition::ptAxisRectRatio);
    position->setCoords(0.5, 0.5);
    setPositionAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    QFont font;
    //font.setBold(true);
    font.setPointSize(12);
    setFont(font);
    setColor(QColor(150,150,150,150));
    setBrush(QColor(255,255,255,150));
    setLayer("overlay");
}
