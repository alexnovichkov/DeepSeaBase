#ifndef RESAMPLER_H
#define RESAMPLER_H

#include "samplerate.h"
#include <QVector>
#include <QString>

class Resampler
{
public:
    explicit Resampler() : src_state(0), m_factor(1.0), m_bufferSize(0) {
    }
    explicit Resampler(double factor, int bufferSize) : m_factor(factor), m_bufferSize(bufferSize)
    {
        init();
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

    void reset() {
        if (src_state)
            src_delete(src_state);
    }

    void init() {
        src_state = src_new(0, 1, &_error);
        src_data.src_ratio = 1.0 / m_factor;
        src_data.end_of_input = 0;

        const int newBlockSize = m_bufferSize; //* m_factor;

        filterOut.resize(newBlockSize+100);
        src_data.output_frames = filterOut.size();
        src_data.data_out = filterOut.data();

        src_data.input_frames = 0;
    }

    void setFactor(double factor) {
        if (factor < 1.0 || qFuzzyCompare(factor, m_factor)) return;
        m_factor = factor;
    }

    double factor() const {return m_factor;}

    void setBufferSize(int bufferSize) {
        if (bufferSize <1 || bufferSize == m_bufferSize) return;
        m_bufferSize = bufferSize;
    }


    QVector<float> process(QVector<float> &chunk)
    {
        if (m_factor > 1.0) {//do filtration
            src_data.data_in = chunk.data();
            src_data.input_frames = chunk.size();

            _error = src_process(src_state, &src_data);
            return filterOut.mid(0, src_data.output_frames_gen);
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

    double m_factor; // 1, 2, 4, 8, 16 etc.
    int m_bufferSize;
};

#endif // RESAMPLER_H

