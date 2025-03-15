#include <clocale>
#include <cmath>
#include <stdexcept>

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolTip>
#include <QtGlobal>
#include <QtMath>

#include <QTextStream>

#include "mainwindow.h"
#include "playliststyle.h"
#include "qthelper.hpp"

#define MAX_VOLUME 130

static void wakeup(void* ctx)
{
    // This callback is invoked from any mpv thread (but possibly also
    // recursively from a thread that is calling the mpv API). Just notify
    // the Qt GUI thread to wake up (so that it can process events with
    // mpv_wait_event()), and return as quickly as possible.
    MainWindow* mainwindow = (MainWindow*)ctx;
    emit mainwindow->mpv_events();
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , settings(new QSettings("fastplayer", "fastplayer"))
    , muted(false)
    , paused(false)
    , time(0)
    , length(0)
    // since mpv doesn't specify the file extensions i just added those from:
    // https://en.wikipedia.org/wiki/Video_file_format#List_of_video_file_formats
    // and removed the ones i think are irrelevant (never saw/heard of them)
    , videoExt({ "webm", "mkv", "flv", "vob", "ogv", "gif", "avi", "mov", "qt", "wmv", "rm", "rmvb", "asf", "amv", "mp4", "m4v", "mp4v", "mpg", "mp2", "mpeg", "3gp", "mpts", "m2ts", "ts" })
    , supportedSubs({ "ass", "idx", "lrc", "mks", "pgs", "rt", "sbv", "scc", "smi", "srt", "ssa", "sub", "sup", "utf", "utf-8", "utf8", "vtt" })
    , boundKeys({ Qt::Key_Right, Qt::Key_Left, Qt::Key_Up, Qt::Key_Down, Qt::Key_Escape, Qt::Key_Return, Qt::Key_Enter })
    , cropH(0)
    , cropV(0)
    , eofReached(false)
{
    setWindowTitle("fastplayer");
    setWindowIcon(QIcon(":/fastplayer"));
    setMinimumSize(640, 480);
    setMouseTracking(true);
    setAcceptDrops(true);
    resize(800, 600);
    loadConfig();

    int fontWidth = fontMetrics().averageCharWidth();
    playIcon = QIcon::fromTheme("media-playback-start");
    pauseIcon = QIcon::fromTheme("media-playback-pause");
    playButton = new QPushButton;
    playButton->setFlat(true);
    playButton->setToolTip("Play/Pause");
    playButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    playButton->setIcon(pauseIcon);

    speedSpin = new QSpinBox;
    // speedSpin->setFocusProxy(this);
    speedSpin->setToolTip("Playback Speed");
    // speedSpin->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    speedSpin->setRange(10, 500);
    speedSpin->setSingleStep(10);
    speedSpin->setValue(playbackSpeed);
    speedSpin->setSuffix("%");

    zoomSpin = new QSpinBox;
    zoomSpin->setToolTip("Video Zoom (Not Saved)");
    // zoomSpin->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    zoomSpin->setRange(10, 900);
    zoomSpin->setSuffix("%");
    zoomSpin->setValue(100);
    zoomSpin->setSingleStep(2);
    rotationSpin = new QSpinBox;
    rotationSpin->setToolTip("Video Rotation (Not Saved)");
    // rotationSpin->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    rotationSpin->setRange(-90, 360);
    rotationSpin->setSuffix("°");
    rotationSpin->setValue(0);
    rotationSpin->setSingleStep(15);
    panXSpin = new QSpinBox;
    panXSpin->setToolTip("Pan Horizontally (Not Saved)");
    // panXSpin->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    panXSpin->setRange(-99, 99);
    panXSpin->setSingleStep(1);
    panXSpin->setValue(0);
    panXSpin->setPrefix("x");
    panXSpin->setSuffix("%");
    panYSpin = new QSpinBox;
    panYSpin->setToolTip("Pan Vertically (Not Saved)");
    // panYSpin->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    panYSpin->setRange(-99, 99);
    panYSpin->setSingleStep(1);
    panYSpin->setValue(0);
    panYSpin->setPrefix("y");
    panYSpin->setSuffix("%");
    cropHSpin = new QSpinBox;
    cropHSpin->setToolTip("Crop Horizontally (Not Saved)");
    cropHSpin->setRange(0, 990);
    cropHSpin->setSingleStep(10);
    cropHSpin->setValue(0);
    cropHSpin->setPrefix("H");
    cropVSpin = new QSpinBox;
    cropVSpin->setToolTip("Crop Vertically (Not Saved)");
    cropVSpin->setRange(0, 990);
    cropVSpin->setSingleStep(10);
    cropVSpin->setValue(0);
    cropVSpin->setPrefix("V");

    brightnessSpin = new QSpinBox;
    brightnessSpin->setPrefix("b");
    brightnessSpin->setToolTip("Brightness (Not Saved)");
    // brightnessSpin->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    brightnessSpin->setRange(-99, 100);
    brightnessSpin->setValue(0);
    contrastSpin = new QSpinBox;
    contrastSpin->setPrefix("c");
    contrastSpin->setToolTip("Contrast (Not Saved)");
    // contrastSpin->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    contrastSpin->setRange(-99, 100);
    contrastSpin->setValue(0);
    saturationSpin = new QSpinBox;
    saturationSpin->setPrefix("s");
    saturationSpin->setToolTip("Saturation (Not Saved)");
    // saturationSpin->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    saturationSpin->setRange(-99, 100);
    saturationSpin->setValue(0);
    gammaSpin = new QSpinBox;
    gammaSpin->setPrefix("g");
    gammaSpin->setToolTip("Gamma (Not Saved)");
    // gammaSpin->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    gammaSpin->setRange(-99, 100);
    gammaSpin->setValue(0);
    hueSpin = new QSpinBox;
    hueSpin->setPrefix("h");
    hueSpin->setToolTip("Hue (Not Saved)");
    // hueSpin->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    hueSpin->setRange(-99, 100);
    hueSpin->setValue(0);

    progressBar = new QProgressBar;
    progressBar->setMouseTracking(true);

    volumeButton = new QPushButton;
    volumeButton->setToolTip("Mute/Unmute (Not Saved)");
    volumeButton->setFlat(true);
    volumeButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    volumeMutedIcon = QIcon::fromTheme("audio-volume-muted");
    volumeLowIcon = QIcon::fromTheme("audio-volume-low");
    volumeMediumIcon = QIcon::fromTheme("audio-volume-medium");
    volumeHighIcon = QIcon::fromTheme("audio-volume-high");

    volumeBar = new QProgressBar;
    volumeBar->setMouseTracking(true);
    volumeBar->setRange(0, MAX_VOLUME);
    volumeBar->setFormat("%v%");
    volumeBar->setValue(currentVolume);
    volumeBar->setFixedWidth(fontWidth * 10);

    audioButton = new QPushButton;
    audioButton->setFlat(true);
    audioButton->setToolTip("Audio Tracks");
    audioButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    audioButton->setIcon(QIcon::fromTheme("media-album-track"));

    subButton = new QPushButton;
    subButton->setFlat(true);
    subButton->setToolTip("Subtitle Tracks");
    subButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    subButton->setIcon(QIcon::fromTheme("add-subtitle"));

    configDialogButton = new QPushButton;
    configDialogButton->setFlat(true);
    configDialogButton->setToolTip("Settings");
    configDialogButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    configDialogButton->setIcon(QIcon::fromTheme("settings"));

    playlistButton = new QPushButton;
    playlistButton->setFlat(true);
    playlistButton->setToolTip("Show/Hide Playlist");
    playlistButton->setFocusPolicy(Qt::NoFocus);
    playlistButton->setIcon(QIcon::fromTheme("media-playlist-normal"));
    playlistButton->setCheckable(true);
    playlistButton->setChecked(playlistVisible);

    mpvWidget = new QWidget(this);
    controlBar = new QWidget;
    // controlBar->setMaximumHeight(fontMetrics().height() * 1.5);
    controlLayout = new QHBoxLayout(controlBar);
    controlBar->setLayout(controlLayout);
    controlLayout->setContentsMargins(QMargins(3, 1, 3, 1));
    auto centralWidget = new QWidget;
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(1);
    setCentralWidget(centralWidget);
    mainLayout->setContentsMargins(QMargins(0, 0, 0, 0));

    mainLayout->addWidget(mpvWidget, 1);
    mainLayout->addWidget(controlBar);

    controlLayout->setSpacing(1);
    controlLayout->addWidget(playButton);
    controlLayout->addWidget(speedSpin);
    controlLayout->addSpacing(3);
    controlLayout->addWidget(zoomSpin);
    controlLayout->addWidget(rotationSpin);
    controlLayout->addWidget(panXSpin);
    controlLayout->addWidget(panYSpin);
    controlLayout->addWidget(cropHSpin);
    controlLayout->addWidget(cropVSpin);
    controlLayout->addSpacing(3);
    controlLayout->addWidget(brightnessSpin);
    controlLayout->addWidget(contrastSpin);
    controlLayout->addWidget(saturationSpin);
    controlLayout->addWidget(gammaSpin);
    controlLayout->addWidget(hueSpin);
    controlLayout->addSpacing(3);
    controlLayout->addWidget(progressBar);
    controlLayout->addSpacing(3);
    controlLayout->addWidget(volumeButton);
    controlLayout->addWidget(volumeBar);
    controlLayout->addWidget(audioButton);
    controlLayout->addSpacing(3);
    controlLayout->addWidget(subButton);
    controlLayout->addWidget(configDialogButton);
    controlLayout->addWidget(playlistButton);

    playButton->setVisible(showPlayPause);
    speedSpin->setVisible(showSpeed);
    zoomSpin->setVisible(showZoom);
    rotationSpin->setVisible(showRotation);
    panXSpin->setVisible(showPanX);
    panYSpin->setVisible(showPanY);
    cropHSpin->setVisible(showCropH);
    cropVSpin->setVisible(showCropV);
    brightnessSpin->setVisible(showBrightness);
    contrastSpin->setVisible(showContrast);
    saturationSpin->setVisible(showSaturation);
    gammaSpin->setVisible(showGamma);
    hueSpin->setVisible(showHue);
    progressBar->setVisible(showProgress);
    volumeButton->setVisible(showMute);
    volumeBar->setVisible(showVolume);
    audioButton->setVisible(showAudio);
    subButton->setVisible(showSub);
    configDialogButton->setVisible(showSettings);
    playlistButton->setVisible(showPlaylistButton);

    // Playlist
    playlistDock = new QDockWidget("Playlist", this);
    playlistDock->setObjectName("Playlist");
    playlistView = new ListView;
    playlistView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    playlistView->setTextElideMode(Qt::ElideRight);
    playlistView->setSelectionMode(QListView::SingleSelection);
    playlistView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    playlistStyle = new PlaylistStyle;
    playlistView->setItemDelegate(playlistStyle);
    playlistModel = new QStandardItemModel;
    playlistView->setModel(playlistModel);

    playlistDock->setWidget(playlistView);
    playlistDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::BottomDockWidgetArea, playlistDock);
    playlistDock->setVisible(playlistVisible);

    //
    setMouseTracking(true);

    configureMpv();

    registerDBus(SERVICE_NAME);

    // if (arg != QString()) {
    //     auto url = QFileInfo::exists(arg) ? QUrl::fromLocalFile(arg) : QUrl(arg);
    //     loadFiles({ url });
    // }
    mpvWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(mpvWidget, &QWidget::customContextMenuRequested, this, &MainWindow::showCustomMenu);
    connect(playButton, &QPushButton::clicked, this, &MainWindow::playPauseClicked);
    connect(speedSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateSpeed);
    connect(zoomSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
        double val = (double)value / 100;
        mpv::qt::set_option_variant(mpv, "video-zoom", log2(val));
    });
    connect(rotationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
        int newValue = (360 + value) % 360;
        auto spin = qobject_cast<QSpinBox*>(sender());
        if (nullptr != spin) {
            QSignalBlocker blocker(spin);
            spin->setValue(newValue);
            mpv::qt::set_option_variant(mpv, "video-rotate", newValue);
        }
    });
    connect(panXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int perc) {
        double value = (double)perc / 100;
        mpv::qt::set_option_variant(mpv, "video-pan-x", value);
    });
    connect(panYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int perc) {
        double value = (double)perc / 100;
        mpv::qt::set_option_variant(mpv, "video-pan-y", value);
    });
    connect(cropHSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val) {
        if (val == 0 && cropVSpin->value() == 0) {
            mpv::qt::set_option_variant(mpv, "video-crop", QString());
        }
        QMap<QString, QVariant> params = mpv::qt::get_property(mpv, "video-params").toMap();
        int w = params["dw"].toInt();
        int h = params["dh"].toInt();
        int x = qBound(0, val, w / 2);
        int y = qBound(0, cropVSpin->value(), h / 2);
        w = w - 2 * x;
        h = h - 2 * y;
        QString value = QString("%1x%2+%3+%4").arg(w).arg(h).arg(x).arg(y);
        mpv::qt::set_option_variant(mpv, "video-crop", value);
    });
    connect(cropVSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val) {
        if (val == 0 && cropHSpin->value() == 0) {
            mpv::qt::set_option_variant(mpv, "video-crop", QString());
        }
        QMap<QString, QVariant> params = mpv::qt::get_property(mpv, "video-params").toMap();
        int w = params["dw"].toInt();
        int h = params["dh"].toInt();
        int x = qBound(0, cropHSpin->value(), w / 2);
        int y = qBound(0, val, h / 2);
        w = w - 2 * x;
        h = h - 2 * y;
        QString value = QString("%1x%2+%3+%4").arg(w).arg(h).arg(x).arg(y);
        mpv::qt::set_option_variant(mpv, "video-crop", value);
    });
    connect(brightnessSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
        mpv::qt::set_option_variant(mpv, "brightness", value);
    });
    connect(contrastSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
        mpv::qt::set_option_variant(mpv, "contrast", value);
    });
    connect(saturationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
        mpv::qt::set_option_variant(mpv, "saturation", value);
    });
    connect(gammaSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
        mpv::qt::set_option_variant(mpv, "gamma", value);
    });
    connect(hueSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
        mpv::qt::set_option_variant(mpv, "hue", value);
    });
    connect(volumeButton, &QPushButton::clicked, this, [=] {
        mpv::qt::set_property_variant(mpv, "mute", !muted);
    });
    connect(configDialogButton, &QPushButton::clicked, this, &MainWindow::showConfigDialog);
    connect(playlistButton, &QPushButton::toggled, this, [=](bool checked) {
        playlistDock->setVisible(checked);
        playlistVisible = checked;
        settings->setValue("playlistVisible", checked);
    });
    connect(playlistView, &QListView::clicked, this, [=](const QModelIndex& index) {
        // qDebug() << "clicked" << index.row();
        if (index.isValid()) {
            // playlistView->setCurrentIndex(index);
            playlistView->selectionModel()->select(index, QItemSelectionModel::Select);
        }
    });
    connect(playlistView, &QListView::doubleClicked, this, [=](const QModelIndex& index) {
        if (index.isValid()) {
            mpv::qt::set_property_variant(mpv, "playlist-pos", index.row());
            mpv::qt::set_property_variant(mpv, "pause", false);
        }
    });

    progressBar->installEventFilter(this);
    volumeBar->installEventFilter(this);
    mpvWidget->installEventFilter(this);
    this->installEventFilter(this);
    speedSpin->installEventFilter(this);
    zoomSpin->installEventFilter(this);
    rotationSpin->installEventFilter(this);
    panXSpin->installEventFilter(this);
    panYSpin->installEventFilter(this);

    brightnessSpin->installEventFilter(this);
    contrastSpin->installEventFilter(this);
    saturationSpin->installEventFilter(this);
    gammaSpin->installEventFilter(this);
    hueSpin->installEventFilter(this);
}

// #include <QDBusReply>
void MainWindow::registerDBus(const QString& service)
{
    if (service != SERVICE_NAME) {
        qInfo() << "Service " << service << " unregistered, ignored";
        return;
    }
    if (sender()) {
        qInfo() << "Existing service was unregistered, trying again";
        disconnect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &MainWindow::registerDBus);
        watcher->deleteLater();
    }

    if (QDBusConnection::sessionBus().registerService(SERVICE_NAME)) {
        // Register the object
        qInfo() << "Service" << service << "registered succesfully.";
        if (QDBusConnection::sessionBus().registerObject("/", this, QDBusConnection::ExportScriptableSlots)) {
            qInfo() << "Object registered succesfully.";
        }
    }
    else {
        qInfo() << "Service" << service << "exists, will wait for it to unregister.";
        watcher = new QDBusServiceWatcher(SERVICE_NAME, QDBusConnection::sessionBus());
        connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &MainWindow::registerDBus);
    }
}

void MainWindow::configureMpv()
{
    mpv = mpv_create();
    if (!mpv) {
        throw std::runtime_error("can't create mpv instance");
    }

    // Create a video child window. Force Qt to create a native window, and
    // pass the window ID to the mpv wid option. Works on: X11, win32, Cocoa

    // mpvWidget->setAttribute(Qt::WA_DontCreateNativeAncestors);
    // mpvWidget->setAttribute(Qt::WA_NativeWindow);
    auto raw_wid = mpvWidget->winId();
#ifdef _WIN32
    // Truncate to 32-bit, as all Windows handles are. This also ensures
    // it doesn't go negative.
    int64_t wid = static_cast<uint32_t>(raw_wid);
#else
    int64_t wid = raw_wid;
#endif
    mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &wid);
    mpv_set_option_string(mpv, "input-default-bindings", "no");
    mpv_set_option_string(mpv, "idle", "yes");
    mpv_set_option_string(mpv, "keep-open", "yes");
    mpv_set_option_string(mpv, "input-cursor-passthrough", "yes");

    // Let us receive property change events with MPV_EVENT_PROPERTY_CHANGE if
    // this property changes.
    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "core-idle", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "eof-reached", MPV_FORMAT_FLAG);

    mpv_observe_property(mpv, 0, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(mpv, 0, "chapter-list", MPV_FORMAT_NODE);
    mpv_observe_property(mpv, 0, "media-title", MPV_FORMAT_STRING);
    mpv_observe_property(mpv, 0, "playlist", MPV_FORMAT_NODE);

    // From this point on, the wakeup function will be called. The callback
    // can come from any thread, so we use the QueuedConnection mechanism to
    // relay the wakeup in a thread-safe way.
    connect(this, &MainWindow::mpv_events, this, &MainWindow::onMpvEvents,
        Qt::QueuedConnection);
    mpv_set_wakeup_callback(mpv, wakeup, this);

    if (mpv_initialize(mpv) < 0) {
        throw std::runtime_error("mpv failed to initialize");
    }
    mpv::qt::set_property_variant(mpv, "volume", currentVolume);
    mpv::qt::set_option_variant(mpv, "sub-font", subFont.family());
    mpv::qt::set_option_variant(mpv, "sub-font-size", subFontSize);
    mpv::qt::set_option_variant(mpv, "sub-border-color", subBorderColor.name(QColor::HexArgb));
    mpv::qt::set_option_variant(mpv, "sub-border-size", subBorderSize);
    mpv::qt::set_option_variant(mpv, "sub-color", subColor.name(QColor::HexArgb));
    double speed = (double)playbackSpeed / 100;
    mpv::qt::set_property_variant(mpv, "speed", speed);
}

void MainWindow::loadConfig()
{
    seekStep = settings->value("seekStep", 10).toInt();
    volumeStep = settings->value("volumeStep", 5).toInt();
    currentVolume = settings->value("volume", 70).toInt();
    currentVolume = qBound(0, currentVolume, MAX_VOLUME);
    subFont = settings->value("subFont", QFont("sans-serif")).value<QFont>();
    subFontSize = settings->value("subFontSize", 55).toInt();
    subBorderColor = settings->value("subBorderColor", QColor(Qt::black)).value<QColor>();
    subBorderSize = settings->value("subBorderSize", 3).toInt();
    subColor = settings->value("subColor", QColor(Qt::yellow)).value<QColor>();
    playbackSpeed = settings->value("playbackSpeed", 100).toInt();

    showPlayPause = settings->value("showPlayPause", true).toBool();
    showSpeed = settings->value("showSpeed", true).toBool();
    showZoom = settings->value("showZoom", true).toBool();
    showRotation = settings->value("showRotation", true).toBool();
    showPanX = settings->value("showPanX", false).toBool();
    showPanY = settings->value("showPanY", false).toBool();
    showCropH = settings->value("showCropH", true).toBool();
    showCropV = settings->value("showCropV", true).toBool();

    showBrightness = settings->value("showBrightness", true).toBool();
    showContrast = settings->value("showContrast", true).toBool();
    showSaturation = settings->value("showSaturation", false).toBool();
    showGamma = settings->value("showGamma", false).toBool();
    showHue = settings->value("showHue", false).toBool();

    showProgress = settings->value("showProgress", true).toBool();
    showMute = settings->value("showMute", true).toBool();
    showVolume = settings->value("showVolume", true).toBool();
    showAudio = settings->value("showAudio", true).toBool();
    showSub = settings->value("showSub", true).toBool();
    showSettings = settings->value("showSettings", true).toBool();
    showPlaylistButton = settings->value("showPlaylistButton", true).toBool();
    playlistVisible = settings->value("playlistVisible", true).toBool();

    restoreState(settings->value("windowState").toByteArray());
    restoreGeometry(settings->value("geometry").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    settings->setValue("geometry", saveGeometry());
    settings->setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == progressBar) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            int x = mouseEvent->position().x();
            if (event->type() == QEvent::MouseButtonPress && mouseEvent->buttons() & Qt::LeftButton) {
                // seek
                progressClicked(x);
            }
            else if (event->type() == QEvent::MouseMove && mouseEvent->buttons() & Qt::LeftButton) {
                // dragging
                progressClicked(x);
            }
            else if (event->type() == QEvent::MouseMove) {
                // tooltip for seek
                showProgressTooltip(mouseEvent->globalPosition().toPoint(), x);
            }
            return true;
        }
        if (event->type() == QEvent::Wheel) {
            auto wheelEvent = static_cast<QWheelEvent*>(event);
            int y = wheelEvent->angleDelta().y();
            seek(y > 0);
            return true;
        }
        return false;
    }
    else if (obj == volumeBar) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            int x = mouseEvent->position().x();
            if (event->type() == QEvent::MouseButtonPress && mouseEvent->buttons() & Qt::LeftButton) {
                // set volume
                volumeBarClicked(x);
            }
            else if (event->type() == QEvent::MouseMove && mouseEvent->buttons() & Qt::LeftButton) {
                // dragging
                volumeBarClicked(x);
            }
            else if (event->type() == QEvent::MouseMove) {
                // tooltip for volume
                showVolumeTooltip(mouseEvent->globalPosition().toPoint(), x);
            }
            return true;
        }
        if (event->type() == QEvent::Wheel) {
            auto wheelEvent = static_cast<QWheelEvent*>(event);
            int y = wheelEvent->angleDelta().y();
            stepVolume(y > 0);
            return true;
        }
        return false;
    }
    else if (obj == mpvWidget) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                playPauseClicked();
                return true;
            }
        }
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                toggleFullscreen();
                playPauseClicked();
                return true;
            }
        }
        if (event->type() == QEvent::Wheel) {
            auto wheelEvent = static_cast<QWheelEvent*>(event);
            int y = wheelEvent->angleDelta().y();
            seek(y > 0);
            return true;
        }
        return QMainWindow::eventFilter(obj, event);
    }
    else if (obj == this || obj == mpvWidget) {
        if (event->type() == QEvent::KeyPress) {
            // qDebug() << "keypress";
            auto keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->modifiers().testFlag(Qt::NoModifier)) {
                if (keyEvent->key() == Qt::Key_Space) {
                    // qDebug() << "space";
                    playPauseClicked();
                }
                else if (keyEvent->key() == Qt::Key_L || keyEvent->key() == Qt::Key_Right) {
                    seek(true);
                }
                else if (keyEvent->key() == Qt::Key_K || keyEvent->key() == Qt::Key_Left) {
                    seek(false);
                }
                else if (keyEvent->key() == Qt::Key_H || keyEvent->key() == Qt::Key_Up) {
                    stepVolume(true);
                }
                else if (keyEvent->key() == Qt::Key_J || keyEvent->key() == Qt::Key_Down) {
                    stepVolume(false);
                }
                else if (keyEvent->key() == Qt::Key_F11) {
                    toggleFullscreen();
                }
                else if (keyEvent->key() == Qt::Key_Escape && isFullScreen()) {
                    toggleFullscreen();
                }
                return true;
            }
        }
    }
    else if (obj == playlistView) {
        if (event->type() == QEvent::MouseButtonPress) {
            // qDebug() << "playlist mouse";
        }
    }
    else if (nullptr != qobject_cast<QAbstractSpinBox*>(obj) && (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)) {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        auto k = keyEvent->key();
        if ((k >= Qt::Key_0 && k <= Qt::Key_9) || (k >= Qt::Key_Left && k <= Qt::Key_Down)) {
            // make spinbox handle only those keys
            return QMainWindow::eventFilter(obj, event);
        }
        // let MainWindow handle the event maybe it is a shortcut
        return eventFilter(this, event);
    }
    // pass the event on to the parent class
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e)
{
    auto mime = e->mimeData();

    if (mime->hasUrls()) {
        e->accept();
        return;
    }
    e->ignore();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent* e)
{
    Q_UNUSED(e);
}

void MainWindow::dragMoveEvent(QDragMoveEvent* e)
{
    Q_UNUSED(e);
}

void MainWindow::dropEvent(QDropEvent* e)
{
    auto mime = e->mimeData();

    if (mime->hasUrls()) {
        e->accept();
        loadFiles(mime->urls());
        return;
    }
    e->ignore();
}

void MainWindow::onFileLoaded()
{
}

void MainWindow::updateTracks(QVariantList list)
{

    if (nullptr != subButton->menu()) {
        subButton->menu()->deleteLater();
    }
    if (nullptr != audioButton->menu()) {
        audioButton->menu()->deleteLater();
    }
    QMenu* audioMenu = new QMenu;
    QMenu* subMenu = new QMenu;
    audioButton->setMenu(audioMenu);
    subButton->setMenu(subMenu);
    QAction* a;

    for (int i = 0; i < list.count(); ++i) {
        auto map = list.at(i).toMap();
        bool selected = map.value("selected").toBool();
        int id = map.value("id").toInt();
        QString lang = map.value("lang").toString();
        QString title = map.value("title").toString();
        QString text = (lang.isEmpty() ? "" : "[" + lang + "]") + (title.isEmpty() ? "" : " " + title);
        if (text.isEmpty()) {
            text = QString("id: %1").arg(id);
        }
        if (selected) {
            text = "* " + text;
        }

        if (map.value("type").toString() == "audio") {
            a = audioMenu->addAction(text);
            connect(a, &QAction::triggered, this, [=] {
                mpv::qt::set_option_variant(mpv, "aid", id);
            });
        }
        else if (map.value("type").toString() == "sub") {
            a = subMenu->addAction(text);
            connect(a, &QAction::triggered, this, [=] {
                mpv::qt::set_option_variant(mpv, "sid", id);
            });
        }
    }

    if (audioMenu->actions().count() == 0) {
        a = audioMenu->addAction("No Audio Tracks");
        a->setEnabled(false);
    }
    if (subMenu->actions().count() == 0) {
        a = subMenu->addAction("No Subtitles");
        a->setEnabled(false);
    }
}

void MainWindow::onPropertyChanged(mpv_event_property* prop)
{
    if (strcmp(prop->name, "time-pos") == 0) {
        if (prop->format == MPV_FORMAT_INT64) {
            time = *(int*)prop->data;
            updateProgress();
        }
    }
    else if (strcmp(prop->name, "track-list") == 0) {
        mpv_node* node = (mpv_node*)prop->data;
        QVariantList list = mpv::qt::node_to_variant(node).toList();
        updateTracks(list);
    }
    else if (strcmp(prop->name, "playlist") == 0) {
        mpv_node* node = (mpv_node*)prop->data;
        QVariantList list = mpv::qt::node_to_variant(node).toList();
        updatePlaylist(list);
    }
    else if (strcmp(prop->name, "pause") == 0) {
        // qDebug() << "pause event" << *(bool*)prop->data;
    }
    else if (strcmp(prop->name, "core-idle") == 0) {
        if (prop->format == MPV_FORMAT_FLAG) {
            paused = *(bool*)prop->data;
            playButton->setIcon(paused ? playIcon : pauseIcon);
        }
    }
    else if (strcmp(prop->name, "volume") == 0) {
        if (prop->format == MPV_FORMAT_INT64) {
            currentVolume = *(int*)prop->data;
            updateVolume();
        }
    }
    else if (strcmp(prop->name, "mute") == 0) {
        if (prop->format == MPV_FORMAT_FLAG) {
            muted = *(bool*)prop->data;
            updateVolume();
        }
    }
    else if (strcmp(prop->name, "eof-reached") == 0) {
        if (prop->format == MPV_FORMAT_FLAG) {
            bool eof = *(bool*)prop->data;
            if (eof) {
                eofReached = true;
            }
        }
    }

    else if (strcmp(prop->name, "media-title") == 0) {
        if (prop->format == MPV_FORMAT_NONE) {
            setWindowTitle("fastplayer");
        }
        else if (prop->format == MPV_FORMAT_STRING) {
            char* title = *(char**)prop->data;

            setWindowTitle(QString::fromUtf8(title) + " - fastplayer");
        }
    }
    else if (strcmp(prop->name, "filename") == 0) {
        QTextStream stream(stdout);
        stream << "filename" << prop->format << prop->data << Qt::endl;
    }

    else if (strcmp(prop->name, "duration") == 0) {
        if (prop->format == MPV_FORMAT_INT64) {
            length = *(int*)prop->data;
            updateProgress();
        }
    }
}

void MainWindow::updateProgress()
{
    progressBar->setMaximum(length);
    progressBar->setValue(time);

    bool withHour = length >= 60 * 60;

    QString timeString = timeStringFromInt(time, withHour);
    QString durationString = timeStringFromInt(length, withHour);
    progressBar->setFormat(QString("%1/%2").arg(timeString, durationString));
}

void MainWindow::progressClicked(int posX)
{
    int clickedTime = (((double)posX / progressBar->width()) * length);
    mpv::qt::set_property_variant(mpv, "time-pos", clickedTime);
}

void MainWindow::showProgressTooltip(QPoint globalPos, int posX)
{
    int tooltipTime = (((double)posX / progressBar->width()) * length);
    QToolTip::showText(globalPos, QString("Jump to: %1").arg(timeStringFromInt(tooltipTime, length > 60 * 60)));
}

void MainWindow::updateVolume()
{
    settings->setValue("volume", currentVolume);
    if (currentVolume == 0 || muted) {
        volumeButton->setIcon(volumeMutedIcon);
    }
    else if (currentVolume < 41) {
        volumeButton->setIcon(volumeLowIcon);
    }
    else if (currentVolume < 81) {
        volumeButton->setIcon(volumeMediumIcon);
    }
    else {
        volumeButton->setIcon(volumeHighIcon);
    }
    volumeBar->setValue(currentVolume);
}

void MainWindow::volumeBarClicked(int posX)
{
    int clickedVolume = (((double)posX / volumeBar->width()) * MAX_VOLUME);
    mpv::qt::set_property_variant(mpv, "volume", clickedVolume);
    if (muted) {
        mpv::qt::set_property_variant(mpv, "mute", false);
    }
}

void MainWindow::showVolumeTooltip(QPoint globalPos, int posX)
{
    int volumeTooltip = (((double)posX / volumeBar->width()) * MAX_VOLUME);
    QToolTip::showText(globalPos, QString("Set Volume: %1%").arg(volumeTooltip));
}

void MainWindow::stepVolume(bool increase)
{
    currentVolume += (increase ? volumeStep : -volumeStep);
    currentVolume = qBound(0, currentVolume, MAX_VOLUME);
    mpv::qt::set_property_variant(mpv, "volume", currentVolume);
    if (muted) {
        mpv::qt::set_property_variant(mpv, "mute", false);
    }
}

void MainWindow::updatePlaylist(QVariantList list)
{
    auto modelIndex = playlistView->currentIndex();
    int row = modelIndex.isValid() ? modelIndex.row() : -1;
    if (row >= list.count()) {
        row = list.count() - 1;
    }
    int iconHeight = fontMetrics().height() * 1;
    QIcon upIcon = QIcon::fromTheme("go-up");
    QIcon downIcon = QIcon::fromTheme("go-down");
    QIcon delIcon = QIcon::fromTheme("list-remove");

    playlistModel->clear();

    for (int i = 0; i < list.count(); ++i) {
        auto media = list.at(i).toMap();
        QString filename = media.value("filename").toString();
        QFileInfo info(filename);
        QString title = info.exists() ? info.completeBaseName() : filename;
        bool current = media.value("current").toBool();
        auto item = new QStandardItem(QString());
        playlistModel->appendRow(item);
        auto index = playlistModel->indexFromItem(item);
        auto widget = new QWidget;
        widget->setContentsMargins(0, 0, 0, 0);
        auto box = new QHBoxLayout(widget);
        box->setContentsMargins(0, 0, 0, 0);
        auto iconLabel = new QLabel;
        if (current) {
            iconLabel->setPixmap(playIcon.pixmap(iconHeight));
        }
        auto titleLabel = new QLabel(title);
        auto upButton = new QPushButton(upIcon, QString());
        auto downButton = new QPushButton(downIcon, QString());
        auto delButton = new QPushButton(delIcon, QString());
        box->addWidget(iconLabel);
        box->addWidget(titleLabel, 1);
        box->addSpacing(5);
        box->addWidget(upButton);
        box->addWidget(downButton);
        box->addSpacing(5);
        box->addWidget(delButton);
        box->addSpacing(5);
        playlistView->setIndexWidget(index, widget);
        if (i == 0) {
            upButton->setEnabled(false);
        }
        else {
            connect(upButton, &QPushButton::clicked, this, [=] {
                QVariantList args;
                args << QString("playlist-move") << i << i - 1;
                mpv::qt::command(mpv, args);
            });
        }
        if (i == list.count() - 1) {
            downButton->setEnabled(false);
        }
        else {
            connect(downButton, &QPushButton::clicked, this, [=] {
                QVariantList args;
                args << QString("playlist-move") << i << i + 2;
                mpv::qt::command(mpv, args);
            });
        }
        connect(delButton, &QPushButton::clicked, this, [=] {
            QVariantList args;
            args << QString("playlist-remove") << i;
            mpv::qt::command(mpv, args);
        });
    }
    if (row != -1) {
        modelIndex = playlistModel->index(row, 0);
        playlistView->setCurrentIndex(modelIndex);
    }
}

void MainWindow::showConfigDialog()
{
    auto d = new QDialog(this);
    d->setWindowTitle("fastplayer Settings");
    d->setWindowIcon(QIcon::fromTheme("settings"));

    auto vbox = new QVBoxLayout(d);
    auto tab = new QTabWidget;
    vbox->addWidget(tab);

    // General

    auto genTab = new QWidget;
    auto genForm = new QFormLayout(genTab);
    genForm->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    genTab->setLayout(genForm);
    auto seekStepSpin = new QSpinBox;
    seekStepSpin->setRange(1, 100);
    seekStepSpin->setValue(seekStep);
    seekStepSpin->setSuffix("s");
    connect(seekStepSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [&](int value) {
        seekStep = value;
        settings->setValue("seekStep", value);
    });
    auto volumeStepSpin = new QSpinBox;
    volumeStepSpin->setRange(1, 20);
    volumeStepSpin->setValue(volumeStep);
    volumeStepSpin->setSuffix("%");
    connect(volumeStepSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [&](int value) {
        volumeStep = value;
        settings->setValue("volumeStep", value);
    });

    genForm->addRow("Seek Step", seekStepSpin);
    genForm->addRow("Volume Step", volumeStepSpin);

    // UI
    auto uiTab = new QWidget;
    auto uiForm = new QFormLayout(uiTab);
    uiForm->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    uiTab->setLayout(uiForm);

    auto showPlayPauseCheck = new QCheckBox;
    showPlayPauseCheck->setChecked(showPlayPause);
    auto showSpeedCheck = new QCheckBox;
    showSpeedCheck->setChecked(showSpeed);
    auto showZoomCheck = new QCheckBox;
    showZoomCheck->setChecked(showZoom);
    auto showRotationCheck = new QCheckBox;
    showRotationCheck->setChecked(showRotation);
    auto showPanXCheck = new QCheckBox;
    showPanXCheck->setChecked(showPanX);
    auto showPanYCheck = new QCheckBox;
    showPanYCheck->setChecked(showPanY);
    auto showCropHCheck = new QCheckBox;
    showCropHCheck->setChecked(showCropH);
    auto showCropVCheck = new QCheckBox;
    showCropVCheck->setChecked(showCropV);

    auto showBrightnessCheck = new QCheckBox;
    showBrightnessCheck->setChecked(showBrightness);
    auto showContrastCheck = new QCheckBox;
    showContrastCheck->setChecked(showContrast);
    auto showSaturationCheck = new QCheckBox;
    showSaturationCheck->setChecked(showSaturation);
    auto showGammaCheck = new QCheckBox;
    showGammaCheck->setChecked(showGamma);
    auto showHueCheck = new QCheckBox;
    showHueCheck->setChecked(showHue);

    auto showProgressCheck = new QCheckBox;
    showProgressCheck->setChecked(showProgress);
    auto showMuteCheck = new QCheckBox;
    showMuteCheck->setChecked(showMute);
    auto showVolumeCheck = new QCheckBox;
    showVolumeCheck->setChecked(showVolume);
    auto showAudioCheck = new QCheckBox;
    showAudioCheck->setChecked(showAudio);
    auto showSubCheck = new QCheckBox;
    showSubCheck->setChecked(showSub);
    auto showSettingsCheck = new QCheckBox;
    showSettingsCheck->setChecked(showSettings);
    auto showPlaylistButtonCheck = new QCheckBox;
    showPlaylistButtonCheck->setChecked(showPlaylistButton);

    connect(showPlayPauseCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showPlayPause = checked;
        playButton->setVisible(checked);
        settings->setValue("showPlayPause", checked);
    });
    connect(showSpeedCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showSpeed = checked;
        speedSpin->setVisible(checked);
        settings->setValue("showSpeed", checked);
    });
    connect(showZoomCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showZoom = checked;
        zoomSpin->setVisible(checked);
        settings->setValue("showZoom", checked);
    });
    connect(showRotationCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showRotation = checked;
        rotationSpin->setVisible(checked);
        settings->setValue("showRotation", checked);
    });
    connect(showPanXCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showPanX = checked;
        panXSpin->setVisible(checked);
        settings->setValue("showPanX", checked);
    });
    connect(showPanYCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showPanY = checked;
        panYSpin->setVisible(checked);
        settings->setValue("showPanY", checked);
    });
    connect(showCropHCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showCropH = checked;
        cropHSpin->setVisible(checked);
        settings->setValue("showCropH", checked);
    });
    connect(showCropVCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showCropV = checked;
        cropVSpin->setVisible(checked);
        settings->setValue("showCropV", checked);
    });
    connect(showBrightnessCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showBrightness = checked;
        brightnessSpin->setVisible(checked);
        settings->setValue("showBrightness", checked);
    });
    connect(showContrastCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showContrast = checked;
        contrastSpin->setVisible(checked);
        settings->setValue("showContrast", checked);
    });
    connect(showSaturationCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showSaturation = checked;
        saturationSpin->setVisible(checked);
        settings->setValue("showSaturation", checked);
    });
    connect(showGammaCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showGamma = checked;
        gammaSpin->setVisible(checked);
        settings->setValue("showGamma", checked);
    });
    connect(showHueCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showHue = checked;
        hueSpin->setVisible(checked);
        settings->setValue("showHue", checked);
    });

    connect(showProgressCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showProgress = checked;
        progressBar->setVisible(checked);
        settings->setValue("showProgress", checked);
    });
    connect(showMuteCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showMute = checked;
        volumeButton->setVisible(checked);
        settings->setValue("showMute", checked);
    });
    connect(showVolumeCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showVolume = checked;
        volumeBar->setVisible(checked);
        settings->setValue("showVolume", checked);
    });
    connect(showAudioCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showAudio = checked;
        audioButton->setVisible(checked);
        settings->setValue("showAudio", checked);
    });
    connect(showSubCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showSub = checked;
        subButton->setVisible(checked);
        settings->setValue("showSub", checked);
    });
    connect(showSettingsCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showSettings = checked;
        configDialogButton->setVisible(checked);
        settings->setValue("showSettings", checked);
    });
    connect(showPlaylistButtonCheck, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState state) {
        bool checked = state == Qt::Checked;
        showPlaylistButton = checked;
        playlistButton->setVisible(checked);
        settings->setValue("showPlaylistButton", checked);
    });

    uiForm->addRow("Show Play Button", showPlayPauseCheck);
    uiForm->addRow("Show Speed Selector", showSpeedCheck);
    uiForm->addRow("Show Zoom Selector", showZoomCheck);
    uiForm->addRow("Show Rotation Selector", showRotationCheck);
    uiForm->addRow("Show Pan X Selector", showPanXCheck);
    uiForm->addRow("Show Pan Y Selector", showPanYCheck);
    uiForm->addRow("Show Crop Horizontally Selector", showCropHCheck);
    uiForm->addRow("Show Crop Vertically Selector", showCropVCheck);
    uiForm->addRow("Show Brightness Selector", showBrightnessCheck);
    uiForm->addRow("Show Contrast Selector", showContrastCheck);
    uiForm->addRow("Show Saturation Selector", showSaturationCheck);
    uiForm->addRow("Show Gamma Selector", showGammaCheck);
    uiForm->addRow("Show Hue Selector", showHueCheck);
    uiForm->addRow("Show Progress Bar", showProgressCheck);
    uiForm->addRow("Show Mute Button", showMuteCheck);
    uiForm->addRow("Show Volume Bar", showVolumeCheck);
    uiForm->addRow("Show Audio Track Selector", showAudioCheck);
    uiForm->addRow("Show Subtitle Selector", showSubCheck);
    uiForm->addRow("Show Settings Button", showSettingsCheck);
    uiForm->addRow("Show Playlist Button", showPlaylistButtonCheck);

    // Subtitle
    auto subTab = new QWidget;
    auto subForm = new QFormLayout(subTab);
    subTab->setLayout(subForm);
    subForm->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    auto fontCombo = new QFontComboBox;
    fontCombo->setCurrentFont(subFont);
    connect(fontCombo, &QFontComboBox::currentFontChanged, d, [&](const QFont& f) {
        subFont = f;
        mpv::qt::set_option_variant(mpv, "sub-font", f.family());
        settings->setValue("subFont", f);
    });

    auto fontSizeSpin = new QSpinBox;
    fontSizeSpin->setRange(10, 100);
    fontSizeSpin->setSingleStep(5);
    fontSizeSpin->setValue(subFontSize);
    connect(fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), d, [&](int value) {
        subFontSize = value;
        mpv::qt::set_option_variant(mpv, "sub-font-size", value);
        settings->setValue("subFontSize", value);
    });

    auto borderButton = new QPushButton("Pick");
    borderButton->setIcon(getSquareIcon(subBorderColor));
    connect(borderButton, &QPushButton::clicked, this, [&] {
        auto color = QColorDialog::getColor(subBorderColor, this, "Border Color", QColorDialog::ShowAlphaChannel);
        if (color.isValid()) {
            subBorderColor = color;
            auto btn = qobject_cast<QPushButton*>(sender());
            if (nullptr != btn) {
                btn->setIcon(getSquareIcon(subBorderColor));
            }
            mpv::qt::set_option_variant(mpv, "sub-border-color", subBorderColor.name(QColor::HexArgb));
            settings->setValue("subBorderColor", color);
        }
    });
    auto borderSizeSpin = new QSpinBox;
    borderSizeSpin->setRange(0, 10);
    borderSizeSpin->setValue(subBorderSize);
    connect(borderSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), d, [&](int value) {
        subBorderSize = value;
        mpv::qt::set_option_variant(mpv, "sub-border-size", value);
        settings->setValue("subBorderSize", value);
    });

    subColorButton = new QPushButton("Pick");
    subColorButton->setIcon(getSquareIcon(subColor));
    connect(subColorButton, &QPushButton::clicked, d, [&] {
        auto color = QColorDialog::getColor(subColor, d, "Subtitle Color", QColorDialog::ShowAlphaChannel);
        if (color.isValid()) {
            subColor = color;
            subColorButton->setIcon(getSquareIcon(subColor));
            mpv::qt::set_option_variant(mpv, "sub-color", subColor.name(QColor::HexArgb));
            settings->setValue("subColor", subColor);
        }
    });

    subForm->addRow("Font", fontCombo);
    subForm->addRow("Font Size", fontSizeSpin);
    subForm->addRow("Border Color", borderButton);
    subForm->addRow("Border Size", borderSizeSpin);
    subForm->addRow("Color", subColorButton);

    tab->addTab(genTab, "General");
    tab->addTab(uiTab, "UI");
    tab->addTab(subTab, "Subtitles");

    auto buttonBox = new QHBoxLayout;

    auto aboutQtButton = new QPushButton("About Qt");
    auto closeButton = new QPushButton("Close");
    auto spacer = new QSpacerItem(1, 1, QSizePolicy::Expanding);
    connect(aboutQtButton, &QPushButton::clicked, qApp, &QApplication::aboutQt);
    connect(closeButton, &QPushButton::clicked, d, &QDialog::close);
    buttonBox->addWidget(aboutQtButton);
    buttonBox->addSpacerItem(spacer);
    buttonBox->addWidget(closeButton);
    vbox->addLayout(buttonBox);

    d->exec();
}

QString MainWindow::timeStringFromInt(int time, bool withHour)
{
    if (withHour) {
        // 00:00:00
        int hour = qFloor(time / (60 * 60));
        int min = qFloor((time - hour * 60 * 60) / 60);
        int sec = time % 60;
        return QString("%1:%2:%3").arg(hour).arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
    }
    else {
        // 00:00
        int min = qFloor(time / 60);
        int sec = time % 60;
        return QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
    }
}

QIcon MainWindow::getSquareIcon(const QColor& color)
{
    QPixmap pix(24, 24);
    QPainter p;
    p.begin(&pix);
    p.fillRect(QRect(QPoint(0, 0), pix.size()), Qt::white);
    p.fillRect(QRect(QPoint(0, 0), pix.size()), color);
    p.end();
    return pix;
}

QString MainWindow::getColorString(const QColor& color)
{
    QString colorString = QString("%1/%2/%3/%4").arg(color.redF()).arg(color.greenF()).arg(color.blueF()).arg(color.alphaF());
    return colorString;
}

void MainWindow::handle_mpv_event(mpv_event* event)
{
    switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
        mpv_event_property* prop = (mpv_event_property*)event->data;
        onPropertyChanged(prop);

        break;
    }
    case MPV_EVENT_SHUTDOWN: {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
        break;
    }
    default:
        // Ignore uninteresting or unknown events.
        // qDebug() << "Unhandled: " << event->event_id;
        ;
    }
}

// This slot is invoked by wakeup() (through the mpv_events signal).
void MainWindow::onMpvEvents()
{
    // Process all events, until the event queue is empty.
    while (mpv) {
        mpv_event* event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) {
            break;
        }
        handle_mpv_event(event);
    }
}

void MainWindow::updateSpeed(int speedPerc)
{
    playbackSpeed = speedPerc;
    double speed = (double)speedPerc / 100;
    mpv::qt::set_property_variant(mpv, "speed", speed);
    settings->setValue("playbackSpeed", speedPerc);
}

void MainWindow::seek(bool forward)
{
    if (length > 0) {
        int delta = forward ? seekStep : -seekStep;
        int newTime = qBound(0, time + delta, length);
        mpv::qt::set_property_variant(mpv, "playback-time", newTime);
    }
}

void MainWindow::playPauseClicked()
{
    if (paused && length > 0 && mpv::qt::get_property_variant(mpv, "time-remaining").toInt() == 0) {
        int index = mpv::qt::get_property(mpv, "playlist-pos").toInt();
        if (index == -1) {
            index = mpv::qt::get_property(mpv, "playlist-count").toInt() - 1;
            if (index >= 0) {
                mpv::qt::set_property_variant(mpv, "playlist-pos", index);
                mpv::qt::set_property_variant(mpv, "pause", false);
            }
        }
    }
    else {
        mpv::qt::set_property_variant(mpv, "pause", !paused);
    }
}

void MainWindow::showCustomMenu(const QPoint& pos)
{
    QMenu* menu = new QMenu(this);
    QAction* a = menu->addAction(tr("&Open File"), this, &MainWindow::onFileOpen);
    a->setToolTip(tr("Open a file"));
    a = menu->addAction(tr("&Settings"), this, &MainWindow::showConfigDialog);
    a->setToolTip(tr("View Settings"));
    menu->addSeparator();
    a = menu->addAction(tr("&Quit"), this, &MainWindow::close);
    a->setToolTip(tr("Quit Application"));
    menu->exec(this->mapToGlobal(pos));
    menu->deleteLater();
}

void MainWindow::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
    }
    else {
        showFullScreen();
    }
}

void MainWindow::onFileOpen()
{

    QUrl fileUrl = QFileDialog::getOpenFileUrl(this, "Open file");
    if (mpv) {
        loadFiles({ fileUrl });
    }
}

void MainWindow::loadFiles(QList<QUrl> urls)
{
    for (int i = 0; i < urls.count(); ++i) {
        QString file = urls.at(i).toLocalFile();
        QFileInfo info(file);
        QString ext = info.suffix().toLower();
        const QByteArray c_filename = file.toUtf8();

        if (!info.exists()) {
            continue;
        }
        if (info.isDir()) {
            QDir dir(info.absoluteFilePath());
            auto infoList = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
            QList<QUrl> subDir;
            for (int i = 0; i < infoList.count(); ++i) {
                QUrl url = QUrl::fromLocalFile(infoList.at(i).absoluteFilePath());
                subDir.append(url);
            }
            loadFiles(subDir);
        }
        else if (supportedSubs.contains(ext)) {
            const char* args[] = { "sub-add", c_filename.data(), NULL };
            mpv_command(mpv, args);
        }
        else if (videoExt.contains(ext)) {
            const QByteArray c_flag = "append-play";
            const char* args[] = { "loadfile", c_filename.data(), c_flag.data(), NULL };
            mpv_command(mpv, args);
        }
    }
    if (paused && eofReached) {
        mpv::qt::set_property_variant(mpv, "pause", !paused);
        eofReached = false;
    }
}
Q_SCRIPTABLE void MainWindow::loadFiles(const QStringList& files)
{
    for (const QString& file : files) {
        QFileInfo info(file);
        if (!info.exists() || info.isDir()) {
            continue;
        }

        const QByteArray c_filename = file.toUtf8();
        QString ext = info.suffix().toLower();
        if (supportedSubs.contains(ext)) {
            const char* args[] = { "sub-add", c_filename.data(), NULL };
            mpv_command(mpv, args);
        }
        else if (videoExt.contains(ext)) {
            const QByteArray c_flag = "append-play";
            const char* args[] = { "loadfile", c_filename.data(), c_flag.data(), NULL };
            mpv_command(mpv, args);
        }
    }
    if (paused && eofReached) {
        mpv::qt::set_property_variant(mpv, "pause", !paused);
        eofReached = false;
    }
}

void MainWindow::onNewWindow()
{
    (new MainWindow())->show();
}

MainWindow::~MainWindow()
{
    bool unreg = QDBusConnection::sessionBus().unregisterService(SERVICE_NAME);
    qInfo() << "unreg" << unreg;

    if (mpv) {
        mpv_terminate_destroy(mpv);
    }
}
