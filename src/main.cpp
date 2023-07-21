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

    using namespace QXlsx;

    QXlsx::Document xlsx;
    xlsx.write(1,1, "x");
    xlsx.write(1,2, "y");
    for (int i=1; i<10; ++i) {
        xlsx.write(i+1, 1, i);
        xlsx.write(i+1, 2, (i % 2)*2);
        xlsx.write(i+1, 3, i*i+10);
    }
    xlsx.addSheet("Chart1", QXlsx::AbstractSheet::ST_ChartSheet);
    QXlsx::Chartsheet *sheet = static_cast<QXlsx::Chartsheet*>(xlsx.currentSheet());
    QXlsx::Chart *chart = sheet->chart();
    chart->setChartType(QXlsx::Chart::CT_ScatterChart);
//    chart->addSeries(QXlsx::CellRange("A1:B10"), xlsx.sheet("Sheet1"), true);
    chart->addSeries(QXlsx::CellRange("A1:A10"), QXlsx::CellRange("B1:B10"), xlsx.sheet("Sheet1"), true);
//    chart->addSeries(QXlsx::CellRange("A1:A10"), QXlsx::CellRange("C1:C10"), xlsx.sheet("Sheet1"), true);

    auto b = chart->addAxis(QXlsx::Axis::Type::Val, QXlsx::Axis::Position::Bottom);
    b->setTitle("bottom");
    auto l = chart->addAxis(QXlsx::Axis::Type::Val, QXlsx::Axis::Position::Left);
    l->setTitle("left");
    l->setCrossAxis(b->id());
//    int br = chart->addAxis(QXlsx::Axis::Type::Val, QXlsx::Axis::Position::Bottom, -1, "");
//    int r = chart->addAxis(QXlsx::Axis::Type::Val, QXlsx::Axis::Position::Right, br, "right");
    chart->setSeriesAxes(0, {b->id(), l->id()});
//    chart->setSeriesAxes(1, {br,r});

    QXlsx::Color color(QXlsx::Color::ColorType::RGBColor, QColor(145,25,230));


    QXlsx::ShapeProperties shape;
    shape.setBlackWhiteMode(ShapeProperties::BlackWhiteMode::Gray);

    PresetGeometry2D g;
    g.prst = ShapeType::donut;
    shape.setPresetGeometry(g);

    QXlsx::LineFormat line;
    line.setWidth(10.0);




    QXlsx::FillProperties fill;
    fill.setType(FillProperties::FillType::SolidFill);
    fill.setColor(color);

    line.setFill(fill);
    shape.setLine(line);


    chart->setSeriesShape(0, shape);
    chart->setSeriesMarkerFormat(0, MarkerFormat(MarkerFormat::MarkerType::None));
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
