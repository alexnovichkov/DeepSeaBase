#ifndef PLOTMODEL_H
#define PLOTMODEL_H

#include <QAbstractTableModel>

class Curve;
class Channel;
#include "fileformats/filedescriptor.h"

using CurvePredicate = bool (Curve *c);

class PlotModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    struct PlottedIndex
    {
        int index = -1;
        bool onLeft = true;
    };
    struct Cycled
    {
        Channel *ch = nullptr;
        bool onLeft = true;
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

    QVector<Channel *> plottedChannels() const;
    QVector<FileDescriptor*> plottedDescriptors() const;
    QVector<PlottedIndex> plottedIndexes() const {return m_plotted;}
    //Этот список хранит каналы, которые были построены на момент первого сдвига
    QVector<Cycled> cycled() const {return m_cycled;}
    Descriptor::DataType curvesDataType() const;
    Curve * plotted(Channel *channel) const;
    bool allCurvesFromSameDescriptor() const {return !m_plotted.isEmpty();}

    Curve *curve(int index, bool left) const;
    Curve *curve(int index) const;

    int leftCurvesCount() const {return m_leftCurves.size();}
    int rightCurvesCount() const {return m_rightCurves.size();}

    void clear(bool forceDeleteFixed = false);
    void updatePlottedIndexes();
    void updateCycled();
    void cycleChannels(bool up);
    void setYValuesPresentation(bool m_leftCurves, int presentation);
    void removeLabels();
    void updateTitles();
    void resetHighlighting();
    void setTemporaryCorrection(int index, double correctionValue, int correctionType);

    //returns true if deleted
    void addCurve(Curve *curve, bool onLeft);
    bool deleteCurve(Curve *curve, bool *removedFromLeft=nullptr);
    bool moveToOtherAxis(Curve *curve);
private:
    void checkDuplicates(const QString name);
    QList<Curve *> m_curves;
    QList<Curve *> m_leftCurves;
    QList<Curve *> m_rightCurves;
    //Этот список хранит индексы каналов, которые имеют графики, в том случае,
    //если все эти каналы - из одной записи. Список обновляется при добавлении
    //или удалении кривых
    QVector<PlottedIndex> m_plotted;
    //Этот список хранит каналы, которые были построены на момент первого сдвига
    QVector<Cycled> m_cycled;

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
