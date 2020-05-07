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
#include "logging.h"

PlayPanel::PlayPanel(Plot *parent) : QWidget(parent), plot(parent)
{
    setWindowFlags(Qt::Tool /*| Qt::WindowTitleHint*/);
    setWindowTitle("Проигрыватель");

    player = new QMediaPlayer(this);
    player->setAudioRole(QAudio::MusicRole);
    player->setNotifyInterval(50);
    connect(player, &QMediaPlayer::positionChanged, this, &PlayPanel::positionChanged);
    connect(player, &QMediaPlayer::mediaStatusChanged, this, &PlayPanel::statusChanged);
    connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &PlayPanel::displayErrorMessage);

    cursor = new TrackingCursor(Qt::green);
    cursor->showYValues = true;
    cursor->attach(plot);
    cursor->setAxes(QwtAxis::xBottom, QwtAxis::yLeft);
    cursor->setVisible(false);

    controls = new PlayerControls(this);
    controls->setState(player->state());
    controls->setVolume(player->volume());
    controls->setMuted(controls->isMuted());
    controls->enable(false);

    connect(controls, &PlayerControls::play, this, &PlayPanel::prepareDataToPlay);
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

    update();
}

PlayPanel::~PlayPanel()
{
    cursor->detach();
    delete cursor;
    //подчищаем старый временный файл, если он был
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
    }
}

void PlayPanel::update()
{
    // ищем канал, который сможем проиграть

    channelsBox->blockSignals(true);
    channelsBox->clear();

    int chIndex = -1;

    int count = 0;
    for (int i=0; i<plot->curves.size(); ++i) {
        Channel *c = plot->curves[i]->channel;
        if (c->type() == Descriptor::TimeResponse) {
            QPixmap pix(10,10);
            pix.fill(plot->curves[i]->pen().color());
            channelsBox->addItem(QIcon(pix), c->name(), QVariant((uint)c));
            if (c == ch) {
                //раньше этот канал уже играл, поэтому мы должны восстановить положение channelsBox
                chIndex = count;
            }
            count++;
        }
    }

    controls->enable(channelsBox->count() > 0);

    channelsBox->blockSignals(false);
    if (channelsBox->count() == 0) return;

    if (chIndex >= 0) {
        //такой канал есть, но, может быть, поменял номер
        channelsBox->setCurrentIndex(chIndex);
        setSource(chIndex);
    }

    else {
        //такой канал был, но удалился, или такого канала никогда не было
        ch = 0;
        channelsBox->setCurrentIndex(0);
        setSource(0);
    }
}

void PlayPanel::setSource(int n)
{
    if (n < 0) return;
    if (channelsBox->count() == 0) return;

    //индекс поменялся, а канал остался прежним
    if (ch && channelsBox->itemData(n).toUInt() == (uint)ch) return;

    ch = (Channel*)channelsBox->itemData(n).toUInt();

    reset();
    //реальная загрузка данных произойдет только при первом проигрывании
    player->setMedia(QMediaContent());

    moveCursor(0);
}

void PlayPanel::prepareDataToPlay()
{
    //загружаем данные
    if (player->media().isNull()) {
        if (!wavFiles.contains(ch)) {
            WavExporter expo(ch, this);
            QTemporaryFile temp;
            temp.setAutoRemove(false);
            temp.open();
            oldTempFile = temp.fileName();
            expo.setTempFile(oldTempFile);
            expo.start();
            wavFiles.insert(ch, oldTempFile);
        }

        player->setMedia(QMediaContent(QUrl::fromLocalFile(wavFiles.value(ch))));
    }
    player->play();
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

    player->setPosition(qint64(xVal * 1000.0));
}

void PlayPanel::reset()
{
    player->stop();
}

void PlayPanel::positionChanged(qint64 progress)
{
    //progress in milliseconds, convert to seconds
    const double xVal = double(progress) / 1000.0;
    moveCursor(xVal);
}

void PlayPanel::statusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
        case QMediaPlayer::UnknownMediaStatus:
        case QMediaPlayer::NoMedia:
        case QMediaPlayer::LoadedMedia:
        case QMediaPlayer::BufferedMedia:
            unsetCursor();
            break;
        case QMediaPlayer::BufferingMedia:
        case QMediaPlayer::LoadingMedia:
        case QMediaPlayer::StalledMedia:
            setCursor(QCursor(Qt::BusyCursor));
            break;
        case QMediaPlayer::EndOfMedia:
            unsetCursor();
            QApplication::alert(this);
            break;
        case QMediaPlayer::InvalidMedia:
            unsetCursor();
            displayErrorMessage();
            break;
    }
}

void PlayPanel::displayErrorMessage()
{
    qDebug()<<player->errorString();
}

void PlayPanel::closeEvent(QCloseEvent *event)
{
    cursor->setVisible(false);
    player->stop();
    emit closeRequested();
    QWidget::closeEvent(event);
}

void PlayPanel::hideEvent(QHideEvent *event)
{
    cursor->setVisible(false);
    player->pause();
    QWidget::hideEvent(event);
}

void PlayPanel::moveCursor(const double xVal)
{
    cursor->moveTo(xVal);
    cursor->updateLabel();
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

void PlayerControls::enable(bool enabled)
{
    m_playButton->setEnabled(enabled);
    m_stopButton->setEnabled(enabled);
    m_muteButton->setEnabled(enabled);
    m_volumeSlider->setEnabled(enabled);
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
