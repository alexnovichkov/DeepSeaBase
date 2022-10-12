#ifndef DATA2D_H
#define DATA2D_H

#include "qcustomplot.h"

#include "dataholder.h"

class Data2D {
public:
    explicit Data2D(DataHolder *data) : data(data) {}
    virtual ~Data2D() {}
    inline bool isEmpty() const {
        return data->samplesCount() == 0;
    }
    inline int size() const {
        return data->samplesCount();
    }

    /*!
      Returns an index of the data point with a (sort-)key that is equal to, just below, or just
      above \a sortKey. If \a expandedRange is true, the data point just below \a sortKey will be
      considered, otherwise the one just above.

      This can be used in conjunction with \ref findEnd to iterate over data points within a given key
      range, including or excluding the bounding data points that are just beyond the specified range.

      If \a expandedRange is true but there are no data points below \a sortKey, \ref constBegin is
      returned.

      If the container is empty, returns \ref constEnd.

      \see findEnd
    */
    int findBegin(double sortKey, bool expandedRange=true) const;

    /*!
      Returns an index of the element after the data point with a (sort-)key that is equal to, just
      above or just below \a sortKey. If \a expandedRange is true, the data point just above \a sortKey
      will be considered, otherwise the one just below.

      This can be used in conjunction with \ref findBegin to iterate over data points within a given
      key range, including the bounding data points that are just below and above the specified range.

      If \a expandedRange is true but there are no data points above \a sortKey, -1 is
      returned.

      If the container is empty, -1 is returned.

      \see findBegin
    */
    int findEnd(double sortKey, bool expandedRange=true) const;

    void limitIteratorsToDataRange(int &begin, int &end, const QCPDataRange &dataRange) const;

    inline double mainKey(int index) {
        return data->xValue(index);
    }
    inline double sortKey(int index) {
        return data->xValue(index);
    }
    inline double mainValue(int index) {
        return data->yValue(index, 0);
    }
    inline QCPRange valueRange(int index) const {
        return {data->yValue(index), data->yValue(index)};
    }
    QCPRange valueRange(bool &foundRange, QCP::SignDomain signDomain,
                                const QCPRange &inKeyRange) const;

    QCPRange keyRange(bool &foundRange, QCP::SignDomain signDomain) const;

    QVector<QCPGraphData> toLineData(int begin, int end) const;
private:
    DataHolder *data = nullptr;
};

#endif // DATA2D_H
