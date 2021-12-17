#ifndef PLOTAREA_H
#define PLOTAREA_H

#include "DockWidget.h"
#include <plot/plot.h>


class QAction;
class Curve;
class FileDescriptor;

enum class PlotType
{
    Time,
    General,
    Octave,
    Spectrogram
};

class PlotArea : public ads::CDockWidget
{
    Q_OBJECT
public:
    PlotArea(int index, PlotType type, QWidget *parent);
    Plot* plot();

    void update();

    //QList<Curve *> curves() const;
    void exportToExcel(bool fullRange, bool dataOnly);
    void updateActions(int filesCount, int channelsCount);
    void deleteCurvesForDescriptor(FileDescriptor *f);
    QVector<Channel*> plottedChannels() const;

    int curvesCount(int type=-1) const;
    PlotType type() const;
signals:
    void needPlotChannels(bool plotOnLeft, const QVector<Channel*> &channels);
public slots:
    void updateLegends();

private:
    Plot *m_plot =nullptr;

    QAction *autoscaleXAct;
    QAction *autoscaleYAct;
    QAction *autoscaleYSlaveAct;
    QAction *autoscaleAllAct;
    QAction *removeLabelsAct;
    QAction *previousDescriptorAct;
    QAction *nextDescriptorAct;
    QAction *arbitraryDescriptorAct;
    QAction *cycleChannelsUpAct;
    QAction *cycleChannelsDownAct;

    QAction *clearPlotAct;
    QAction *savePlotAct;
    QAction *switchCursorAct;
    QAction *trackingCursorAct;
    QAction *copyToClipboardAct;
    QAction *printPlotAct;
    QAction *interactionModeAct;
    QAction *playAct;

    PlotType plotType = PlotType::General;
};

#endif // PLOTAREA_H
