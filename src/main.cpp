#include "mainwindow.h"
#include "app.h"

#include <QtDebug>
#include <QtCore>
#include <QSplashScreen>

#include "windows.h"
#include "winuser.h"

#include <QFile>
#include <QCollator>

int main(int argc, char *argv[])
{
    //This mutex is used to prevent the user from installing new versions
    //of app while app is still running, and to prevent
    //the user from uninstalling a running application.
    CreateMutex(NULL,false,L"DeepSeaMutex");

    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR","1");
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setApplicationName("DeepSeaBase");
    QCoreApplication::setOrganizationName("DeepSeaBase");
    QCoreApplication::setApplicationVersion(QString(DEEPSEABASE_VERSION));

    Application a(argc, argv);

    QPixmap pixmap(":/icons/splash.png");
    QSplashScreen splash(pixmap);
    splash.show();
    splash.showMessage("Подождите, идет загрузка записей...",Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
    a.processEvents();

    MainWindow w;
    w.showMaximized();
    splash.finish(&w);
    return a.exec();
}
