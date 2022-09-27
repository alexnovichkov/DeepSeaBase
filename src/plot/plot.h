#ifndef PLOT_H
#define PLOT_H

//#include <qwt_plot.h>
//#include <qwt_plot_dict.h>
#include <math.h>
#include "colorselector.h"

#include <QObject>

class PlotInterface;
class Curve;
class Channel;
class FileDescriptor;
class ZoomStack;
class PlotTracker;
class QAction;
class Picker;
class QPrinter;
class PlotModel;
class QMenu;
class Cursors;
class CursorBox;
class Selectable;


#include <QWidget>

#include "enums.h"



class Plot : public QObject
{
    Q_OBJECT
public:
    explicit Plot(Enums::PlotType type, QWidget *parent = 0);
    virtual ~Plot();

    PlotModel *model() {return m;}
    Enums::PlotType type() const {return plotType;}
    QWidget *widget() const;
    PlotInterface *impl() const;

    double screenToPlotCoordinates(Enums::AxisType axis, double value);
    double plotToScreenCoordinates(Enums::AxisType axis, double value);
    Range plotRange(Enums::AxisType axis);
    Range screenRange(Enums::AxisType axis);

    virtual QWidget *toolBarWidget() {return nullptr;}
    virtual void updateActions(int filesCount, int channelsCount) {Q_UNUSED(filesCount); Q_UNUSED(channelsCount);}

    bool sergeiMode = false;
    bool xScaleIsLogarithmic = false; //false = linear, true = logarithmic

    Enums::InteractionMode interactionMode = Enums::InteractionMode::ScalingInteraction;

    bool hasCurves() const;
    int curvesCount(int type=-1) const;

    QString axisTitleText(Enums::AxisType id) const;

    virtual bool canBePlottedOnLeftAxis(Channel *ch, QString *message=nullptr) const;
    virtual bool canBePlottedOnRightAxis(Channel *ch, QString *message=nullptr) const;

    void focusPlot();

    //default implementation returns pos as QwtText,
    //reimplemented in spectrograms to add Z coordinate
    virtual QString pointCoordinates(const QPointF &pos);

    //default implementation returns LineCurve
    //reimplemented in other plot types
    virtual Curve * createCurve(const QString &legendName, Channel *channel);
    virtual void deleteCurve(Curve *curve, bool doReplot = true);
    //default implementation updates labels according to curves count on left and on right
    //reimplemented in spectrograms
    virtual void updateAxesLabels();
    //reimplemented in plot types
    virtual void plotChannel(Channel * ch, bool plotOnLeft, int fileIndex=0);

    virtual void onDropEvent(bool plotOnLeft, const QVector<Channel *> &channels);
    void update();

    void updatePlottedIndexes();
    void plotCurvesForDescriptor(FileDescriptor *d, int fileIndex=0);

    void cycleChannels(bool up);

//    void updateTrackingPanel();

    void deleteCurveFromLegend(Curve *curve);
    void deleteCurvesForDescriptor(FileDescriptor *descriptor);
    void deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot = true);
    void deleteSelectedCurve(Selectable *selected);

    void switchLabelsVisibility();
    void setAxis(Enums::AxisType axis, const QString &name);

    void setScale(Enums::AxisType id, double min, double max, double step = 0);
    void removeLabels();
    void setInteractionMode(Enums::InteractionMode mode);
    void switchInteractionMode();
    void switchTrackingCursor();
    void toggleAutoscale(Enums::AxisType axis, bool toggled);
    void autoscale(Enums::AxisType axis = Enums::AxisType::atInvalid);
    /**
     * @brief recalculateScale пересчитывает границы графиков,
     * отдельно для левой или правой вертикальной оси
     * @param leftAxis
     */
    void recalculateScale(bool leftAxis);

    void saveSpectrum(double zVal);
    void saveThroughput(double xVal);

    void updateLabels();
    void removeCursor(Selectable *selected);
    void showContextMenu(const QPoint &pos, Enums::AxisType axis);
    void editLegendItem(Curve *curve);

    ZoomStack *zoom = nullptr;
    Picker *picker = nullptr;
protected:
    PlotInterface *m_plot = nullptr;

    PlotModel *m = nullptr;

    QString xName;
    QString yLeftName;
    QString yRightName;
    bool axisLabelsVisible = true;
    int yValuesPresentationLeft;
    int yValuesPresentationRight;

    PlotTracker *tracker = nullptr;

    ColorSelector *colors = nullptr;
    Cursors *cursors = nullptr;
    CursorBox *cursorBox;

    //default implementation updates either left or right axis
    //reimplemented in Spectrogram
    virtual void updateBounds();
    //default implementation does nothing
    //reimplemented in spectrogram
    virtual void setRightScale(Enums::AxisType id, double min, double max);
    virtual QMenu *createMenu(Enums::AxisType axis, const QPoint &pos);

    QString yValuesPresentationSuffix(int yValuesPresentation) const;
public slots:
    void savePlot();
    void switchCursor();
    void copyToClipboard();
    void print();
    void updateLegends();

    void moveCurve(Curve *curve, Enums::AxisType axis);
    void deleteAllCurves(bool forceDeleteFixed = false);
signals:
    //испускаем, когда удаляем или добавляем графики
    void curvesCountChanged();//->MainWindow.updateActions
    void focusThisPlot();
    //испускаем, когда добавляем график
    void channelPlotted(Channel *ch); //<- MainWindow::addPlotArea
    //испускаем, когда удаляем кривую
    void curveDeleted(Channel *);

    void trackingPanelCloseRequested();
    void saveTimeSegment(const QVector<FileDescriptor*> &files, double from, double to);
    void saveHorizontalSlice(double zVal);
    void saveVerticalSlice(double xVal);
    //испускаем, когда бросаем каналы на график
    void needPlotChannels(bool plotOnLeft, const QVector<Channel*> &channels);
private slots:


private:
    QColor getNextColor();

    Enums::PlotType plotType = Enums::PlotType::General;

//    // QWidget interface
//protected:
//    void dragEnterEvent(QDragEnterEvent *event) override;
//    void dragMoveEvent(QDragMoveEvent *event) override;
//    void dragLeaveEvent(QDragLeaveEvent *event) override;
};

#endif // PLOT_H
