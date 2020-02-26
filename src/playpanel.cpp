#include "playpanel.h"
#include "plot.h"
#include "qwt_plot_marker.h"
#include "filedescriptor.h"

#include <QtWidgets>
#include <QAudioOutput>
#include "dataiodevice.h"
#include "curve.h"
#include "trackingpanel.h"

PlayPanel::PlayPanel(Plot *parent) : QWidget(parent), plot(parent)
{
    setWindowFlags(Qt::Tool /*| Qt::WindowTitleHint*/);
    setWindowTitle("Проигрыватель");

    cursor = new TrackingCursor(Qt::green);
    cursor->showYValues = true;
    cursor->attach(plot);
    cursor->setAxes(QwtPlot::xBottom, QwtPlot::yLeft);
    cursor->setVisible(false);

    playButton = new QToolButton(this);
    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    stopButton = new QToolButton(this);
    stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    pauseButton = new QToolButton(this);
    pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    muteButton = new QToolButton(this);
    muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));

    connect(playButton,SIGNAL(pressed()),SLOT(start()));
    connect(stopButton,SIGNAL(pressed()),SLOT(stop()));
    connect(pauseButton,SIGNAL(pressed()),SLOT(pause()));

    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);

    QHBoxLayout *l = new QHBoxLayout(this);
    l->addWidget(playButton);
    l->addWidget(stopButton);
    l->addWidget(pauseButton);
    l->addWidget(muteButton);
    l->addWidget(volumeSlider);
    setLayout(l);

    ch = 0;
    audio = 0;
    audioData = 0;
    initialPos = 0.0;
    update();
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

void PlayPanel::update()
{
    // ищем канал, который сможем проиграть
    //ситуации: 1. график удалился - очищаем
    //          2. график поменялся - очищаем и меняем
    //          3. график не поменялся

    if (!ch) { // графика не было, ищем первый выделенный канал с временными данными
               // или просто первый временной
        Channel *firstTimeResponce = 0;
        foreach (Curve *c, plot->graphs) {
            if (c->channel->type() == Descriptor::TimeResponse) {
                firstTimeResponce = c->channel;
                if (c->highlighted) {
                    ch = c->channel;
                    break;
                }
            }
        }

        if (firstTimeResponce && !ch) { // не нашли выделенный канал, берем просто первый временной
            ch = firstTimeResponce;
        }
    }

    else {
        // график есть
        bool present = false;
        Channel *highlightedChannel = 0;
        foreach (Curve *c, plot->graphs) {
            if (c->highlighted) highlightedChannel = c->channel;

            if (c->channel == ch) {
                present = true;
            }
        }

        if (!present) {// график удалился - останавливаем
            reset();
            ch = 0;

            // ищем другой канал, который мы можем проиграть
            Channel *firstTimeResponce = 0;
            foreach (Curve *c, plot->graphs) {
                if (c->channel->type() == Descriptor::TimeResponse) {
                    firstTimeResponce = c->channel;
                    if (c->highlighted) {
                        ch = c->channel;
                        break;
                    }
                }
            }

            if (firstTimeResponce) { // не нашли выделенный канал, берем просто первый временной
                ch = firstTimeResponce;
            }
        }

        else { // график есть, но возможно уже не текущий
            if (highlightedChannel && (ch != highlightedChannel)) {
                reset();
                ch = highlightedChannel;
            }
        }
    }

    if (!ch) {
        playButton->setEnabled(false);
        stopButton->setEnabled(false);
        pauseButton->setEnabled(false);
        muteButton->setEnabled(false);
    }
    else {
        playButton->setEnabled(true);
        stopButton->setEnabled(true);
        pauseButton->setEnabled(true);
        muteButton->setEnabled(true);
    }
}

void PlayPanel::updateSelectedCursor(QwtPlotMarker *c)
{
    if (cursor == c) {
        cursor->setCurrent(true);
    }
    else {
        cursor->setCurrent(false);
    }
}

void PlayPanel::setXValue(QwtPlotMarker *c, double xVal)
{
    if (cursor != c) return;
    if (!ch) return;

    // здесь xVal - произвольное число, соответствующее какому-то положению на оси X
    moveCursor(xVal);

    if (audioData) {
        audioData->reset();
        audioData->seek(/*количество байт с начала*/xVal/ch->xStep()*sizeof(double));
    }
}

void PlayPanel::reset()
{
    if (audio) {
        audio->stop();
        delete audio;
        audio = 0;
    }
}

void PlayPanel::audioStateChanged(QAudio::State state)
{
    //qDebug()<<state;
}

void PlayPanel::audioPosChanged()
{
    const double xVal = audioData->positionSec();
    moveCursor(xVal);
}

void PlayPanel::start()
{
    if (!ch) return;

    if (ch->type()!=Descriptor::TimeResponse) return;

    if (audio) {
        if (audio->state() == QAudio::ActiveState) // already playing
            return;

        if (audio->state() == QAudio::SuspendedState) {
            audio->resume();
            return;
        }
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

    audio->setNotifyInterval(50);

    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
    connect(audio, SIGNAL(notify()), this, SLOT(audioPosChanged()));
    audio->start(audioData);
}

void PlayPanel::stop()
{
    if (audio) audio->stop();
    moveCursor(0);
}

void PlayPanel::pause()
{
    if (audio) audio->suspend();
}



void PlayPanel::closeEvent(QCloseEvent *event)
{
    cursor->setVisible(false);
    emit closeRequested();
    QWidget::closeEvent(event);
}

void PlayPanel::hideEvent(QHideEvent *event)
{
    cursor->setVisible(false);
    QWidget::hideEvent(event);
}

void PlayPanel::moveCursor(const double xVal)
{
    cursor->moveTo(xVal);
    //if (cursor->yValues.isEmpty()) cursor->yValues << xVal;
    //else cursor->yValues[0] = xVal;
    cursor->updateLabel();
}
