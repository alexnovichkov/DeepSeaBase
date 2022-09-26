#ifndef ABSTRACTDATA2D_H
#define ABSTRACTDATA2D_H

#include "plot/qcustomplot.h"

class Data {
public:
    virtual ~Data() {}
    virtual bool isEmpty() const = 0;
    virtual int size() const = 0;

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
    virtual int findBegin(double sortKey, bool expandedRange=true) const = 0;

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
    virtual int findEnd(double sortKey, bool expandedRange=true) const = 0;

    virtual void limitIteratorsToDataRange(int &begin, int &end, const QCPDataRange &dataRange) const = 0;

    virtual double mainKey(int index) = 0;
    virtual double sortKey(int index) = 0;
    virtual double mainValue(int index) = 0;
    virtual QCPRange valueRange(int index) const = 0;
    virtual QCPRange valueRange(bool &foundRange, QCP::SignDomain signDomain,
                                const QCPRange &inKeyRange) const = 0;

    virtual QCPRange keyRange(bool &foundRange, QCP::SignDomain signDomain) const = 0;

    virtual QVector<QCPGraphData> toLineData() const = 0;
};

#endif // ABSTRACTDATA2D_H
