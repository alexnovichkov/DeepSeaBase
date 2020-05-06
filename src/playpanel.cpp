#include "playpanel.h"
#include "plot/plot.h"
#include "qwt_plot_marker.h"
#include "fileformats/filedescriptor.h"

#include <QtWidgets>
#include <QAudioOutput>
#include <QMediaPlayer>
#include "dataiodevice.h"
#include "plot/curve.h"
#include "trackingpanel.h"
#include "wavexporter.h"

PlayPanel::PlayPanel(Plot *parent) : QWidget(parent), plot(parent)
{
    setWindowFlags(Qt::Tool /*| Qt::WindowTitleHint*/);
    setWindowTitle("Проигрыватель");

    player = new QMediaPlayer(this);
    player->setAudioRole(QAudio::MusicRole);
    player->setNotifyInterval(50);
    connect(player, &QMediaPlayer::durationChanged, this, &PlayPanel::durationChanged);
    connect(player, &QMediaPlayer::positionChanged, this, &PlayPanel::positionChanged);
    connect(player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &PlayPanel::metaDataChanged);
//    connect(playlist, &QMediaPlaylist::currentIndexChanged, this, &Player::playlistPositionChanged);
    connect(player, &QMediaPlayer::mediaStatusChanged, this, &PlayPanel::statusChanged);
    connect(player, &QMediaPlayer::stateChanged, this, &PlayPanel::stateChanged);

    connect(player, &QMediaPlayer::bufferStatusChanged, this, &PlayPanel::bufferingProgress);
    connect(player, &QMediaPlayer::audioAvailableChanged, this, &PlayPanel::audioAvailableChanged);
    connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &PlayPanel::displayErrorMessage);

    cursor = new TrackingCursor(Qt::green);
    cursor->showYValues = true;
    cursor->attach(plot);
    cursor->setAxes(QwtAxis::xBottom, QwtAxis::yLeft);
    cursor->setVisible(false);

    PlayerControls *controls = new PlayerControls(this);
    controls->setState(player->state());
    controls->setVolume(player->volume());
    controls->setMuted(controls->isMuted());

    connect(controls, &PlayerControls::play, player, &QMediaPlayer::play);
    connect(controls, &PlayerControls::pause, player, &QMediaPlayer::pause);
    connect(controls, &PlayerControls::stop, player, &QMediaPlayer::stop);
    connect(controls, &PlayerControls::changeVolume, player, &QMediaPlayer::setVolume);
    connect(controls, &PlayerControls::changeMuting, player, &QMediaPlayer::setMuted);

    connect(player, &QMediaPlayer::stateChanged, controls, &PlayerControls::setState);
    connect(player, &QMediaPlayer::volumeChanged, controls, &PlayerControls::setVolume);
    connect(player, &QMediaPlayer::mutedChanged, controls, &PlayerControls::setMuted);

    channelsBox = new QComboBox(this);
    connect(channelsBox,SIGNAL(currentIndexChanged(int)),SLOT(setSource(int)));
    channelsBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);



    QHBoxLayout *l = new QHBoxLayout(this);
    l->addWidget(channelsBox);
    l->addWidget(controls);
    setLayout(l);



    ch = 0;
    initialPos = 0.0;
    update();
}

PlayPanel::~PlayPanel()
{
    cursor->detach();
    delete cursor;
    QFile::remove(oldTempFile);
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
        //такой канал уже был в списке, ничего не делаем
    }
    else if (ch) {
        //такой канал был, но удалился


        ch = 0;
        if (!channels.keys().contains(oldIndex))
            setSource(oldIndex);
        else
            setSource(0);
    }
}

void PlayPanel::setSource(int n)
{
    if (!channels.keys().contains(n)) return;

    if (ch && channels.key(ch)==n) return;

    reset();
    player->setMedia(QMediaContent());

    ch = channels.value(n);
//    data = ch->wavData(0, ch->samplesCount(), 1.0);

    QFile::remove(oldTempFile);

    WavExporter expo(ch, this);
    QTemporaryFile temp;
    temp.setAutoRemove(false);
    temp.open();
    oldTempFile = temp.fileName();
    expo.setTempFile(oldTempFile);
    expo.start();

    player->setMedia(QMediaContent(QUrl::fromLocalFile(oldTempFile)));
    moveCursor(0);
}

void PlayPanel::mute()
{
//    if (!audioData->muted()) {
//        audioData->setMuted(true);
//        muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
//    }
//    else {
//        audioData->setMuted(false);
//        muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
//    }
}

void PlayPanel::setVolume(int vol)
{
//    if (!audioData) return;

//    qreal linearVolume;
//    if (vol == 0) {
//        linearVolume = 0.0;
//        audioData->setMuted(true);
//        muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
//    }
//    else {
//        linearVolume = QAudio::convertVolume(vol / qreal(100.0),
//                                             QAudio::LogarithmicVolumeScale,
//                                             QAudio::LinearVolumeScale);
//        muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
//        audioData->setMuted(false);
//    }
//    audioData->setVolume(linearVolume);
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

void PlayPanel::setXValue(double xVal)
{
    if (!cursor->current) cursor->setCurrent(true);
    if (!ch) return;

    // здесь xVal - произвольное число, соответствующее какому-то положению на оси X
    moveCursor(xVal);

    player->setPosition(qint64(xVal * 1000));
}

void PlayPanel::reset()
{
    player->stop();
}

void PlayPanel::durationChanged(qint64 duration)
{
    qDebug()<<"new duration"<<duration;
}

void PlayPanel::positionChanged(qint64 progress)
{
    //progress in milliseconds
    const double xVal = progress / 1000;
    moveCursor(xVal);
}

void PlayPanel::metaDataChanged()
{
    if (player->isMetaDataAvailable()) {
        qDebug()<<"metadata available";
//        player->metaData()
//        setTrackInfo(QString("%1 - %2")
//                .arg(m_player->metaData(QMediaMetaData::AlbumArtist).toString())
//                .arg(m_player->metaData(QMediaMetaData::Title).toString()));

//        if (m_coverLabel) {
//            QUrl url = m_player->metaData(QMediaMetaData::CoverArtUrlLarge).value<QUrl>();

//            m_coverLabel->setPixmap(!url.isEmpty()
//                    ? QPixmap(url.toString())
//                    : QPixmap());
//        }
    }
}

void PlayPanel::statusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::LoadingMedia ||
        status == QMediaPlayer::BufferingMedia ||
        status == QMediaPlayer::StalledMedia)
        setCursor(QCursor(Qt::BusyCursor));
    else
        unsetCursor();

    // handle status message
    switch (status) {
    case QMediaPlayer::UnknownMediaStatus:
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        //setStatusInfo(QString());
            qDebug()<<status;
        break;
    case QMediaPlayer::LoadingMedia:
        qDebug()<<"Loading...";
        break;
    case QMediaPlayer::StalledMedia:
        qDebug()<<"Media Stalled";
        break;
    case QMediaPlayer::EndOfMedia:
        QApplication::alert(this);
        break;
    case QMediaPlayer::InvalidMedia:
        qDebug() << player->errorString();
        break;
    }
}

void PlayPanel::stateChanged(QMediaPlayer::State state)
{
    qDebug()<<state;
}

void PlayPanel::bufferingProgress(int progress)
{
    qDebug()<<"Buffering" << progress;
}

void PlayPanel::audioAvailableChanged(bool available)
{
    qDebug()<<"audio available:"<<available;
}

void PlayPanel::displayErrorMessage()
{
    qDebug()<<player->errorString();
}

void PlayPanel::audioStateChanged(QAudio::State state)
{
    Q_UNUSED(state)
    //
}

void PlayPanel::audioPosChanged()
{
//    if (!ch || !audioData) return;

//    QAudio::Error e = audio->error();
//    if (e != QAudio::NoError) qDebug()<<"error"<<e;
//    const double xVal = 0.5 * audioData->pos() * ch->xStep();
//    moveCursor(xVal);
}

void PlayPanel::start()
{
//    if (!ch) return;

//    if (ch->type()!=Descriptor::TimeResponse) return;

//    if (audio) {
//        if (audio->state() == QAudio::ActiveState) // already playing
//            return;

//        if (audio->state() == QAudio::SuspendedState) {
//            audio->resume();
//            return;
//        }
//        audio->stop();
//        delete audio;
//        audio = 0;
//    }

////    if (audioData) {
////        delete audioData;
////        audioData = 0;
////    }

////    audioData = new DataIODevice(ch, this);
////    audioData->open(QIODevice::ReadOnly);

//    QAudioFormat format;
//    // Set up the format, eg.
//    format.setSampleRate(qRound(1.0/ch->xStep()));
//    format.setChannelCount(1);
//    format.setSampleSize(16);
//    format.setCodec("audio/pcm");
//    format.setByteOrder(QAudioFormat::LittleEndian);
//    format.setSampleType(QAudioFormat::UnSignedInt);

//    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
//    if (!info.isFormatSupported(format)) {
//        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
//        return;
//    }

//    audio = new QAudioOutput(format, this);
//    audio->setBufferSize(qRound(0.5/ch->xStep())); // 0.5 sec

//    audio->setNotifyInterval(50);

//    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
//    connect(audio, SIGNAL(notify()), this, SLOT(audioPosChanged()));
//    setVolume(volumeSlider->value());
//    audio->start(audioData);
}

void PlayPanel::stop()
{
    player->stop();
    moveCursor(0);
}

void PlayPanel::pause()
{
    player->pause();
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
//    if (audio) {
//        audio->setVolume(mute?0:1);
//    }
}

bool PlayPanel::muted() const
{
//    if (audio) return qFuzzyIsNull(audio->volume());
    return true;
}

/*******************************************************************/


PlayerControls::PlayerControls(QWidget *parent)
    : QWidget(parent)
{
    m_playButton = new QToolButton(this);
    m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    connect(m_playButton, &QAbstractButton::clicked, this, &PlayerControls::playClicked);

    m_stopButton = new QToolButton(this);
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_stopButton->setEnabled(false);

    connect(m_stopButton, &QAbstractButton::clicked, this, &PlayerControls::stop);

    m_muteButton = new QToolButton(this);
    m_muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));

    connect(m_muteButton, &QAbstractButton::clicked, this, &PlayerControls::muteClicked);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);

    connect(m_volumeSlider, &QSlider::valueChanged, this, &PlayerControls::onVolumeSliderValueChanged);

    QBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_stopButton);
    layout->addWidget(m_playButton);
    layout->addWidget(m_muteButton);
    layout->addWidget(m_volumeSlider);
    setLayout(layout);
}

QMediaPlayer::State PlayerControls::state() const
{
    return m_playerState;
}

void PlayerControls::setState(QMediaPlayer::State state)
{
    if (state != m_playerState) {
        m_playerState = state;

        switch (state) {
        case QMediaPlayer::StoppedState:
            m_stopButton->setEnabled(false);
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            break;
        case QMediaPlayer::PlayingState:
            m_stopButton->setEnabled(true);
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
            break;
        case QMediaPlayer::PausedState:
            m_stopButton->setEnabled(true);
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            break;
        }
    }
}

int PlayerControls::volume() const
{
    qreal linearVolume =  QAudio::convertVolume(m_volumeSlider->value() / qreal(100),
                                                QAudio::LogarithmicVolumeScale,
                                                QAudio::LinearVolumeScale);

    return qRound(linearVolume * 100);
}

void PlayerControls::setVolume(int volume)
{
    qreal logarithmicVolume = QAudio::convertVolume(volume / qreal(100),
                                                    QAudio::LinearVolumeScale,
                                                    QAudio::LogarithmicVolumeScale);

    m_volumeSlider->setValue(qRound(logarithmicVolume * 100));
}

bool PlayerControls::isMuted() const
{
    return m_playerMuted;
}

void PlayerControls::setMuted(bool muted)
{
    if (muted != m_playerMuted) {
        m_playerMuted = muted;

        m_muteButton->setIcon(style()->standardIcon(muted
                ? QStyle::SP_MediaVolumeMuted
                : QStyle::SP_MediaVolume));
    }
}

void PlayerControls::playClicked()
{
    switch (m_playerState) {
    case QMediaPlayer::StoppedState:
    case QMediaPlayer::PausedState:
        emit play();
        break;
    case QMediaPlayer::PlayingState:
        emit pause();
        break;
    }
}

void PlayerControls::muteClicked()
{
    emit changeMuting(!m_playerMuted);
}

void PlayerControls::onVolumeSliderValueChanged()
{
    emit changeVolume(volume());
}
