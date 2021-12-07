#include "mainwindow.h"
#include "app.h"

#include <QtDebug>
#include <QtCore>
#include <QSplashScreen>

#include "windows.h"
#include "winuser.h"

#include <QFile>
#include <QCollator>

#include "fileformats/matlabfile.h"

void test()
{
    mat_t *matfp;
        const char *FILENAME = "export.mat";

        char *structname = "Test";
        const char *fieldnames[1] = { "Data"};
        const char *Datafieldnames[3] = { "name", "unit", "value" };

        //Open file
        matfp = Mat_CreateVer(FILENAME, NULL, MAT_FT_MAT5);

        size_t structdim0[2] = { 1, 1 };
        matvar_t* matstruct0 = Mat_VarCreateStruct(structname, 2, structdim0, 0, 0); //main struct: Test

        Mat_VarAddStructField(matstruct0, "field1");


        //Note: Matlab starts counting at 1

        //create nested structs:

        //Test(1).Data
        char* mystring0 = "Data"; //Test(1).Data
        size_t dim3[2] = { 1, 4 };
        auto field3 = Mat_VarCreate(NULL, MAT_C_CHAR, MAT_T_UTF8, 2, dim3, mystring0, 0);
        Mat_VarSetStructFieldByName(matstruct0, "field1", 0, field3); //save pointer into main struct - freeing matstruct1 causes a segfault at Mat_VarFree(matstruct0);


        //save main struct
        Mat_VarWrite(matfp, matstruct0, MAT_COMPRESSION_NONE);
        //cleanup
        Mat_VarFree(matstruct0);


        //Close file
        Mat_Close(matfp);
}

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

//    MatlabFile f("E:/Shared/1/APS.mat");
//    f.read();


    MainWindow w;
    w.showMaximized();
    splash.finish(&w);
    return a.exec();
}
