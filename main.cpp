#include "mainwindow.h"
#include <QApplication>

QString manufactureName()
{
    return QString("Radioavionica");
}

QString applicationName()
{
    return QString("VideoSystem");
}

void setApplicationSettingsProperties()
{
    QCoreApplication::setOrganizationName(manufactureName());
    QCoreApplication::setOrganizationDomain("radioavionica.ru");
    QCoreApplication::setApplicationName(applicationName());
}

Q_COREAPP_STARTUP_FUNCTION(setApplicationSettingsProperties)


int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<std::vector<signed short>>("std::vector<signed short>");
    qRegisterMetaType<std::vector<unsigned char>>("std::vector<unsigned char>");
    qRegisterMetaType<GstClockTime>("GstClockTime");
    qRegisterMetaType<QSharedPointer<std::vector<signed short>>>("QSharedPointer<std::vector<signed short>>");
    qRegisterMetaType<QSharedPointer<std::vector<unsigned char>>>("QSharedPointer<std::vector<unsigned char>>");
    MainWindow w;
    w.show();

    int ret = a.exec();

    return ret;
}
