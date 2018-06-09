#include "settings.h"
#include <QSettings>
#include <QDir>

QString restoreLeftCameraAddress()
{
    QSettings settings;
    return settings.value("leftCameraAddress", "rtsp://172.168.100.8/H.264/media.smp").toString();
}

QString restoreRightCameraAddress()
{
    QSettings settings;
    return settings.value("rightCameraAddress", "rtsp://172.168.100.8/H.264/media.smp").toString();
}

QString restoreDefaultVideoFolder()
{
    QSettings settings;

    return settings.value("defaultVideoFolder", QDir::homePath() + "/HDD_Files_2").toString();
    // return settings.value("defaultVideoFolder", "/hdd/videoTemp").toString();
}
