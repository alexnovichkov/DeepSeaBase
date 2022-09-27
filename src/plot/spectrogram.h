#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

#include "plot.h"

class Spectrogram : public Plot
{
public:
    Spectrogram(QWidget *parent = 0);

    virtual void deleteCurve(Curve *curve, bool doReplot = true) override;
    virtual QString pointCoordinates(const QPointF &pos) override;
    virtual void updateAxesLabels() override;
    virtual void plotChannel(Channel *ch, bool plotOnLeft, int fileIndex = 0) override;
    virtual void onDropEvent(bool plotOnLeft, const QVector<Channel *> &channels) override;
protected:
    virtual void updateBounds() override;
    virtual bool canBePlottedOnLeftAxis(Channel *ch, QString *message = nullptr) const override;
    virtual bool canBePlottedOnRightAxis(Channel *ch, QString *message = nullptr) const override;
    virtual void setRightScale(Enums::AxisType id, double min, double max) override;

    QMenu * createMenu(Enums::AxisType axis, const QPoint &pos) override;
private:
    int colorMap = 0;

};

#endif // SPECTROGRAM_H
