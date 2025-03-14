#pragma once

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QListView>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QString>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <mpv/client.h>

#include <QDebug>

#define SERVICE_NAME "local.fastplayer"

class QTextEdit;
class PlaylistStyle;

class ListView : public QListView
{
    using QListView::QListView;
    // TODO: improve listview
};

class QDBusServiceWatcher;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", SERVICE_NAME)

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

public Q_SLOTS:
    Q_SCRIPTABLE void loadFiles(const QStringList& files);

private slots:
    void registerDBus(const QString& service);
    void loadFiles(QList<QUrl> urls);
    void onFileOpen();
    void onNewWindow();
    void onMpvEvents();
    void updateSpeed(int speedPerc);
    void seek(bool forward);
    void playPauseClicked();
    void toggleFullscreen();
    void showCustomMenu(const QPoint& pos);
    void showConfigDialog();

signals:
    void mpv_events();

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dragLeaveEvent(QDragLeaveEvent*) override;
    void dragMoveEvent(QDragMoveEvent*) override;
    void dropEvent(QDropEvent*) override;

private:
    QSettings* settings;
    QWidget* mpvWidget;
    mpv_handle* mpv;
    QDBusServiceWatcher* watcher;

    QString draggedFile;
    bool muted;
    bool paused;
    int time;
    int length;
    QStringList videoExt;
    QStringList supportedSubs;
    QList<int> boundKeys;
    int cropH;
    int cropV;

    // settings
    int seekStep;
    int volumeStep;
    int currentVolume;
    QColor subColor;
    QColor subBorderColor;
    int subBorderSize;
    QFont subFont;
    int subFontSize;
    int playbackSpeed;
    bool showPlayPause;
    bool showSpeed;
    bool showZoom;
    bool showRotation;
    bool showPanX;
    bool showPanY;
    bool showCropH;
    bool showCropV;
    bool showBrightness;
    bool showContrast;
    bool showSaturation;
    bool showGamma;
    bool showHue;
    bool showProgress;
    bool showMute;
    bool showVolume;
    bool showAudio;
    bool showSub;
    bool showSettings;
    bool showPlaylistButton;
    bool playlistVisible;
    bool eofReached;

    // UI
    QIcon playIcon;
    QIcon pauseIcon;

    QIcon volumeMutedIcon;
    QIcon volumeLowIcon;
    QIcon volumeMediumIcon;
    QIcon volumeHighIcon;

    QVBoxLayout* mainLayout;
    QWidget* controlBar;
    QHBoxLayout* controlLayout;
    QPushButton* playButton;
    QSpinBox* speedSpin;
    QSpinBox* zoomSpin;
    QSpinBox* rotationSpin;
    QSpinBox* panXSpin;
    QSpinBox* panYSpin;
    QSpinBox* cropHSpin;
    QSpinBox* cropVSpin;

    QSpinBox* brightnessSpin;
    QSpinBox* contrastSpin;
    QSpinBox* saturationSpin;
    QSpinBox* gammaSpin;
    QSpinBox* hueSpin;

    QProgressBar* progressBar;
    QPushButton* volumeButton;
    QProgressBar* volumeBar;
    QPushButton* audioButton;
    QPushButton* subButton;
    QPushButton* configDialogButton;

    QPushButton* playlistButton;
    QDockWidget* playlistDock;
    ListView* playlistView;
    QStandardItemModel* playlistModel;
    PlaylistStyle* playlistStyle;

    // Dialog
    QPushButton* subColorButton;
    //
    void configureMpv();
    void loadConfig();
    void onFileLoaded();
    void updateTracks(QVariantList list = QVariantList());
    void onPropertyChanged(mpv_event_property* prop);
    void updateProgress();
    void progressClicked(int posX);
    void showProgressTooltip(QPoint globalPos, int posX);
    void updateVolume();
    void volumeBarClicked(int posX);
    void showVolumeTooltip(QPoint globalPos, int posX);
    void stepVolume(bool increase);
    void updatePlaylist(QVariantList list);

    QString timeStringFromInt(int time, bool withHour);
    QIcon getSquareIcon(const QColor& color);
    QString getColorString(const QColor& color);
    void handle_mpv_event(mpv_event* event);

protected:
    void closeEvent(QCloseEvent* event);
};
