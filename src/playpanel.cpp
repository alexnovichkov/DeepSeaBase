#include "playpanel.h"
#include "plot.h"
#include "qwt_plot_marker.h"
#include "filedescriptor.h"

#include <QtWidgets>
#include <QAudioOutput>
#include "dataiodevice.h"

PlayPanel::PlayPanel(Plot *parent) : QWidget(parent), plot(parent)
{
    setWindowFlags(Qt::Tool /*| Qt::WindowTitleHint*/);
    setWindowTitle("Проигрыватель");

    cursor = new QwtPlotMarker();
    cursor->setLineStyle( QwtPlotMarker::VLine );
    cursor->setLinePen( Qt::green, 0, Qt::SolidLine );

    playButton = new QToolButton(this);
    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    stopButton = new QToolButton(this);
    stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    pauseButton = new QToolButton(this);
    pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    muteButton = new QToolButton(this);
    muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));

    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);

    QHBoxLayout *l = new QHBoxLayout(this);
    l->addWidget(playButton);
    l->addWidget(stopButton);
    l->addWidget(pauseButton);
    l->addWidget(muteButton);
    l->addWidget(volumeSlider);
    setLayout(l);
}

PlayPanel::~PlayPanel()
{
    cursor->detach();
    delete cursor;
}

void PlayPanel::switchVisibility()
{
    if (isVisible()) {
        setVisible(false);
        cursor->setVisible(false);
    }
    else {
        setVisible(true);
        cursor->setVisible(true);
//        update();
    }
}

void PlayPanel::reset()
{
    ch = 0;

}

void PlayPanel::audioStateChanged(QAudio::State state)
{

}

void PlayPanel::audioPosChanged()
{

}

void PlayPanel::start()
{
    if (!ch) return;

    if (ch->type()!=Descriptor::TimeResponse) return;

    if (audio) {
        audio->stop();
        delete audio;
        audio = 0;
    }

    if (audioData) {
        delete audioData;
        audioData = 0;
    }

    audioData = new DataIODevice(ch, this);
    audioData->open(QIODevice::ReadOnly);

    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(qRound(1.0/ch->xStep()));
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        return;
    }

    audio = new QAudioOutput(format, this);
    audio->setBufferSize(2 * qRound(1.0/ch->xStep()));
    audio->setNotifyInterval(1000);

    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
    connect(audio, SIGNAL(notify()), this, SLOT(audioPosChanged()));
    audio->start(audioData);
    audio->setVolume(0.01);
}

void PlayPanel::stop()
{

}

void PlayPanel::pause()
{

}

void PlayPanel::closeEvent(QCloseEvent *event)
{
    emit closeRequested();
    QWidget::closeEvent(event);
}

void PlayPanel::hideEvent(QHideEvent *event)
{
    cursor->setVisible(false);
    QWidget::hideEvent(event);
}
