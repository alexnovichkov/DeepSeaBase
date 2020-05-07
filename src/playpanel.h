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
    void enable(bool enabled);

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
    void positionChanged(qint64 progress);
    void statusChanged(QMediaPlayer::MediaStatus status);
    void displayErrorMessage();

    void setSource(int n);

public slots:
    void update();
    void updateSelectedCursor(QwtPlotMarker *cursor);
    void setXValue(double xVal);

protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void hideEvent(QHideEvent *event);
private:
    void moveCursor(const double xVal);
    void prepareDataToPlay();

    Channel *ch = 0;
    QMap<Channel*,QString> wavFiles;
    Plot *plot;
    TrackingCursor *cursor;
    QComboBox *channelsBox;

    QMediaPlayer *player;
    PlayerControls *controls;

    QString oldTempFile;
};

#endif // PLAYPANEL_H
