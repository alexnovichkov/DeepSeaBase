#ifndef PLOTAREA_H
#define PLOTAREA_H

#include <QPointer>

#include "DockWidget.h"
#include "enums.h"

class QAction;
class Curve;
class FileDescriptor;
class QGridLayout;
class Channel;
class QLabel;
class QCPPlot;
class QStackedLayout;

class PlotArea : public ads::CDockWidget
{
    Q_OBJECT
public:
    PlotArea(int index, QWidget *parent);
    ~PlotArea();
    QCPPlot* plot();
    void addPlot(Enums::PlotType type);

    /**
     * @brief getPlotType
     * @param channels список каналов, которые планируется построить в новом графике
     * @return тип графика, подходящий для каналов channels
     */
    static Enums::PlotType getPlotType(const QVector<Channel*> &channels);

    /**
     * @brief plotTypesCompatible используется для определения возможности построения
     * каналов другого типа на уже имеющемся графике
     * @param first
     * @param second
     * @return true если типы совместимы. Пока что только Enums::PlotType::General
     * и Enums::PlotType::Octave считаются совместимыми.
     */
    static bool plotTypesCompatible(Enums::PlotType first, Enums::PlotType second);

    void update();

    /**
     * @brief exportToExcel экспортирует кривые в Excel и запускает его
     * @param fullRange если true, то будут экспортированы все данные. Если false,
     * то будут экспортированы только данные в пределах диапазона осей графика
     * @param dataOnly если true, то графики не экспортируются.
     */
    void exportToExcel(bool fullRange, bool dataOnly);
    void updateActions(int filesCount, int channelsCount);
    void deleteCurvesForDescriptor(FileDescriptor *f, const QVector<int> &indexes = QVector<int>());
    void deleteAllCurves();

    /**
     * @brief replotDescriptor вызывается при переходе на предыдущую/следующую запись
     * @param f
     * @param fileIndex
     */
    void replotDescriptor(FileDescriptor *f, int fileIndex);

    /**
     * @brief addCorrection добавляет коррекцию к построенным каналам в отдельном диалоговом окне.
     * @param additionalFiles дополнительные файлы, к которым нужно применить коррекцию
     * помимо построенных каналов.
     */
    void addCorrection(const QList<FileDescriptor*> &additionalFiles);

    /**
     * @brief plottedChannels возвращает список каналов, для которых построены графики
     * @return список
     */
    QVector<Channel*> plottedChannels() const;

    /**
     * @brief firstVisible возвращает первый видимый канал
     * @return
     */
    Channel* firstVisible() const;

    /**
     * @brief plottedDescriptors возвращает массив записей с
     * @return
     */
    QMap<FileDescriptor *, QVector<int> > plottedDescriptors() const;

    /**
     * @brief curvesCount возвращает количество построенных каналов определенного типа
     * @param type тип канала согласно типам UFF
     * @return
     */
    int curvesCount(int type=-1) const;

    /**
     * @brief resetCycling вызывается для сброса состояния навигации по произвольным записям
     */
    void resetCycling();
signals:
    /**
     * @brief descriptorRequested испускается по запросу от действий previousDescriptorAct
     * и nextDescriptorAct в mainWindow
     * @param direction 1 для следующей записи, -1 для предыдущей записи
     * @param checked состояние режима Сергея
     */
    void descriptorRequested(int direction, bool checked);

    //redirected from m_plot
    void needPlotChannels(bool plotOnLeft, const QVector<Channel*> &channels);
    void curvesCountChanged(); //<- MainWindow::updateActions()
    void channelPlotted(Channel *ch);
    void curveDeleted(Channel *);
    void focusThisPlot();
    void saveHorizontalSlice(const QVector<double>& zVal);
    void saveVerticalSlice(const QVector<double>& xVal);
    void saveTimeSegment(const QVector<FileDescriptor*> &files, double from, double to);
public slots:
    void updateLegends();
private slots:

private:
    void exportSonogramToExcel(bool fullRange, bool dataOnly);
//    QGridLayout *plotsLayout;
    QSplitter *splitter;
    QPointer<QCPPlot> m_plot;
    QLabel * infoLabel;
    QStackedLayout *stackedL;

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
