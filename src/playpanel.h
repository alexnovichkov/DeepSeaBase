#ifndef PLAYPANEL_H
#define PLAYPANEL_H

#include <QWidget>
#include <QAudio>
#include <QMap>
#include <QMediaPlayer>

class Plot;
class QwtPlotMarker;
class QToolButton;
class QSlider;
class QAudioOutput;
class DataIODevice;
class Channel;
class TrackingCursor;
class QComboBox;
class QBuffer;
class QFile;

class QAbstractButton;
class QAbstractSlider;

class PlayerControls : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerControls(QWidget *parent = nullptr);

    QMediaPlayer::State state() const;
    int volume() const;
    bool isMuted() const;

public slots:
    void setState(QMediaPlayer::State state);
    void setVolume(int volume);
    void setMuted(bool muted);

signals:
    void play();
    void pause();
    void stop();
    void changeVolume(int volume);
    void changeMuting(bool muting);

private slots:
    void playClicked();
    void muteClicked();
    void onVolumeSliderValueChanged();

private:
    QMediaPlayer::State m_playerState = QMediaPlayer::StoppedState;
    bool m_playerMuted = false;
    QAbstractButton *m_playButton = nullptr;
    QAbstractButton *m_stopButton = nullptr;
    QAbstractButton *m_muteButton = nullptr;
    QAbstractSlider *m_volumeSlider = nullptr;
};


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
    void durationChanged(qint64 duration);
    void positionChanged(qint64 progress);
    void metaDataChanged();
    void statusChanged(QMediaPlayer::MediaStatus status);
    void stateChanged(QMediaPlayer::State state);
    void bufferingProgress(int progress);
    void audioAvailableChanged(bool available);
    void displayErrorMessage();

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
    QComboBox *channelsBox;
    QMap<int, Channel*> channels;

    QMediaPlayer *player;

    QString oldTempFile;

    double initialPos; // начальная позиция проигрывания
};

#endif // PLAYPANEL_H
