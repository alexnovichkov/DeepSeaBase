#ifndef PLAYPANEL_H
#define PLAYPANEL_H

#include <QWidget>
#include <QAudio>

class Plot;
class QwtPlotMarker;
class QToolButton;
class QSlider;
class QAudioOutput;
class DataIODevice;
class Channel;
class TrackingCursor;

class PlayPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PlayPanel(Plot *parent=0);
    ~PlayPanel();
    void switchVisibility();
    void reset();
signals:
    void closeRequested();

private slots:
    void audioStateChanged(QAudio::State state);
    void audioPosChanged();
    void start();
    void stop();
    void pause();

public slots:
    void update();
    void updateSelectedCursor(QwtPlotMarker *cursor);
    void setXValue(QwtPlotMarker *c, double xVal);
    // QWidget interface
protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void hideEvent(QHideEvent *event);
private:
    Channel *ch;
    Plot *plot;
    TrackingCursor *cursor;
    QToolButton *playButton;
    QToolButton *stopButton;
    QToolButton *pauseButton;
    QToolButton *muteButton;
    QSlider *volumeSlider;

    QAudioOutput* audio;
    DataIODevice *audioData;
    double initialPos; // начальная позиция проигрывания
};

#endif // PLAYPANEL_H
