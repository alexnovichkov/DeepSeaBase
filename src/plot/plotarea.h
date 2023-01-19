#ifndef PLOTAREA_H
#define PLOTAREA_H

#include "DockWidget.h"
#include "enums.h"

class Plot;
class QAction;
class Curve;
class FileDescriptor;
class QGridLayout;
class Channel;
class QLabel;

class PlotArea : public ads::CDockWidget
{
    Q_OBJECT
public:
    PlotArea(int index, QWidget *parent);
    ~PlotArea();
    Plot* plot();
    void addPlot(Enums::PlotType type);

    void update();

    //QList<Curve *> curves() const;
    void exportToExcel(bool fullRange, bool dataOnly);
    void updateActions(int filesCount, int channelsCount);
    void deleteCurvesForDescriptor(FileDescriptor *f);
    void replotDescriptor(FileDescriptor *f, int fileIndex);

    QVector<Channel*> plottedChannels() const;
    QMap<FileDescriptor *, QVector<int> > plottedDescriptors() const;
    int curvesCount(int type=-1) const;
    void onDropEvent(bool plotOnLeft, const QVector<Channel*> &channels);
    void resetCycling();
signals:
    void descriptorRequested(int direction, bool checked);
    //void cycleChannelsRequested(bool up);

    //redirected from m_plot
    void needPlotChannels(bool plotOnLeft, const QVector<Channel*> &channels);
    void curvesCountChanged(); //<- MainWindow::updateActions()
    void channelPlotted(Channel *ch);
    void curveDeleted(Channel *);
    void focusThisPlot();
    void saveHorizontalSlice(double zVal);
    void saveVerticalSlice(double xVal);
    void saveTimeSegment(const QVector<FileDescriptor*> &files, double from, double to);
    void needClearPlot();
public slots:
    void updateLegends();
private slots:

private:
    void exportSonogramToExcel(bool fullRange, bool dataOnly);
    QGridLayout *plotsLayout;
    Plot *m_plot = nullptr;
    QLabel * infoLabel;

    QAction *toolBarAction = nullptr;

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
    QAction *cursorBoxAct = nullptr;
    QAction *copyToClipboardAct = nullptr;
    QAction *printPlotAct = nullptr;
    QAction *interactionModeAct = nullptr;
    QAction *playAct = nullptr;

    // QWidget interface
protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
};

#endif // PLOTAREA_H
