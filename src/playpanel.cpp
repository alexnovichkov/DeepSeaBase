#include "playpanel.h"
#include "plot/qcpplot.h"

#include "fileformats/filedescriptor.h"

#include <QtWidgets>
#include <QAudioOutput>
#include <QMediaPlayer>
#include "plot/curve.h"
#include "plot/qcpcursorplayer.h"
#include "plot/plotmodel.h"
#include "wavexporter.h"
#include "logging.h"


PlayPanel::PlayPanel(QCPPlot *parent) : QWidget(parent), plot(parent)
{DD;
    player = new QMediaPlayer(this);
    player->setAudioRole(QAudio::MusicRole);
    player->setNotifyInterval(50);
    connect(player, &QMediaPlayer::positionChanged, this, &PlayPanel::positionChanged);
    connect(player, &QMediaPlayer::mediaStatusChanged, this, &PlayPanel::statusChanged);
    connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &PlayPanel::displayErrorMessage);

    cursor = new QCPCursorPlayer(plot);
    cursor->setColor(Qt::green);
    cursor->setShowValues(false);
    connect(cursor, &Cursor::cursorPositionChanged, this, &PlayPanel::setValue);
//    cursor->attach();

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
    l->setMargin(0);
    l->addWidget(controls);
    l->addWidget(channelsBox);
    l->addStretch(1);
    setLayout(l);

    update();
}

PlayPanel::~PlayPanel()
{DD;
    delete cursor;
    //подчищаем старый временный файл, если он был
    QFile::remove(oldTempFile);
}

void PlayPanel::update()
{DD;
    // ищем канал, который сможем проиграть

    channelsBox->blockSignals(true);
    channelsBox->clear();

    int chIndex = -1;

    int count = 0;
    const auto curves = plot->model()->curves([](Curve *c){return c->channel->type() == Descriptor::TimeResponse;});
    for (auto c: curves) {
        QPixmap pix(10,10);
        pix.fill(c->pen().color());
        channelsBox->addItem(QIcon(pix), c->channel->name(), QVariant((qulonglong)(c->channel)));
        if (c->channel == ch) {
            //раньше этот канал уже играл, поэтому мы должны восстановить положение channelsBox
            chIndex = count;
        }
        count++;
    }
    if (channelsBox->count()==0) reset();

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
{DD;
    if (n < 0) return;
    if (channelsBox->count() == 0) return;

    //индекс поменялся, а канал остался прежним
    if (ch && channelsBox->itemData(n).toULongLong() == (qulonglong)ch) return;

    //запоминаем позицию для старого канала
    if (ch) {
        positions[ch] = cursor->currentPosition().x();
    }

    //новый канал
    ch = (Channel*)channelsBox->itemData(n).toULongLong();

    //reset();
    //реальная загрузка данных произойдет только при первом проигрывании
    if (wavFiles.contains(ch)) player->setMedia(QMediaContent(QUrl::fromLocalFile(wavFiles.value(ch))));
    else player->setMedia(QMediaContent());
    double x = positions.contains(ch)?positions[ch]:ch->data()->xMin();
    cursor->moveTo({x, 0}, false);
}

void PlayPanel::prepareDataToPlay()
{DD;
    //загружаем данные
    if (player->media().isNull()) {
        if (!wavFiles.contains(ch)) {
            WavExporter expo(ch, this);
            expo.setSimplified(true);
            QTemporaryFile temp(QDir::tempPath()+"/DeepSeaBase.XXXXXX.wav");
            temp.setAutoRemove(false);
            temp.open();
            oldTempFile = temp.fileName();
            expo.setTempFile(oldTempFile);
            expo.start();
            wavFiles.insert(ch, oldTempFile);
        }

        player->setMedia(QMediaContent(QUrl::fromLocalFile(wavFiles.value(ch))));
        double x = positions.contains(ch)?positions[ch]:ch->data()->xMin();
        player->setPosition(qint64(x * 1000.0));
    }
    player->play();
}

void PlayPanel::setValue()
{DD;
    if (!ch) return;
    player->setPosition(qint64((cursor->currentPosition().x()-ch->data()->xMin()) * 1000.0));
}

void PlayPanel::moveTo(const QPoint &pos)
{DD;
    const auto xVal = plot->screenToPlotCoordinates(Enums::AxisType::atBottom, pos.x());
    positions[ch] = xVal;
    cursor->moveTo({xVal, 0}, false);
}

void PlayPanel::reset()
{DD;
    player->stop();
}

void PlayPanel::positionChanged(qint64 progress)
{DD;
    //progress in milliseconds, convert to seconds
    const double xVal = double(progress) / 1000.0;
    cursor->moveTo({ch?xVal+ch->data()->xMin():xVal, 0}, true);
}

void PlayPanel::statusChanged(QMediaPlayer::MediaStatus status)
{DD;
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
{DD;
    LOG(ERROR)<<player->errorString();
}

void PlayPanel::closeEvent(QCloseEvent *event)
{DD;
    player->stop();
    emit closeRequested();
    QWidget::closeEvent(event);
}

void PlayPanel::hideEvent(QHideEvent *event)
{DD;
    player->pause();
    QWidget::hideEvent(event);
}

/*******************************************************************/


PlayerControls::PlayerControls(QWidget *parent)
    : QWidget(parent)
{DD;
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
    m_volumeSlider->setMaximumWidth(200);

    connect(m_volumeSlider, &QSlider::valueChanged, this, &PlayerControls::onVolumeSliderValueChanged);

    QBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_playButton);
    layout->addWidget(m_stopButton);
    layout->addWidget(m_muteButton);
    layout->addWidget(m_volumeSlider);
    setLayout(layout);
}

QMediaPlayer::State PlayerControls::state() const
{DD;
    return m_playerState;
}

void PlayerControls::setState(QMediaPlayer::State state)
{DD;
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
{DD;
    qreal linearVolume =  QAudio::convertVolume(m_volumeSlider->value() / qreal(100),
                                                QAudio::LogarithmicVolumeScale,
                                                QAudio::LinearVolumeScale);

    return qRound(linearVolume * 100);
}

void PlayerControls::setVolume(int volume)
{DD;
    qreal logarithmicVolume = QAudio::convertVolume(volume / qreal(100),
                                                    QAudio::LinearVolumeScale,
                                                    QAudio::LogarithmicVolumeScale);

    m_volumeSlider->setValue(qRound(logarithmicVolume * 100));
}

bool PlayerControls::isMuted() const
{DD;
    return m_playerMuted;
}

void PlayerControls::enable(bool enabled)
{DD;
    m_playButton->setEnabled(enabled);
    m_stopButton->setEnabled(enabled);
    m_muteButton->setEnabled(enabled);
    m_volumeSlider->setEnabled(enabled);
}

void PlayerControls::setMuted(bool muted)
{DD;
    if (muted != m_playerMuted) {
        m_playerMuted = muted;

        m_muteButton->setIcon(style()->standardIcon(muted
                ? QStyle::SP_MediaVolumeMuted
                : QStyle::SP_MediaVolume));
    }
}

void PlayerControls::playClicked()
{DD;
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
{DD;
    emit changeMuting(!m_playerMuted);
}

void PlayerControls::onVolumeSliderValueChanged()
{DD;
    emit changeVolume(volume());
}
