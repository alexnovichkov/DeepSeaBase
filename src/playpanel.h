#ifndef PLAYPANEL_H
#define PLAYPANEL_H

#include <QWidget>
#include <QAudio>
#include <QMap>

class Plot;
class QwtPlotMarker;
class QToolButton;
class QSlider;
class QAudioOutput;
class DataIODevice;
class Channel;
class TrackingCursor;
class QComboBox;


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
    void setSource(int n);
    void mute();
    void setVolume(int vol);

public slots:
    void update();
    void updateSelectedCursor(QwtPlotMarker *cursor);
    void setXValue(double xVal);
    // QWidget interface
protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void hideEvent(QHideEvent *event);
private:
    void moveCursor(const double xVal);
    void muteVolume(bool mute);
    bool muted() const;
    Channel *ch;
    Plot *plot;
    TrackingCursor *cursor;
    QToolButton *playButton;
    QToolButton *stopButton;
    QToolButton *pauseButton;
    QToolButton *muteButton;
    QComboBox *channelsBox;
    QMap<int, Channel*> channels;
    QSlider *volumeSlider;

    QAudioOutput* audio;
    DataIODevice *audioData;
    double initialPos; // начальная позиция проигрывания
};

#endif // PLAYPANEL_H
