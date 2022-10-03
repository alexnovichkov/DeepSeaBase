#include "data2d.h"
#include "logging.h"

int Data2D::findBegin(double sortKey, bool expandedRange) const
{
    if (isEmpty()) return -1;

    int from = -1;

    if (data->xValuesFormat()==DataHolder::XValuesUniform) {
        const auto min = data->xMin();
        const auto step = data->xStep();
        if (!qFuzzyIsNull(step)) {
            from = qCeil((sortKey - min)/step);
        }
    }
    else {
        for (int i=0; i < data->samplesCount(); ++i) {
            if (data->xValue(i) >= sortKey) {
                from = i;
                break;
            }
        }
    }
    if (from > 0 && expandedRange) from--;
    return from;
}

int Data2D::findEnd(double sortKey, bool expandedRange) const
{
    if (isEmpty()) return -1;

    int from = -1;

    if (data->xValuesFormat()==DataHolder::XValuesUniform) {
        const auto min = data->xMin();
        const auto step = data->xStep();
        if (!qFuzzyIsNull(step)) {
            from = qFloor((sortKey - min)/step);
        }
    }
    else {
        for (int i=0; i < data->samplesCount(); ++i) {
            if (data->xValue(i) > sortKey) {
                from = i;
                break;
            }
        }
    }
    if (from != -1 && expandedRange) from++;
    return from;
}

void Data2D::limitIteratorsToDataRange(int &begin, int &end, const QCPDataRange &dataRange) const
{
    begin = std::clamp(dataRange.begin(), 0, data->samplesCount()-1);
    end = std::clamp(dataRange.end(), 0, data->samplesCount()-1);
    if (begin > end) std::swap(begin, end);
}

QCPRange Data2D::valueRange(bool &foundRange, QCP::SignDomain signDomain, const QCPRange &inKeyRange) const
{
    if (isEmpty()) {
        foundRange = false;
        return QCPRange();
    }

    if (inKeyRange == QCPRange()) return {data->yMin(0), data->yMax(0)};

    int begin = findBegin(inKeyRange.lower, false);
    int end = findEnd(inKeyRange.upper, false);
    auto range = data->yMinMax(begin, end);

    switch (signDomain) {
        case QCP::sdBoth: {
            foundRange = true;
            break;
        }
        case QCP::sdPositive: {
            if (range.second < 0) {
                foundRange = false;
                return QCPRange();
            }
            else {
                foundRange = true;
                range.first = 0;
            }
            break;
        }
        case QCP::sdNegative: {
            if (range.first > 0) {
                foundRange = false;
                return QCPRange();
            }
            else {
                foundRange = true;
                range.second = 0;
            }
            break;
        }

    }

    return {range.first, range.second};
}

QCPRange Data2D::keyRange(bool &foundRange, QCP::SignDomain signDomain) const
{
    QCPRange result;
    if (isEmpty()) {
        foundRange = false;
    }

    else switch (signDomain) {
        case QCP::sdBoth: {
            foundRange = true;
            result = {data->xMin(), data->xMax()};
            break;
        }
        case QCP::sdPositive: {
            if (data->xMax() < 0) {
                foundRange = false;
                result = QCPRange();
            }
            else {
                foundRange = true;
                result = {0, data->xMax()};
            }
            break;
        }
        case QCP::sdNegative: {
            if (data->xMin() > 0) {
                foundRange = false;
                result = QCPRange();
            }
            else {
                foundRange = true;
                result = {data->xMin(), 0};
            }
            break;
        }

    }
    return result;
}

QVector<QCPGraphData> Data2D::toLineData() const
{DD0;
    QVector<QCPGraphData> result(data->samplesCount());

    for (int i=0; i<data->samplesCount(); ++i) {
        result[i] = {data->xValue(i), data->yValue(i, 0)};
    }
    return result;
}
