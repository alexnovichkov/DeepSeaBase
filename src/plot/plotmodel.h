#ifndef PLOTMODEL_H
#define PLOTMODEL_H

#include <QAbstractTableModel>

class Curve;
#include "fileformats/filedescriptor.h"

using CurvePredicate = bool (Curve *c);

class PlotModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    struct PlottedIndex
    {
        int channelIndex = -1;
        bool onLeft = true;
        Channel *ch = nullptr;
        int fileIndex = 0;
    };
    enum PlotModelColumn {
        PlotColorColumn=0,
        PlotTitleColumn,
        PlotAxisColumn,
        PlotCorrectionColumn,
        //PlotFileNumberColumn,
        PlotColumnsCount
    };

public:
    PlotModel(QObject *parent = nullptr);

    bool isEmpty() const;
    int size(int type=-1) const;

    Curve *firstOf(CurvePredicate predicate);

    const QList<Curve*> &curves() const {return m_curves;}
    QList<Curve *> curves(CurvePredicate predicate) const;
    Curve *selectedCurve() const;

    QVector<Channel *> plottedChannels() const;
//    QMap<FileDescriptor *, QVector<int> > plottedDescriptors() const;

    /**
     * @brief plottedDescriptors возвращает список записей, каналы которых построены
     * в текущей модели
     * @return
     */
    QVector<FileDescriptor *> plottedDescriptors() const;

    /**
     * @brief plottedIndexesForDescriptor возвращает список индексов построенных каналов
     * для записи d
     * @param d
     * @return
     */
    QVector<int> plottedIndexesForDescriptor(FileDescriptor *d) const;

    /**
     * @brief plottedIndexes возвращает вспомогательный список построенных каналов,
     * использующийся для быстрой навигации по построенным записям (режим Сергея)
     * @return
     */
    QVector<PlottedIndex> plottedIndexes() const {return m_plotted;}
    Descriptor::DataType curvesDataType() const;
    Curve * plotted(Channel *channel) const;
    Curve * plotted(FileDescriptor *descriptor) const;
    bool allCurvesFromSameDescriptor() const;

    Curve *curve(int index, bool left) const;
    Curve *curve(int index) const;

    int leftCurvesCount() const {return m_leftCurves.size();}
    int rightCurvesCount() const {return m_rightCurves.size();}

    void clear(bool forceDeleteFixed = false);
    void updatePlottedIndexes();
    void updatePlottedIndexes(FileDescriptor *d, int fileIndex);
    void cycleChannels(bool up);
    void setYValuesPresentation(bool m_leftCurves, int presentation);
    void removeLabels();
    void updateTitles();
    void resetHighlighting();
    void setTemporaryCorrection(Channel *ch, double correctionValue, int correctionType);

    //returns true if deleted
    void addCurve(Curve *curve, bool onLeft);
    bool deleteCurve(Curve *curve, bool *removedFromLeft=nullptr);
    bool moveToOtherAxis(Curve *curve);
private:
    void checkDuplicates(const QString name);
    QList<Curve *> m_curves;
    QList<Curve *> m_leftCurves;
    QList<Curve *> m_rightCurves;
    //Этот список хранит индексы каналов, которые имеют графики. Список обновляется при добавлении
    //или удалении кривых
    QVector<PlottedIndex> m_plotted;

    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
};

#endif // PLOTMODEL_H
