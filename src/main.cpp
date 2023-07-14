#include "mainwindow.h"
#include "app.h"

#include <QtDebug>
#include <QtCore>
#include <QSplashScreen>

#include "windows.h"
#include "winuser.h"

#include <QFile>
#include <QCollator>

#include "logging.h"
INITIALIZE_EASYLOGGINGPP

#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxchart.h"
#include "xlsxcellrange.h"
#include "xlsxcellrange.h"
#include "xlsxlineformat.h"

int main(int argc, char *argv[])
{
    bool logToFile = false;
    if (argc > 1) {
        for (int i=1; i<argc; ++i) {
            if (strcmp(argv[i], "-l") || strcmp(argv[i], "--log")) logToFile = true;
        }
    }

    START_EASYLOGGINGPP(argc, argv);
    el::Loggers::addFlag(el::LoggingFlag::LogDetailedCrashReason);
    el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);

    if (logToFile) {
        el::Configurations fileConf("file.conf");
        el::Loggers::reconfigureAllLoggers(fileConf);
    }
    else
    {
        el::Configurations consoleConf("console.conf");
        el::Loggers::reconfigureAllLoggers(consoleConf);
    }

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

    QXlsx::Document xlsx;
    xlsx.write(1,1, "x");
    xlsx.write(1,2, "y");
    for (int i=1; i<10; ++i) {
        xlsx.write(i+1, 1, i);
        xlsx.write(i+1, 2, i*i);
    }
    xlsx.addSheet("Chart1", QXlsx::AbstractSheet::ST_ChartSheet);
    QXlsx::Chartsheet *sheet = static_cast<QXlsx::Chartsheet*>(xlsx.currentSheet());
    QXlsx::Chart *chart = sheet->chart();
    chart->setChartType(QXlsx::Chart::CT_ScatterChart);
    chart->addSeries(QXlsx::CellRange("A1:B10"), xlsx.sheet("Sheet1"));
    chart->setChartStyle(QXlsx::Chart::CS_ScatterLineMarker);
    QXlsx::LineFormat format;
    format.setColor(QColor(Qt::lightGray));
    format.setWidth(1);
    chart->setSeriesFormat(0, format);
    xlsx.saveAs("chartsheet1.xlsx");

    QPixmap pixmap(":/icons/splash.png");
    QSplashScreen splash(pixmap);
    splash.show();
    splash.showMessage(QString(DEEPSEABASE_VERSION)+" - Подождите, идет загрузка записей...",Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
    a.processEvents();

    MainWindow w;
    w.showMaximized();
    splash.finish(&w);
    return a.exec();
}
