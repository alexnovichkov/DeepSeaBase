#ifndef RESAMPLER_H
#define RESAMPLER_H

#include "samplerate.h"
#include <QVector>
#include <QString>

class Resampler
{
public:
    explicit Resampler() : src_state(0), m_factor(1), m_bufferSize(0) {
    }
    explicit Resampler(int factor, int bufferSize) : m_factor(factor), m_bufferSize(bufferSize)
    {
        init();
    }

    virtual ~Resampler()
    {
        if (src_state)
            src_delete(src_state);
    }

    void reset() {
        if (src_state)
            src_delete(src_state);
    }

    void init() {
        src_state = src_new(2, 1, &_error);
        src_data.src_ratio = 1.0 / m_factor;
        src_data.end_of_input = 0;

        const int newBlockSize = m_bufferSize * m_factor;

        filterOut.resize(newBlockSize+100);
        src_data.output_frames = filterOut.size();
        src_data.data_out = filterOut.data();
    }

    void setFactor(int factor) {
        if (factor < 1 || factor == m_factor) return;
        m_factor = factor;
    }

    int factor() const {return m_factor;}

    void setBufferSize(int bufferSize) {
        if (bufferSize <1 || bufferSize == m_bufferSize) return;
        m_bufferSize = bufferSize;
    }


    QVector<float> process(QVector<float> &chunk)
    {
        if (m_factor > 1) {//do filtration
            src_data.data_in = chunk.data();
            src_data.input_frames = chunk.size();

            _error = src_process(src_state, &src_data);
            return filterOut.mid(0, m_bufferSize/m_factor);
        }
        else {
            return chunk;
        }
    }

    QVector<double> process(const QVector<double> &chunk)
    {
        QVector<float> chunk1(chunk.size());
        std::copy(chunk.begin(), chunk.end(), chunk1.begin());
        QVector<float> result1 = process(chunk1);
        QVector<double> result(result1.size());
        std::copy(result1.begin(), result1.end(), result.begin());
        return result;
    }

    QString error() const {return src_strerror(_error);}
private:
    SRC_DATA src_data;
    SRC_STATE *src_state;
    QVector<float> filterOut;
    int _error;

    int m_factor; // 1, 2, 4, 8, 16 etc.
    int m_bufferSize;
};

#endif // RESAMPLER_H
