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
    void replotDescriptor(FileDescriptor *f);

    QVector<Channel*> plottedChannels() const;
    QVector<FileDescriptor*> plottedDescriptors() const;
    int curvesCount(int type=-1) const;
    PlotType type() const;
signals:
    void curvesCountChanged(); //<- MainWindow::updateActions()
    void channelPlotted(Channel *ch);
    void curveDeleted(Channel *);
    void descriptorRequested(int direction, bool checked);
public slots:
    void updateLegends();

private:
    Plot *m_plot =nullptr;

    QAction *autoscaleXAct = nullptr;
    QAction *autoscaleYAct = nullptr;
    QAction *autoscaleYSlaveAct = nullptr;
    QAction *autoscaleAllAct = nullptr;
    QAction *removeLabelsAct = nullptr;
    QAction *previousDescriptorAct = nullptr;
    QAction *nextDescriptorAct = nullptr;
    QAction *arbitraryDescriptorAct = nullptr;
    QAction *cycleChannelsUpAct = nullptr;
    QAction *cycleChannelsDownAct = nullptr;

    QAction *clearPlotAct = nullptr;
    QAction *savePlotAct = nullptr;
    QAction *switchCursorAct = nullptr;
    QAction *trackingCursorAct = nullptr;
    QAction *copyToClipboardAct = nullptr;
    QAction *printPlotAct = nullptr;
    QAction *interactionModeAct = nullptr;
    QAction *playAct = nullptr;

    PlotType plotType = PlotType::General;
};

#endif // PLOTAREA_H
