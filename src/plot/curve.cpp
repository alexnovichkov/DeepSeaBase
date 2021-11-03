#include "curve.h"
#include "qwt_symbol.h"
#include "pointlabel.h"

#include "fileformats/filedescriptor.h"
#include <qwt_curve_fitter.h>
#include "logging.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"
#include "qwt_clipper.h"
#include "dataholder.h"
#include "qwt_plot.h"

Curve::Curve(const QString &title, Channel *channel)
{DD;
    Q_UNUSED(title)

    this->channel = channel;
    this->duplicate = false;
    this->highlighted = false;
}

Curve::~Curve()
{DD;
    foreach(PointLabel *l, labels) l->detach();
    qDeleteAll(labels);
    labels.clear();

    //maybe clear data that is over 1000000 samples
    if (channel) channel->maybeClearData();
}



void Curve::addLabel(PointLabel *label)
{DD;
    labels << label;
}

void Curve::removeLabel(PointLabel *label)
{DD;
    if (labels.contains(label)) {
        labels.removeOne(label);
        label->detach();
        delete label;
    }
}

void Curve::removeLabels()
{
    foreach (PointLabel *label, labels) {
        label->detach();
        delete label;
    }
    labels.clear();
}

PointLabel *Curve::findLabel(const QPoint &pos/*, QwtAxisId yAxis*/)
{DD;
    foreach (PointLabel *l, labels)
        if (l->contains(pos/*, yAxis*/))
            return l;

    return 0;
}

PointLabel *Curve::findLabel(const int point)
{DD;
    foreach (PointLabel *l, labels)
        if (l->point() == point)
            return l;

    return 0;
}

void Curve::resetHighlighting()
{DD;
    setPen(oldPen);
//    foreach(PointLabel *label, labels)
//        if (label) label->setSelected(false);
    highlighted = false;
}

void Curve::highlight()
{DD;
    QPen p = pen();
    oldPen = p;
    p.setWidth(2);
    setPen(p);
    highlighted = true;
}



double Curve::yMin() const
{DD;
    return channel->data()->yMin();
}

double Curve::yMax() const
{DD;
    return channel->data()->yMax();
}

double Curve::xMin() const
{DD;
    return channel->data()->xMin();
}

double Curve::xMax() const
{DD;
    return channel->data()->xMax();
}

int Curve::samplesCount() const
{DD;
    return channel->data()->samplesCount();
}



void Curve::setVisible(bool visible)
{DD;
    //d->setVisible(visible);
    foreach (PointLabel *label, labels) {
        label->setVisible(visible);
    }
}

void Curve::evaluateScale(int &from, int &to, const QwtScaleMap &xMap) const
{DD;
    const double startX = xMap.s1();
    const double endX = xMap.s2();

    if (channel->data()->xValuesFormat()==DataHolder::XValuesUniform) {
        from = qRound((startX - channel->data()->xMin())/channel->data()->xStep())-1;
        to = qRound((endX - channel->data()->xMin())/channel->data()->xStep())+1;
    }
    else {
        for (int i=0; i<to; ++i) {
            if (samplePoint(i).x() >= startX) {
                from = i-1;
                break;
            }
        }
        for (int i=to; i>=from; --i) {
            if (samplePoint(i).x() <= endX) {
                to = i+1;
                break;
            }
        }
    }
    if (from < 0) from = 0;
    if (to >= channel->data()->samplesCount()) to = channel->data()->samplesCount()-1;
}

void Curve::switchFixed()
{DD;
    fixed = !fixed;
}

