#ifndef RESAMPLER_H
#define RESAMPLER_H

#include "samplerate.h"
#include <QVector>
#include <QString>
#include <QDebug>

class Resampler
{
public:
    explicit Resampler() : src_state(0), m_factor(1.0), m_bufferSize(0) {
    }
    explicit Resampler(double factor, int bufferSize) : src_state(0), m_factor(factor), m_bufferSize(bufferSize)
    {
        init(0);
    }

    virtual ~Resampler()
    {
        if (src_state)
            src_delete(src_state);
    }

    /**
     * @brief setLastChunk устанавливает флаг,
     * что сейчас будет последняя порция данных
     */
    void setLastChunk() {
        src_data.end_of_input = 1;
    }

//    void reset() {
//        if (src_state)
//            src_delete(src_state);
//        src_state = 0;
//    }

    void init(size_t size) {
        src_data.src_ratio = 1.0/m_factor;
        src_data.end_of_input = 0;

        if (!src_is_valid_ratio(1.0/m_factor)) {
            qDebug()<<"Ratio"<<m_factor<<"is not valid";
        }

        if (size > 0) {
            filterOut.resize(double(size) / m_factor + 100);
            src_data.output_frames = filterOut.size();
            src_data.data_out = filterOut.data();
        }
    }

    void setFactor(double factor) {
        if (factor < 1.0 || qFuzzyCompare(factor+1.0, m_factor+1.0)) return;
        m_factor = factor;
    }

    double factor() const {return m_factor;}

    void setBufferSize(int bufferSize) {
        if (bufferSize <1 || bufferSize == m_bufferSize) return;
        m_bufferSize = bufferSize;
    }


    QVector<float> process(QVector<float> &chunk)
    {
        if (m_factor != 1.0) {//do filtration
            src_data.data_in = chunk.data();
            src_data.input_frames = chunk.size();

            _error = src_simple(&src_data, SRC_SINC_BEST_QUALITY, 1);
            return filterOut.mid(0, src_data.output_frames_gen);
        }
        else {
            return chunk;
        }
    }

    QVector<double> process(const QVector<double> &chunk)
    {
        QVector<float> chunk1(chunk.size());
        std::copy(chunk.constBegin(), chunk.constEnd(), chunk1.begin());
        QVector<float> result1 = process(chunk1);
        QVector<double> result(result1.size());
        std::copy(result1.constBegin(), result1.constEnd(), result.begin());
        return result;
    }

    QString error() const {return src_strerror(_error);}
private:
    SRC_DATA src_data;
    SRC_STATE *src_state;
    QVector<float> filterOut;
    int _error;

    double m_factor; // 1, 2, 4, 8, 16 etc.
    int m_bufferSize;
};

#endif // RESAMPLER_H

