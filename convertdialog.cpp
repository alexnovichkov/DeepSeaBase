#include "convertdialog.h"

ConvertDialog::ConvertDialog(QWidget *parent) :
    QDialog(parent)
{

}

void ConvertDialog::setDatabase(QList<DfdFileDescriptor *> *dataBase)
{
    this->dataBase = dataBase;
}

QStringList ConvertDialog::getSpfFile(const QVector<int> &indexes, const QString &dir)
{
    QStringList spfFile;
    spfFile << "[DeepSeaProjectFile]";
    spfFile << "MainWindow=133,139,1280,572";
    spfFile << QString("PanelsNum=%1").arg(indexes.size()); // число панелей равно числу файлов для обработки
    spfFile << "GWColors=12632256,12632256,8421504,8388736";
    spfFile << "GColor0=180";
    spfFile << "GColor1=8388608";
    spfFile << "GColor2=8421376";
    spfFile << "GColor3=16256";
    spfFile << "GColor4=33023";
    spfFile << "GColor5=16711680";
    spfFile << "GColor6=32896";
    spfFile << "GColor7=16776960";
    spfFile << "GColor8=12644592";
    spfFile << "GColor9=8388736";
    spfFile << "GColor10=16711935";
    spfFile << "GColor11=30720";
    spfFile << "GColor12=0";
    spfFile << "GColor13=8421631";
    spfFile << "GColor14=16744576";
    spfFile << "GColor15=10789024";
    spfFile << "SonColor=0";
    spfFile << QString("SoDatDir=%1").arg(dir); // файлы отсортированы по папкам
    spfFile << QString("DSDatDir=%1").arg(dir); // файлы сохраняются в ту же папку
    spfFile << "TpNameDat=1"; // к имени файла добавляется дата и время создания

    for (int i=0; i<indexes.size(); ++i) {
        DfdFileDescriptor *dfd = dataBase->at(indexes.at(i));
        spfFile << QString("[Panel%1]").arg(i);
        spfFile << QString("NmPan=п.%1").arg(i);
        spfFile << QString("PanelPar=%1,%2,%3")
                   .arg(i)
                   .arg(panelTypeForMethod(method)) //однооконная графическая панель
                   .arg(i+1);
        spfFile << "tVal=0"; // отображение величин: степенное

        spfFile << QString("[Panel%1\\Wind1]").arg(i); // параметры окна
        spfFile << QString("WinNm=%1").arg(dataTypeDescription(method)); // описание окна
        spfFile << QString("ProcMethod=%1,%2")
                   .arg(methodDll(method))
                   .arg(method);
        spfFile << QString("FileNm=%1").arg(dfd->dfdFileName);
        QStringList channelsList;
        foreach(int ch, channels) channelsList << QString::number(ch);
        spfFile << "Channels="+channelsList.join(',');
        spfFile << QString("ActChannel=%1").arg(activeChannel);
        spfFile << QString("BaseChannel=%1").arg(baseChannel);
        spfFile << "MinMax=*,*,*,*";
        spfFile << "AStrip=0";
        spfFile << "StepBack=(00000000)"; // TODO: добавить возможность устанавливать перекрытие
        spfFile << "ShiftDat=0"; // TODO: добавить возможность устанавливать смещение
        // число отсчетов в канале
        // TODO: добавить возможность устанавливать правую границу выборки
        quint32 NI = dfd->NumInd *
                     dfd->channels.at(activeChannel)->ChanBlockSize
                     / dfd->BlockSize;
        spfFile << QString("Duration=%1").arg(NI);
        spfFile << "TablName=";
        spfFile << "ViewPar=1,0,1,0,1,1,0";

        spfFile << QString("[Panel%1\\Wind1\\Method]").arg(i);
        spfFile.append(getMethodSettings());
//        spfFile << "YName"+dfd->channels.at(activeChannel)->YName;
//        spfFile << "BlockIn=8192"; // 512,1024,2048,4096,8192
//        spfFile << "Wind=Хеннинга";
//        spfFile << "TypeAver=линейное";
//        spfFile << "NAver=150";
//        spfFile << "Values=измеряемые";
//        spfFile << "TypeScale=в децибелах";
    }
    return spfFile;
}

QStringList ConvertDialog::getMethodSettings()
{
    return QStringList();
}
