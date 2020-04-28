#include "playpanel.h"
#include "plot/plot.h"
#include "qwt_plot_marker.h"
#include "fileformats/filedescriptor.h"

#include <QtWidgets>
#include <QAudioOutput>
#include "dataiodevice.h"
#include "plot/curve.h"
#include "trackingpanel.h"

PlayPanel::PlayPanel(Plot *parent) : QWidget(parent), plot(parent)
{
    setWindowFlags(Qt::Tool /*| Qt::WindowTitleHint*/);
    setWindowTitle("Проигрыватель");

    cursor = new TrackingCursor(Qt::green);
    cursor->showYValues = true;
    cursor->attach(plot);
    cursor->setAxes(QwtAxis::xBottom, QwtAxis::yLeft);
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
    connect(muteButton,SIGNAL(pressed()),SLOT(mute()));

    channelsBox = new QComboBox(this);
    connect(channelsBox,SIGNAL(currentIndexChanged(int)),SLOT(setSource(int)));
    channelsBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(100);
    connect(volumeSlider,SIGNAL(valueChanged(int)),SLOT(setVolume(int)));

    QHBoxLayout *l = new QHBoxLayout(this);
    l->addWidget(channelsBox);
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

    //ищем номер предыдущего канала. Если он удален, то насильно вызваем setSource,
    //так как channelsBox нормально не обновится

    int oldIndex = -1;
    if (channels.values().contains(ch)) oldIndex = channels.key(ch);

    channels.clear();
    channelsBox->clear();

    for (int i=0; i<plot->curves.size(); ++i) {
        Channel *c = plot->curves[i]->channel;
        if (c->type() == Descriptor::TimeResponse) {
            channels.insert(i,c);
            QPixmap pix(10,10);
            pix.fill(plot->curves[i]->pen().color());
            channelsBox->addItem(QIcon(pix), c->name());
        }
    }

    if (channels.values().contains(ch)) {
        //такой канал уже есть
    }
    else if (ch) {
        //такой канал был, но удалился
        reset();
        ch = 0;
        if (!channels.keys().contains(oldIndex))
            setSource(oldIndex);
        else
            setSource(0);
    }

    playButton->setEnabled(!channels.isEmpty());
    stopButton->setEnabled(!channels.isEmpty());
    pauseButton->setEnabled(!channels.isEmpty());
    muteButton->setEnabled(!channels.isEmpty());
    volumeSlider->setEnabled(!channels.isEmpty());
}

void PlayPanel::setSource(int n)
{
    if (!channels.keys().contains(n)) return;

    if (ch && channels.key(ch)==n) return;

    ch = channels.value(n);
    if (audio) audio->stop();
    moveCursor(0);
}

void PlayPanel::mute()
{
    if (!audioData->muted()) {
        audioData->setMuted(true);
        muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
    }
    else {
        audioData->setMuted(false);
        muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    }
}

void PlayPanel::setVolume(int vol)
{
    if (!audioData) return;

    qreal linearVolume;
    if (vol == 0) {
        linearVolume = 0.0;
        audioData->setMuted(true);
        muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
    }
    else {
        linearVolume = QAudio::convertVolume(vol / qreal(100.0),
                                             QAudio::LogarithmicVolumeScale,
                                             QAudio::LinearVolumeScale);
        muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
        audioData->setMuted(false);
    }
    audioData->setVolume(linearVolume);
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

void PlayPanel::setXValue(/*QwtPlotMarker *c, */double xVal)
{
    if (!cursor->current) cursor->setCurrent(true);
    if (!ch) return;

    // здесь xVal - произвольное число, соответствующее какому-то положению на оси X
    moveCursor(xVal);

    if (audioData) {
        //audioData->reset();
        audioData->seek(/*количество байт с начала*/ xVal / ch->xStep() * sizeof(qint16));
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
    Q_UNUSED(state)
    //qDebug()<<state;
}

void PlayPanel::audioPosChanged()
{
    if (!ch || !audioData) return;
    const double xVal = 0.5 * audioData->pos() * ch->xStep();
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
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        return;
    }

    audio = new QAudioOutput(format, this);
    audio->setBufferSize(1 * qRound(1.0/ch->xStep())); // 10 sec

    audio->setNotifyInterval(50);

    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
    connect(audio, SIGNAL(notify()), this, SLOT(audioPosChanged()));
    setVolume(volumeSlider->value());
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
    pause();
    emit closeRequested();
    QWidget::closeEvent(event);
}

void PlayPanel::hideEvent(QHideEvent *event)
{
    cursor->setVisible(false);
    pause();
    QWidget::hideEvent(event);
}

void PlayPanel::moveCursor(const double xVal)
{
    cursor->moveTo(xVal);
    //if (cursor->yValues.isEmpty()) cursor->yValues << xVal;
    //else cursor->yValues[0] = xVal;
    cursor->updateLabel();
}

void PlayPanel::muteVolume(bool mute)
{
    if (audio) {
        audio->setVolume(mute?0:1);
    }
}

bool PlayPanel::muted() const
{
    if (audio) return qFuzzyIsNull(audio->volume());
    return true;
}
