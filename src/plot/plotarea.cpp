#include "plotarea.h"

#include <QtWidgets>
#include "plot/plot.h"
#include "spectrogram.h"
#include "plot/curve.h"
#include "settings.h"
#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "plot/qcppointmarker.h"
#include "plotmodel.h"
#include "channelsmimedata.h"
#include "correctiondialog.h"
#include <QAxObject>

#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxchart.h"
#include "qcpplot.h"

PlotArea::PlotArea(int index, QWidget *parent)
    : ads::CDockWidget(QString("График %1").arg(index), parent)
{DD;
    setObjectName(QString("График %1").arg(index));
    setAcceptDrops(true);
    setAsCurrentTab();
    setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);
    setFeature(ads::CDockWidget::CustomCloseHandling, true);
    setFeature(ads::CDockWidget::DockWidgetFocusable, true);

    bool toggleAutoscaleAtStartup = false;

    autoscaleXAct = new QAction("Автомасштабирование по оси X", this);
    autoscaleXAct->setIcon(QIcon(":/icons/autoscale-x.png"));
    autoscaleXAct->setCheckable(true);

    connect(autoscaleXAct, &QAction::toggled, [this](bool toggled){
        if (m_plot) m_plot->toggleAutoscale(Enums::AxisType::atBottom /* x axis */,toggled);
        se->setSetting("autoscale-x", toggled);
    });

    autoscaleYAct = new QAction("Автомасштабирование по оси Y", this);
    autoscaleYAct->setIcon(QIcon(":/icons/autoscale-y-main.png"));
    autoscaleYAct->setCheckable(true);
    connect(autoscaleYAct, &QAction::toggled, [this](bool toggled){
        if (m_plot) m_plot->toggleAutoscale(Enums::AxisType::atLeft /* y axis */,toggled);
        se->setSetting("autoscale-y", toggled);
    });

    autoscaleYSlaveAct = new QAction("Автомасштабирование по правой оси Y", this);
    autoscaleYSlaveAct->setIcon(QIcon(":/icons/autoscale-y-slave.png"));
    autoscaleYSlaveAct->setCheckable(true);
    connect(autoscaleYSlaveAct, &QAction::toggled, [this](bool toggled){
        if (m_plot) m_plot->toggleAutoscale(Enums::AxisType::atRight /* y slave axis */,toggled);
        se->setSetting("autoscale-y-slave", toggled);
    });

    if (toggleAutoscaleAtStartup) {
        bool autoscale = se->getSetting("autoscale-x", true).toBool();
        autoscaleXAct->setChecked(autoscale);
        if (m_plot) m_plot->toggleAutoscale(Enums::AxisType::atBottom /* x axis */, autoscale);

        autoscale = se->getSetting("autoscale-y", true).toBool();
        autoscaleYAct->setChecked(autoscale);
        if (m_plot) m_plot->toggleAutoscale(Enums::AxisType::atLeft /* y axis */, autoscale);

        autoscale = se->getSetting("autoscale-y-slave", true).toBool();
        autoscaleYSlaveAct->setChecked(autoscale);
        if (m_plot) m_plot->toggleAutoscale(Enums::AxisType::atRight /* y slave axis */, autoscale);
    }
    else {
        autoscaleXAct->setChecked(true);
        autoscaleYAct->setChecked(true);
        autoscaleYSlaveAct->setChecked(true);
    }

    autoscaleAllAct  = new QAction("Автомасштабирование по всем осям", this);
    autoscaleAllAct->setIcon(QIcon(":/icons/autoscale-all.png"));
    connect(autoscaleAllAct, &QAction::triggered, [this](){
        if (m_plot) m_plot->autoscale();
    });

    removeLabelsAct  = new QAction("Удалить все подписи", this);
    removeLabelsAct->setIcon(QIcon(":/icons/remove-labels.png"));
    connect(removeLabelsAct, &QAction::triggered, [this](){
        if (m_plot) m_plot->removeLabels();
    });

    previousDescriptorAct = new QAction("Предыдущая запись", this);
    previousDescriptorAct->setIcon(QIcon(":/icons/rminus.png"));
    previousDescriptorAct->setToolTip(QString("%1 <font color=gray size=-1>Ctrl+Up</font>")
                          .arg("Предыдущая запись"));
    connect(previousDescriptorAct, &QAction::triggered, [this](bool checked){
        emit descriptorRequested(-1, checked);});

    nextDescriptorAct = new QAction("Следущая запись", this);
    nextDescriptorAct->setIcon(QIcon(":/icons/rplus.png"));
    nextDescriptorAct->setToolTip(QString("%1 <font color=gray size=-1>Ctrl+Down</font>")
                          .arg("Следущая запись"));
    connect(nextDescriptorAct, &QAction::triggered, [this](bool checked){
        emit descriptorRequested(1, checked);});

    arbitraryDescriptorAct = new QAction("Произвольная запись", this);
    arbitraryDescriptorAct->setIcon(QIcon(":/icons/rany.png"));
    arbitraryDescriptorAct->setCheckable(true);
    connect(arbitraryDescriptorAct, &QAction::triggered, [this](bool checked){emit descriptorRequested(0, checked);});

    cycleChannelsUpAct = new QAction("Предыдущий канал", this);
    cycleChannelsUpAct->setIcon(QIcon(":/icons/cminus.png"));
    connect(cycleChannelsUpAct, &QAction::triggered, [this](){
        if (m_plot) m_plot->cycleChannels(true);
    });

    cycleChannelsDownAct = new QAction("Следующий канал", this);
    cycleChannelsDownAct->setIcon(QIcon(":/icons/cplus.png"));
    connect(cycleChannelsDownAct, &QAction::triggered, [this](){
        if (m_plot) m_plot->cycleChannels(false);
    });

    clearPlotAct  = new QAction(QString("Очистить график"), this);
    clearPlotAct->setIcon(QIcon(":/icons/cross.png"));
    connect(clearPlotAct, &QAction::triggered, [=](){
        if (m_plot) {
            resetCycling();
            m_plot->deleteAllCurves(false);
        }
    });

    savePlotAct = new QAction(QString("Сохранить график..."), this);
    savePlotAct->setIcon(QIcon(":/icons/picture.ico"));
    connect(savePlotAct, &QAction::triggered, [=](){
        if (m_plot) m_plot->savePlot();
    });

    cursorBoxAct = new QAction(QString("Показать окно курсоров"), this);
    cursorBoxAct->setIcon(QIcon(":/icons/tracking.png"));
    cursorBoxAct->setCheckable(true);
    cursorBoxAct->setObjectName("trackingCursor");
    connect(cursorBoxAct, &QAction::triggered, [=](){
        if (m_plot) m_plot->switchCursorBox();
    });

    copyToClipboardAct = new QAction(QString("Копировать в буфер обмена"), this);
    copyToClipboardAct->setIcon(QIcon(":/icons/clipboard.png"));
    connect(copyToClipboardAct, &QAction::triggered, [=](){
        if (m_plot) m_plot->copyToClipboard(true);
    });

    printPlotAct = new QAction(QString("Распечатать рисунок"), this);
    printPlotAct->setIcon(QIcon(":/icons/print.png"));
    connect(printPlotAct, &QAction::triggered, [=](){
        if (m_plot) m_plot->print();
    });

    interactionModeAct = new QAction(QString("Включить режим изменения данных"), this);
    interactionModeAct->setIcon(QIcon(":/icons/data.png"));
    interactionModeAct->setCheckable(true);
    connect(interactionModeAct, &QAction::triggered, [=](){
        if (m_plot) m_plot->switchInteractionMode();
    });

    setToolBarIconSize(QSize(24,24), StateDocked);
    setToolBarIconSize(QSize(24,24), StateFloating);
    setToolBarIconSize(QSize(24,24), StateHidden);
    setToolBarStyle(Qt::ToolButtonIconOnly, StateDocked);
    setToolBarStyle(Qt::ToolButtonIconOnly, StateFloating);
    setToolBarStyle(Qt::ToolButtonIconOnly, StateHidden);
    auto scaleToolBar = createDefaultToolBar();
    scaleToolBar->setIconSize(QSize(24,24));
//    scaleToolBar->setOrientation(Qt::Vertical);
    scaleToolBar->addAction(autoscaleXAct);
    scaleToolBar->addAction(autoscaleYAct);
    scaleToolBar->addAction(autoscaleYSlaveAct);
    scaleToolBar->addAction(autoscaleAllAct);
    scaleToolBar->addSeparator();
    scaleToolBar->addAction(removeLabelsAct);
    scaleToolBar->addSeparator();
    scaleToolBar->addAction(previousDescriptorAct);
    scaleToolBar->addAction(nextDescriptorAct);
    scaleToolBar->addAction(arbitraryDescriptorAct);
    scaleToolBar->addAction(cycleChannelsUpAct);
    scaleToolBar->addAction(cycleChannelsDownAct);
    scaleToolBar->addSeparator();
    scaleToolBar->addAction(clearPlotAct);
    scaleToolBar->addAction(savePlotAct);
    scaleToolBar->addAction(copyToClipboardAct);
    scaleToolBar->addAction(printPlotAct);
    scaleToolBar->addAction(interactionModeAct);
    scaleToolBar->addAction(cursorBoxAct);

    splitter = new QSplitter(Qt::Horizontal, this);
    infoLabel = new QLabel("- Перетащите сюда каналы, чтобы построить их графики\n"
                                "- Если зажать Ctrl, то будут построены графики для всех\n"
                                "  выделенных файлов", this);
    QFont font;
    font.setPointSize(12);
    infoLabel->setFont(font);
    infoLabel->setAlignment(Qt::AlignCenter);

    stackedL = new QStackedLayout;
    stackedL->addWidget(infoLabel);
    stackedL->addWidget(splitter);
    stackedL->setCurrentIndex(0);

    auto w = new QWidget(this);
    w->setLayout(stackedL);
    setWidget(w);
}

PlotArea::~PlotArea()
{
//    if (m_plot) m_plot->deleteAllCurves(true);
}

QCPPlot *PlotArea::plot()
{DD;
    return m_plot;
}

void PlotArea::addPlot(Enums::PlotType type)
{DD;
    if (m_plot) {
        if (m_plot->type() != type) {
            if (m_plot->toolBarWidget()) {
                toolBar()->removeAction(toolBarAction);
                delete m_plot->toolBarWidget(); //при помещении на тулбар виджет меняет хозяина,
                //так что приходится принудительно удалять
            }
            delete m_plot->legend(); //при помещении в сплиттер легенда меняет хозяина
            delete m_plot;
        }
        else return;
    }
    else {
        stackedL->setCurrentIndex(1);
    }
    m_plot = new QCPPlot(type, this);
    switch (type) {
        case Enums::PlotType::Spectrogram:
            setIcon(QIcon(":/icons/spectrocurve.png"));
            break;
        case Enums::PlotType::Octave:
            setIcon(QIcon(":/icons/barcurve.png"));
            break;
        case Enums::PlotType::Time:
            setIcon(QIcon(":/icons/timecurve.png"));
            break;
        case Enums::PlotType::General:
            setIcon(QIcon(":/icons/linecurve.png"));
            break;
    }
    if (!m_plot) return;

    if (m_plot->toolBarWidget()) {
        toolBarAction = toolBar()->addWidget(m_plot->toolBarWidget());
    }

//    plotsLayout->addWidget(m_plot, 1,0);
//    plotsLayout->addWidget(m_plot->legend(), 1,1);
//    plotsLayout->setColumnStretch(0, 1);
    if (splitter->count()>0) {
        splitter->replaceWidget(0, m_plot);
        splitter->replaceWidget(1, m_plot->legend());
    }
    else {
        splitter->addWidget(m_plot);
        splitter->addWidget(m_plot->legend());
        splitter->setStretchFactor(0,1);
    }

//    splitter->setStretchFactor(1,10);
    //splitter->setSizes({30,10});
//    m_plot->legend()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(m_plot, SIGNAL(curvesCountChanged()), this, SIGNAL(curvesCountChanged()));
    connect(m_plot, SIGNAL(channelPlotted(Channel*)), this, SIGNAL(channelPlotted(Channel*)));
    connect(m_plot, SIGNAL(curveDeleted(Channel*)), this, SIGNAL(curveDeleted(Channel*)));
    connect(m_plot, SIGNAL(needPlotChannels(bool,QVector<Channel*>)), this, SIGNAL(needPlotChannels(bool,QVector<Channel*>)));
    connect(m_plot, SIGNAL(focusThisPlot()), this, SLOT(setFocus()));
    connect(m_plot, SIGNAL(trackingPanelCloseRequested()), cursorBoxAct, SLOT(toggle()));
    connect(m_plot, SIGNAL(saveHorizontalSlice(QVector<double>)), this, SIGNAL(saveHorizontalSlice(QVector<double>)));
    connect(m_plot, SIGNAL(saveVerticalSlice(QVector<double>)), this, SIGNAL(saveVerticalSlice(QVector<double>)));
    connect(m_plot, SIGNAL(saveTimeSegment(QVector<FileDescriptor*>,double,double)), this,
            SIGNAL(saveTimeSegment(QVector<FileDescriptor*>,double,double)));

    bool autoscale = se->getSetting("autoscale-x", true).toBool();
    m_plot->toggleAutoscale(Enums::AxisType::atBottom /* x axis */, autoscale);
    autoscale = se->getSetting("autoscale-y", true).toBool();
    m_plot->toggleAutoscale(Enums::AxisType::atLeft /* y axis */, autoscale);
    autoscale = se->getSetting("autoscale-y-slave", true).toBool();
    m_plot->toggleAutoscale(Enums::AxisType::atRight /* y slave axis */, autoscale);

    emit curvesCountChanged();
}

Enums::PlotType PlotArea::getPlotType(const QVector<Channel *> &channels)
{
    auto type = Enums::PlotType::General;
    if (!channels.isEmpty()) {
        if (channels.first()->type() == Descriptor::TimeResponse)
            type = Enums::PlotType::Time;
        else if (channels.first()->data()->blocksCount()>1)
            type = Enums::PlotType::Spectrogram;
        else if (channels.first()->octaveType() != 0)
            type = Enums::PlotType::Octave;
    }
    return type;
}

bool PlotArea::plotTypesCompatible(Enums::PlotType first, Enums::PlotType second)
{
    if (first == second) return true;
    if (first == Enums::PlotType::Octave && second == Enums::PlotType::General)
        return true;
    if (second == Enums::PlotType::Octave && first == Enums::PlotType::General)
        return true;
    return false;
}

void PlotArea::update()
{DD;
    if (m_plot) m_plot->update();
}

//void setAxis(QAxObject *xAxis, const QString &title)
//{DD;
//    xAxis->dynamicCall("SetHasTitle(bool)",true);
//    QAxObject *axisTitle = xAxis->querySubObject("AxisTitle()");
//    axisTitle->setProperty("Text", title);
//    xAxis->setProperty("MinorTickMark", /*xlOutside*/3);
//    xAxis->dynamicCall("SetHasMajorGridlines(bool)", true);
//    QAxObject *xGridLines = xAxis->querySubObject("MajorGridlines()");
//    QAxObject *xGridFormat = xGridLines->querySubObject("Format");
//    QAxObject *xGridFormatLine = xGridFormat->querySubObject("Line");
//    xGridFormatLine->setProperty("Visible",true);
//    xGridFormatLine->setProperty("DashStyle",7);

//    QAxObject *format = xAxis->querySubObject("Format");
//    QAxObject *formatLine = format->querySubObject("Line");
//    formatLine->setProperty("Visible",true);
//    QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
//    formatLineForeColor->setProperty("ObjectThemeColor", 13);

//    delete formatLineForeColor;
//    delete formatLine;
//    delete format;
//    delete xGridFormatLine;
//    delete xGridFormat;
//    delete xGridLines;
//    delete axisTitle;
//}

void PlotArea::exportSonogramToExcel(bool fullRange, bool dataOnly)
{
    auto curves = m_plot->model()->curves([](Curve*c){return c->isVisible();});
    if (curves.isEmpty()) return;

    Curve *curve =  curves.first();

    Channel *channel = curve->channel;
    FileDescriptor *descriptor = channel->descriptor();

    Range rangeX = m_plot->plotRange(Enums::AxisType::atBottom);
    Range rangeZ = m_plot->plotRange(Enums::AxisType::atLeft);

    int xMin = fullRange?0:qMax(channel->data()->nearest(rangeX.min), 0);
    int xMax = fullRange?channel->data()->samplesCount()-1:
                         qMin(channel->data()->nearest(rangeX.max), channel->data()->samplesCount()-1);
    int zMin = fullRange?0:qMax(channel->data()->nearestZ(rangeZ.min), 0);
    int zMax = fullRange?channel->data()->blocksCount()-1:
                         qMin(channel->data()->nearestZ(rangeZ.max), channel->data()->blocksCount()-1);
    //получаем рабочую книгу
    QXlsx::Document output;
    output.addSheet(channel->name());
    output.selectSheet(channel->name());

    // записываем название файла и описатели
    QStringList descriptions = descriptor->dataDescription().twoStringDescription();
    while (descriptions.size()<2) descriptions << "";

    output.write(1, 1, descriptor->fileName());
    output.write(2, 1, curve->title());
    output.write(3, 1, descriptions.constFirst());
    output.write(4, 1, descriptions.at(1));

    // записываем данные
    for (int col=xMin; col<=xMax; col++) {
        output.write(5, col+2-xMin, channel->data()->xValue(col));
    }
    for (int row=zMin; row<=zMax; row++) {
        output.write(row+6-zMin, 1, channel->data()->zValue(row));
        for (int col=xMin; col<=xMax; col++) {
            output.write(row+6-zMin, col+2-xMin, channel->data()->yValue(col, row));
        }
    }

    //Сохранение рисунка сонограммы
    if (!dataOnly) {
        //получаем рисунок сонограммы
        m_plot->copyToClipboard(false);
        //вставляем рисунок из буфера обмена в лист
        output.addSheet(channel->name()+"-рис.");
        output.selectSheet(channel->name()+"-рис.");

        output.insertImage(1,1, qApp->clipboard()->image());
    }

    QTemporaryFile tempFile("DeepSeaBase-XXXXXX.xlsx");
    tempFile.open();
    output.saveAs(tempFile.fileName());

    QDesktopServices::openUrl(QUrl::fromLocalFile(tempFile.fileName()));
}

void PlotArea::exportToExcel(bool fullRange, bool dataOnly)
{DD;
    if (m_plot && m_plot->type() == Enums::PlotType::Spectrogram) {
        exportSonogramToExcel(fullRange, dataOnly);
        return;
    }

    const auto size = curvesCount();

    if (size == 0) {
        QMessageBox::warning(this, "Графиков нет", "Постройте хотя бы один график!");
        return;
    }

     Channel *channel = m_plot->model()->curve(0)->channel;
     FileDescriptor *descriptor = channel->descriptor();

     // проверяем, все ли каналы из одного файла
     bool allChannelsFromOneFile = true;
     for (int i=1; i<size; ++i) {
         if (m_plot->model()->curve(i)->channel->descriptor()->fileName() != descriptor->fileName()) {
             allChannelsFromOneFile = false;
             break;
         }
     }

     //проверяем, все ли каналы имеют одинаковое разрешение по х
     bool allChannelsHaveSameXStep = true;
     for (int i=1; i<size; ++i) {
         if (!qFuzzyCompare(m_plot->model()->curve(i)->channel->data()->xStep()+1.0,
                            channel->data()->xStep()+1.0)) {
             allChannelsHaveSameXStep = false;
             break;
         }
     }

     //проверяем, все ли каналы имеют одинаковую длину
     bool allChannelsHaveSameLength = true;
     for (int i=1; i<size; ++i) {
         if (m_plot->model()->curve(i)->channel->data()->samplesCount() != channel->data()->samplesCount()) {
             allChannelsHaveSameLength = false;
             break;
         }
     }

     const int samplesCount = channel->data()->samplesCount();
     const bool writeToSeparateColumns = !allChannelsHaveSameXStep || !allChannelsHaveSameLength;

     double minX = channel->data()->xMin();
     double maxX = channel->data()->xMax();

     Range range = m_plot->plotRange(Enums::AxisType::atBottom);

     for (int i=1; i<size; ++i) {
         Channel *ch = m_plot->model()->curve(i)->channel;
         if (ch->data()->xMin() < minX) minX = ch->data()->xMin();
         if (ch->data()->xMax() > maxX) maxX = ch->data()->xMax();
     }
     if (minX >= range.min && maxX <= range.max)
         fullRange = true;

     // определяем, будут ли экспортированы графики;
     bool exportPlots = true;
     if (samplesCount > 32000 && fullRange && !dataOnly) {
         if (QMessageBox::question(this, "Слишком много данных",
                              "В одном или всех каналах число отсчетов превышает 32000.\n"
                              "Экспортировать только данные (графики не будут экспортированы)?")==QMessageBox::Yes)
         exportPlots = false;
         else return;
     }
     if (dataOnly) exportPlots = false;

     //получаем рабочую книгу
     QXlsx::Document output;
     output.addSheet("data");
     output.selectSheet("data");

     // записываем название файла и описатели
     QStringList descriptions = descriptor->dataDescription().twoStringDescription();
     while (descriptions.size()<2) descriptions << "";

     // записываем название файла и описатели
     if (allChannelsFromOneFile) {
         QStringList descriptions = descriptor->dataDescription().twoStringDescription();
         while (descriptions.size()<2) descriptions << "";

         output.write(1, 1, descriptor->fileName());
         output.write(2, 2, descriptions.constFirst());
         output.write(3, 2, descriptions.at(1));
     }
     else {
         for (int i=0; i<size; ++i) {
             Curve *curve = m_plot->model()->curve(i);
             QStringList descriptions = curve->channel->descriptor()->dataDescription().twoStringDescription();
             while (descriptions.size()<2) descriptions << "";

             output.write(1, writeToSeparateColumns? 2+i*2 : 2+i, curve->channel->descriptor()->fileName());
             output.write(2, writeToSeparateColumns? 2+i*2 : 2+i, descriptions.constFirst());
             output.write(3, writeToSeparateColumns? 2+i*2 : 2+i, descriptions.at(1));
         }
     }

     // записываем название канала
     for (int i=0; i<size; ++i) {
         Curve *curve = m_plot->model()->curve(i);
         output.write(4, writeToSeparateColumns? 2+i*2 : 2+i, curve->title());
     }

     QVector<int> selectedSamples;

     bool someStepIsZero = false;

     int coef = 1;

     // если все каналы имеют одинаковый шаг по х, то в первый столбец записываем
     // данные х
     // если каналы имеют разный шаг по х, то для каждого канала отдельно записываем
     // по два столбца
     // если шаг по х нулевой, то предполагаем октаву, размножаем данные для графика
     if (!writeToSeparateColumns) {
         const int numCols = size;
         const bool zeroStep = qFuzzyIsNull(channel->data()->xStep());
         someStepIsZero |= zeroStep;

         if (zeroStep && se->getSetting("plotOctaveAsHistogram").toInt()==1)
             coef = 2;

         int currentRow = 5;
         for (int i = 0; i < samplesCount; ++i) {
             double val = channel->data()->xValue(i);
             if (!fullRange && (val < range.min || val > range.max) ) continue;

             currentRow = i*coef+5;

             if (coef == 2) {//размножаем каждое значение на 2
                 double f1 = i==0 ? val/pow(10.0,0.05):sqrt(val*channel->data()->xValue(i-1));
                 double f2 = i==samplesCount-1?val*pow(10.0,0.05):sqrt(val*channel->data()->xValue(i+1));
                 //первый ряд: (f1, Li)
                 output.write(currentRow, 1, f1);
                 for (int j = 0; j < numCols; ++j)
                     output.write(currentRow, j+2, m_plot->model()->curve(j)->channel->data()->yValue(i));
                 //второй ряд: (f2, Li)
                 output.write(currentRow+1, 1, f2);
                 for (int j = 0; j < numCols; ++j)
                     output.write(currentRow+1, j+2, m_plot->model()->curve(j)->channel->data()->yValue(i));
             }
             else {
                 output.write(currentRow, 1, val);
                 for (int j = 0; j < numCols; ++j)
                     output.write(currentRow, j+2, m_plot->model()->curve(j)->channel->data()->yValue(i));
             }
         }
     }
     else {
         for (int i=0; i<size; ++i) {
             Curve *curve = m_plot->model()->curve(i);
             Channel *ch = curve->channel;
             bool zeroStep = qFuzzyIsNull(ch->data()->xStep());
             someStepIsZero |= zeroStep;

             coef = 1;
             if (zeroStep && se->getSetting("plotOctaveAsHistogram").toInt()==1)
                 coef = 2;

             int currentCol = i*2+1;
             int currentRow = 5;

             for (int j = 0; j < ch->data()->samplesCount(); j++) {
                 double val = ch->data()->xValue(j);
                 if (!fullRange && (val < range.min || val > range.max) ) continue;

                 if (coef == 2) {//размножаем каждое значение на 2
                     double f1 = j==0 ? val/pow(10.0,0.05):sqrt(val*ch->data()->xValue(j-1));
                     double f2 = j==ch->data()->samplesCount()-1?val*pow(10.0,0.05):sqrt(val*ch->data()->xValue(j+1));
                     //первый ряд: (f1, Li)
                     output.write(currentRow,   currentCol,   f1);
                     output.write(currentRow,   currentCol+1, ch->data()->yValue(j));
                     output.write(currentRow+1, currentCol,   f2);
                     output.write(currentRow+1, currentCol+1, ch->data()->yValue(j));
                 }
                 else {
                     output.write(currentRow,   currentCol,   val);
                     output.write(currentRow,   currentCol+1, ch->data()->yValue(j));
                 }
                 currentRow += coef;
             }
         }
     }

     if (exportPlots) {
         output.addSheet("plot", QXlsx::AbstractSheet::ST_ChartSheet);
         auto sheet = static_cast<QXlsx::Chartsheet*>(output.currentSheet());
         auto chart = sheet->chart();
         chart->setChartType(QXlsx::Chart::CT_ScatterChart);

         for (int i = 0; i < size; ++i) {
             Curve *curve = m_plot->model()->curve(i);
             Channel *ch = curve->channel;
             bool zeroStep = qFuzzyIsNull(ch->data()->xStep());

             int coef = 1;
             if (zeroStep && se->getSetting("plotOctaveAsHistogram").toInt()==1)
                 coef = 2;
             int xCol = writeToSeparateColumns ? i*2+1 : 1;
             int yCol = writeToSeparateColumns ? i*2+2 : i+2;
             chart->addSeries(QXlsx::CellRange(4, xCol, 4+ch->data()->samplesCount()*coef, xCol),
                              QXlsx::CellRange(4, yCol, 4+ch->data()->samplesCount()*coef, yCol),
                              output.sheet("data"),
                              true);
             QXlsx::LineFormat lf;
             lf.setWidth(m_plot->model()->curve(i)->pen().widthF());
             lf.setType(QXlsx::FillProperties::FillType::SolidFill);

             Qt::PenStyle lineStyle = m_plot->model()->curve(i)->pen().style();
             switch (lineStyle) {
                 case Qt::NoPen:       lf.setType(QXlsx::FillProperties::FillType::NoFill); break;
                 case Qt::SolidLine: break;
                 case Qt::DashLine:    lf.setStrokeType(QXlsx::LineFormat::StrokeType::Dash); break;
                 case Qt::DotLine:     lf.setStrokeType(QXlsx::LineFormat::StrokeType::Dot); break;
                 case Qt::DashDotLine: lf.setStrokeType(QXlsx::LineFormat::StrokeType::DashDot); break;
                 case Qt::DashDotDotLine: lf.setStrokeType(QXlsx::LineFormat::StrokeType::LongDashDotDot); break;
                 default: break;
             }

             QColor color = m_plot->model()->curve(i)->pen().color();
             lf.setColor(color);
             chart->setSeriesLineFormat(i, lf);

             QXlsx::MarkerFormat mf;
             switch (curve->markerShape()) {
                 case Curve::MarkerShape::NoMarker: mf.setType(QXlsx::MarkerFormat::MarkerType::None); break;
                 case Curve::MarkerShape::Square: mf.setType(QXlsx::MarkerFormat::MarkerType::Square); break;
                 case Curve::MarkerShape::Diamond: mf.setType(QXlsx::MarkerFormat::MarkerType::Diamond); break;
                 case Curve::MarkerShape::Triangle:
                 case Curve::MarkerShape::TriangleInverted:
                     mf.setType(QXlsx::MarkerFormat::MarkerType::Triangle); break;
                 case Curve::MarkerShape::Circle:
                 case Curve::MarkerShape::Disc:
                     mf.setType(QXlsx::MarkerFormat::MarkerType::Circle); break;
                 case Curve::MarkerShape::Plus: mf.setType(QXlsx::MarkerFormat::MarkerType::Plus); break;
                 case Curve::MarkerShape::Cross: mf.setType(QXlsx::MarkerFormat::MarkerType::Cross); break;
                 case Curve::MarkerShape::Star: mf.setType(QXlsx::MarkerFormat::MarkerType::Star); break;
                 case Curve::MarkerShape::Dot: mf.setType(QXlsx::MarkerFormat::MarkerType::Dot); break;
                 default: mf.setType(QXlsx::MarkerFormat::MarkerType::Diamond); break;
             }
             mf.setSize(curve->markerSize());
             chart->setSeriesMarkerFormat(i, mf);
         }

         // перемещаем графики на дополнительную вертикальную ось,
         // если они были там в программе
         // и меняем название кривой
         auto b = chart->addAxis(QXlsx::Axis::Type::Val, QXlsx::Axis::Position::Bottom);
         b->setTitle(stripHtml(m_plot->axisTitle(Enums::AxisType::atBottom)));
         if (someStepIsZero)
             b->scaling()->logBase = 10; // TODO: replace with a method
         b->setRange(int(range.min/10)*10, range.max);
         b->setMajorGridLines(QColor(Qt::black), 0.5, QXlsx::LineFormat::StrokeType::Dash);
         auto l = chart->addAxis(QXlsx::Axis::Type::Val, QXlsx::Axis::Position::Left);
         l->setTitle(stripHtml(m_plot->axisTitle(Enums::AxisType::atLeft)));
         l->setCrossAxis(b);
         l->setMajorGridLines(QColor(Qt::black), 0.5, QXlsx::LineFormat::StrokeType::Dash);
         b->setCrossesAt(QXlsx::Axis::CrossesType::Minimum);

         b->setMajorTickMark(QXlsx::Axis::TickMark::Out);
         b->setMinorTickMark(QXlsx::Axis::TickMark::Out);
         l->setMajorTickMark(QXlsx::Axis::TickMark::Out);
         l->setMinorTickMark(QXlsx::Axis::TickMark::Out);

         for ( int i=0; i<size; ++i) {
             Curve *curve = m_plot->model()->curve(i);
             QXlsx::Axis* r = nullptr;
             QXlsx::Axis* br = nullptr;
             if (curve->yAxis()==Enums::AxisType::atRight) {
                 if (!r) {
                     br = chart->addAxis(QXlsx::Axis::Type::Val, QXlsx::Axis::Position::Bottom);
                     r = chart->addAxis(QXlsx::Axis::Type::Val, QXlsx::Axis::Position::Right);
                     r->setTitle(stripHtml(m_plot->axisTitle(Enums::AxisType::atRight)));
                     r->setCrossAxis(br);
                     r->setMajorTickMark(QXlsx::Axis::TickMark::Out);
                     r->setMinorTickMark(QXlsx::Axis::TickMark::Out);
                 }

                 chart->setSeriesAxes(i, {br->id(), r->id()});
             }
             else {
                 chart->setSeriesAxes(i, {b->id(), l->id()});
             }

             QVector<int> labels;
             for (PointLabel *label: curve->labels)
                 labels << label->point().x+1;

             chart->setSeriesLabels(i, labels, QXlsx::Label::ShowCategory, QXlsx::Label::Position::Top);

         }

         // рамка вокруг графика
         QXlsx::LineFormat lf;
         lf.setType(QXlsx::FillProperties::FillType::NoFill);
         chart->setChartLineFormat(lf);

         // рамка вокруг полотна
         //QXlsx::LineFormat lf1;
         lf.setType(QXlsx::FillProperties::FillType::SolidFill);
         lf.setColor(QColor(Qt::black));
         lf.setWidth(0.5);
         chart->setPlotAreaLineFormat(lf);

         //легенда
         chart->setChartLegend(QXlsx::Chart::Right, false);

#if 0
         // цвета графиков
         for (int i = 0; i< size; ++i) {
             Curve *curve = m_plot->model()->curve(i);
             QAxObject * serie = series->querySubObject("Item(int)", i+1);
             if (serie) {

                 foreach(PointLabel *label, curve->labels) {
                     QAxObject* point = serie->querySubObject("Points(QVariant)", label->point().x+1);
                     QVariantList options = {0, 0, 0, 0, 0, -1, 0, 0, 0, 0};

                     point->dynamicCall("ApplyDataLabels()", options);
                     QAxObject* dataLabel = point->querySubObject("DataLabel");
                     dataLabel->setProperty("ShowCategoryName", -1);
                     dataLabel->setProperty("ShowValue", 0);
                     dataLabel->setProperty("Position", 0);
                     delete dataLabel;
                     delete point;
                 }

             }
             delete serie;
         }

         QAxObject *legendObject = chart->querySubObject("Legend");
         QAxObject *legendFormat = legendObject->querySubObject("Format");
         QAxObject *legendFormatFill = legendFormat->querySubObject("Fill");

         legendFormatFill->setProperty("Visible", 1);
         legendFormatFill->querySubObject("ForeColor")->setProperty("RGB", QColor(Qt::white).rgb());
         legendFormat->querySubObject("Line")->setProperty("Visible", 1);
         legendFormat->querySubObject("Line")->querySubObject("ForeColor")->setProperty("RGB", QColor(Qt::black).rgb());

         delete legendFormatFill;
         delete legendFormat;
         delete legendObject;
         delete series;
         delete chart;
         delete charts;
#endif
     }

     QTemporaryFile tempFile(QDir::tempPath()+"/DeepSeaBase-XXXXXX.xlsx");
//     tempFile.setAutoRemove(false);

     if (tempFile.open()) {
         output.saveAs(tempFile.fileName());

         QDesktopServices::openUrl(QUrl::fromLocalFile(tempFile.fileName()));
     }
     else QMessageBox::warning(this, "Экспорт в Excel", "Не удалось создать временный файл");
}

//void PlotArea::exportToExcel(bool fullRange, bool dataOnly)
//{DD;
//    if (m_plot && m_plot->type() == Enums::PlotType::Spectrogram) {
//        exportSonogramToExcel(fullRange, dataOnly);
//        return;
//    }

//    static QAxObject *excel = 0;

//   // QList<Curve*> &curves = m_plot->curves;

//    const auto size = curvesCount();

//    if (size == 0) {
//        QMessageBox::warning(this, "Графиков нет", "Постройте хотя бы один график!");
//        return;
//    }

//     Channel *channel = m_plot->model()->curve(0)->channel;
//     FileDescriptor *descriptor = channel->descriptor();

//     // проверяем, все ли каналы из одного файла
//     bool allChannelsFromOneFile = true;
//     for (int i=1; i<size; ++i) {
//         if (m_plot->model()->curve(i)->channel->descriptor()->fileName() != descriptor->fileName()) {
//             allChannelsFromOneFile = false;
//             break;
//         }
//     }

//     //проверяем, все ли каналы имеют одинаковое разрешение по х
//     bool allChannelsHaveSameXStep = true;
//     for (int i=1; i<size; ++i) {
//         if (!qFuzzyCompare(m_plot->model()->curve(i)->channel->data()->xStep()+1.0,
//                            channel->data()->xStep()+1.0)) {
//             allChannelsHaveSameXStep = false;
//             break;
//         }
//     }

//     //проверяем, все ли каналы имеют одинаковую длину
//     bool allChannelsHaveSameLength = true;
//     for (int i=1; i<size; ++i) {
//         if (m_plot->model()->curve(i)->channel->data()->samplesCount() != channel->data()->samplesCount()) {
//             allChannelsHaveSameLength = false;
//             break;
//         }
//     }

//     const int samplesCount = channel->data()->samplesCount();
//     const bool writeToSeparateColumns = !allChannelsHaveSameXStep || !allChannelsHaveSameLength;

//     double minX = channel->data()->xMin();
//     double maxX = channel->data()->xMax();

//     Range range = m_plot->plotRange(Enums::AxisType::atBottom);

//     for (int i=1; i<size; ++i) {
//         Channel *ch = m_plot->model()->curve(i)->channel;
//         if (ch->data()->xMin() < minX) minX = ch->data()->xMin();
//         if (ch->data()->xMax() > maxX) maxX = ch->data()->xMax();
//     }
//     if (minX >= range.min && maxX <= range.max)
//         fullRange = true;

//     // определяем, будут ли экспортированы графики;
//     bool exportPlots = true;
//     if (samplesCount > 32000 && fullRange && !dataOnly) {
//         if (QMessageBox::question(this, "Слишком много данных",
//                              "В одном или всех каналах число отсчетов превышает 32000.\n"
//                              "Экспортировать только данные (графики не будут экспортированы)?")==QMessageBox::Yes)
//         exportPlots = false;
//         else return;
//     }
//     if (dataOnly) exportPlots = false;

//     if (!excel) {
//         //excel = new QAxObject("Excel.Application",this);
//         excel = new QAxObject("{00024500-0000-0000-c000-000000000046}&",this);
//     }
//     if (!excel) return;
//     //LOG(DEBUG)<<excel->generateDocumentation();

//     excel->setProperty("Visible", true);

//     //получаем рабочую книгу
//     QAxObject * workbooks = excel->querySubObject("WorkBooks");
//     QAxObject * workbook = excel->querySubObject("ActiveWorkBook");
//     if (!workbook) {
//         workbooks->dynamicCall("Add");
//     }
//     workbook = excel->querySubObject("ActiveWorkBook");

//     // получаем список листов и добавляем новый лист
//     QAxObject *worksheets = workbook->querySubObject("Sheets");
//     worksheets->dynamicCall("Add()");
//     QAxObject * worksheet = workbook->querySubObject("ActiveSheet");

//     // записываем название файла и описатели
//     if (allChannelsFromOneFile) {
//         QStringList descriptions = descriptor->dataDescription().twoStringDescription();
//         while (descriptions.size()<2) descriptions << "";

//         QAxObject *cells = worksheet->querySubObject("Cells(Int,Int)", 1, 1);
//         if (cells) cells->setProperty("Value", descriptor->fileName());

//         cells = worksheet->querySubObject("Cells(Int,Int)", 2, 2);
//         if (cells) cells->setProperty("Value", descriptions.constFirst());

//         cells = worksheet->querySubObject("Cells(Int,Int)", 3, 2);
//         if (cells) cells->setProperty("Value", descriptions.at(1));

//         delete cells;
//     }
//     else {
//         for (int i=0; i<size; ++i) {
//             Curve *curve = m_plot->model()->curve(i);
//             QStringList descriptions = curve->channel->descriptor()->dataDescription().twoStringDescription();
//             while (descriptions.size()<2) descriptions << "";

//             QAxObject *cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 1, 2+i)
//                                                       : worksheet->querySubObject("Cells(Int,Int)", 1, 2+i*2);
//             cells->setProperty("Value", curve->channel->descriptor()->fileName());

//             cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 2, 2+i)
//                                            : worksheet->querySubObject("Cells(Int,Int)", 2, 2+i*2);
//             if (cells) cells->setProperty("Value", descriptions.constFirst());

//             cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 3, 2+i)
//                                            : worksheet->querySubObject("Cells(Int,Int)", 3, 2+i*2);
//             if (cells) cells->setProperty("Value", descriptions.at(1));

//             delete cells;
//         }
//     }

//     // записываем название канала
//     for (int i=0; i<size; ++i) {
//         Curve *curve = m_plot->model()->curve(i);
//         QAxObject *cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 4, 2+i)
//                                                   : worksheet->querySubObject("Cells(Int,Int)", 4, 2+i*2);
//         cells->setProperty("Value", curve->title());
//         delete cells;
//     }

//     QVector<int> selectedSamples;

//     bool someStepIsZero = false;

//     // если все каналы имеют одинаковый шаг по х, то в первый столбец записываем
//     // данные х
//     // если каналы имеют разный шаг по х, то для каждого канала отдельно записываем
//     // по два столбца
//     // если шаг по х нулевой, то предполагаем октаву, размножаем данные для графика
//     if (!writeToSeparateColumns) {
//         const int numCols = size;
//         const bool zeroStep = qFuzzyIsNull(channel->data()->xStep());
//         someStepIsZero |= zeroStep;

//         QList<QVariant> cellsList;
//         QList<QVariant> rowsList;
//         for (int i = 0; i < samplesCount; ++i) {
//             double val = channel->data()->xValue(i);
//             if (!fullRange && (val < range.min || val > range.max) ) continue;

//             cellsList.clear();

//             if (zeroStep && se->getSetting("plotOctaveAsHistogram").toInt()==1) {//размножаем каждое значение на 2
//                 double f1 = i==0 ? val/pow(10.0,0.05):sqrt(val*channel->data()->xValue(i-1));
//                 double f2 = i==samplesCount-1?val*pow(10.0,0.05):sqrt(val*channel->data()->xValue(i+1));
//                 //первый ряд: (f1, Li)
//                 cellsList << f1;
//                 for (int j = 0; j < numCols; ++j) {
//                     cellsList << m_plot->model()->curve(j)->channel->data()->yValue(i);
//                 }
//                 rowsList << QVariant(cellsList);
//                 cellsList.clear();
//                 //второй ряд: (f2, Li)
//                 cellsList << f2;
//                 for (int j = 0; j < numCols; ++j) {
//                     cellsList << m_plot->model()->curve(j)->channel->data()->yValue(i);
//                 }
//                 rowsList << QVariant(cellsList);
//                 cellsList.clear();
//             }
//             else {
//                 cellsList << val;
//                 for (int j = 0; j < numCols; ++j) {
//                     cellsList << m_plot->model()->curve(j)->channel->data()->yValue(i);
//                 }
//                 rowsList << QVariant(cellsList);
//             }
//         }
//         int numRows = rowsList.size();

//         QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1);
//         QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + numRows - 1, 1 + numCols);
//         QAxObject* range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );

//         range->setProperty("Value", QVariant(rowsList) );

//         // выделяем диапазон, чтобы он автоматически использовался для построения диаграммы
//         Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 4, 1);
//         Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 4 + numRows, 1 + numCols);
//         range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );
//         range->dynamicCall("Select (void)");

//         delete range;
//         delete Cell1;
//         delete Cell2;
//     }
//     else {
//         for (int i=0; i<size; ++i) {
//             Curve *curve = m_plot->model()->curve(i);
//             Channel *ch = curve->channel;
//             bool zeroStep = qFuzzyIsNull(ch->data()->xStep());
//             someStepIsZero |= zeroStep;

//             QList<QVariant> cellsList;
//             QList<QVariant> rowsList;
//             for (int j = 0; j < ch->data()->samplesCount(); j++) {
//                 double val = ch->data()->xValue(j);
//                 if (!fullRange && (val < range.min || val > range.max) ) continue;

//                 cellsList.clear();
//                 if (zeroStep && se->getSetting("plotOctaveAsHistogram").toInt()==1) {//размножаем каждое значение на 2
//                     double f1 = j==0 ? val/pow(10.0,0.05):sqrt(val*ch->data()->xValue(j-1));
//                     double f2 = j==ch->data()->samplesCount()-1?val*pow(10.0,0.05):sqrt(val*ch->data()->xValue(j+1));
//                     //первый ряд: (f1, Li)
//                     cellsList << f1 << ch->data()->yValue(j);
//                     rowsList << QVariant(cellsList);
//                     cellsList.clear();
//                     //второй ряд: (f2, Li)
//                     cellsList << f2 << ch->data()->yValue(j);
//                     rowsList << QVariant(cellsList);
//                     cellsList.clear();
//                 }
//                 else {
//                     cellsList << val << ch->data()->yValue(j);
//                     rowsList << QVariant(cellsList);
//                 }
//             }
//             int numRows = rowsList.size();
//             selectedSamples << numRows;

//             QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1+i*2);
//             QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + numRows-1, 2 + i*2);
//             QAxObject* range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );

//             range->setProperty("Value", QVariant(rowsList) );

//             delete Cell1;
//             delete Cell2;
//             delete range;
//         }
//     }

//     if (exportPlots) {
//         QAxObject *charts = workbook->querySubObject("Charts");
//         charts->dynamicCall("Add()");
//         QAxObject *chart = workbook->querySubObject("ActiveChart");
//         chart->setProperty("ChartType", 75);
//         QAxObject * series = chart->querySubObject("SeriesCollection");

//         // отдельно строить кривые нужно, только если у нас много пар столбцов с данными
//         if (writeToSeparateColumns) {
//             int seriesCount = series->property("Count").toInt();

//             // удаляем предыдущие графики, созданные по умолчанию
//             bool ok=true;
//             while ((seriesCount>0) && ok) {
//                 QAxObject * serie = series->querySubObject("Item (int)", 1);
//                 if (serie) {
//                     serie->dynamicCall("Delete()");
//                     delete serie;
//                     seriesCount--;
//                 }
//                 else ok=false;
//             }

//             for (int i=0; i<size; ++i) {
//                 QAxObject * serie = series->querySubObject("NewSeries()");
//                 if (serie) {
//                     //xvalues
//                     QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1+i*2);
//                     QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + selectedSamples.at(i)-1, 1 + i*2);
//                     if (Cell1 && Cell2) {
//                         QAxObject * xvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
//                         if (xvalues)
//                             serie->setProperty("XValues", xvalues->asVariant());
//                         delete xvalues;
//                     }
//                     delete Cell1;
//                     delete Cell2;

//                     //yvalues
//                     Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 2+i*2);
//                     Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + selectedSamples.at(i)-1, 2 + i*2);
//                     if (Cell1 && Cell2) {
//                         QAxObject * yvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
//                         if (yvalues)
//                             serie->setProperty("Values", yvalues->asVariant());
//                         delete yvalues;
//                     }
//                     delete Cell1;
//                     delete Cell2;
//                 }
//                 delete serie;
//             }
//         }

//         // перемещаем графики на дополнительную вертикальную ось,
//         // если они были там в программе
//         // и меняем название кривой
//         int seriesCount = series->property("Count").toInt();
//         bool addRightAxis = false;
//         for ( int i=0; i<seriesCount; ++i) {
//             Curve *curve = m_plot->model()->curve(i);
//             QAxObject * serie = series->querySubObject("Item (int)", i+1);
//             if (serie) {
//                 if (curve->yAxis()==Enums::AxisType::atRight) {
//                     serie->setProperty("AxisGroup", 2);
//                     addRightAxis = true;
//                 }
//                 serie->setProperty("Name", curve->channel->name());
//             }
//             delete serie;
//         }
//         if (addRightAxis) {
//             QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&,const QVariant&)", 2,2);
//             if (yAxis) setAxis(yAxis, stripHtml(m_plot->axisTitleText(Enums::AxisType::atRight)));
//             delete yAxis;
//         }


//         // добавляем подписи осей
//         QAxObject *xAxis = chart->querySubObject("Axes(const QVariant&)", 1);
//         if (xAxis) {
//             setAxis(xAxis, stripHtml(m_plot->axisTitleText(Enums::AxisType::atBottom)));
//             if (someStepIsZero) xAxis->setProperty("ScaleType", -4133); //logarithmic
//             xAxis->setProperty("MaximumScale", range.max);
//             xAxis->setProperty("MinimumScale", int(range.min/10)*10);
//         }
//         delete xAxis;

//         QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&)", 2);
//         if (yAxis) {
//             setAxis(yAxis, stripHtml(m_plot->axisTitleText(Enums::AxisType::atLeft)));
//             yAxis->setProperty("CrossesAt", -1000);
//         }
//         delete yAxis;

//         // рамка вокруг графика
//         QAxObject *plotArea = chart->querySubObject("PlotArea");
//         if (plotArea) setLineColor(plotArea, 13);
//         delete plotArea;

//         // цвета графиков
//         for (int i = 0; i< size; ++i) {
//             Curve *curve = m_plot->model()->curve(i);
//             QAxObject * serie = series->querySubObject("Item(int)", i+1);
//             if (serie) {
//                 QAxObject *format = serie->querySubObject("Format");
//                 QAxObject *formatLine = format->querySubObject("Line");
//                 if (formatLine) {
//                     formatLine->setProperty("Weight", m_plot->model()->curve(i)->pen().width());
//                     Qt::PenStyle lineStyle = m_plot->model()->curve(i)->pen().style();
//                     int msoLineStyle = 1;
//                     switch (lineStyle) {
//                         case Qt::NoPen:
//                         case Qt::SolidLine: msoLineStyle = 1; break;
//                         case Qt::DashLine: msoLineStyle = 4; break;
//                         case Qt::DotLine: msoLineStyle = 3; break;
//                         case Qt::DashDotLine: msoLineStyle = 5; break;
//                         case Qt::DashDotDotLine: msoLineStyle = 6; break;
//                         default: break;
//                     }
//                     formatLine->setProperty("DashStyle", msoLineStyle);

//                     QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
//                     QColor color = m_plot->model()->curve(i)->pen().color();
//                     //меняем местами красный и синий, потому что Excel неправильно понимает порядок
//                     int red = color.red();
//                     color.setRed(color.blue());
//                     color.setBlue(red);
//                     if (formatLineForeColor) formatLineForeColor->setProperty("RGB", color.rgb());
//                     delete formatLineForeColor;
//                 }

//                 foreach(PointLabel *label, curve->labels) {
//                     QAxObject* point = serie->querySubObject("Points(QVariant)", label->point().x+1);
//                     QVariantList options = {0, 0, 0, 0, 0, -1, 0, 0, 0, 0};

//                     point->dynamicCall("ApplyDataLabels()", options);
//                     QAxObject* dataLabel = point->querySubObject("DataLabel");
//                     dataLabel->setProperty("ShowCategoryName", -1);
//                     dataLabel->setProperty("ShowValue", 0);
//                     dataLabel->setProperty("Position", 0);
//                     delete dataLabel;
//                     delete point;
//                 }

//                 delete formatLine;
//                 delete format;
//             }
//             delete serie;
//         }

//         QAxObject *chartArea = chart->querySubObject("ChartArea");
//         chartArea->querySubObject("Format")->querySubObject("Line")->setProperty("Visible", 0);
//         delete chartArea;

//         QAxObject *legendObject = chart->querySubObject("Legend");
//         QAxObject *legendFormat = legendObject->querySubObject("Format");
//         QAxObject *legendFormatFill = legendFormat->querySubObject("Fill");

//         legendFormatFill->setProperty("Visible", 1);
//         legendFormatFill->querySubObject("ForeColor")->setProperty("RGB", QColor(Qt::white).rgb());
//         legendFormat->querySubObject("Line")->setProperty("Visible", 1);
//         legendFormat->querySubObject("Line")->querySubObject("ForeColor")->setProperty("RGB", QColor(Qt::black).rgb());

//         delete legendFormatFill;
//         delete legendFormat;
//         delete legendObject;
//         delete series;
//         delete chart;
//         delete charts;
//     }

////    QFile file1("chartArea.html");
////    file1.open(QIODevice::WriteOnly | QIODevice::Text);
////    QTextStream out(&file1);
////    out << chartArea->generateDocumentation();
////    file1.close();


//     delete worksheet;
//     delete worksheets;
//     delete workbook;
//     delete workbooks;
//}

void PlotArea::updateActions(int filesCount, int channelsCount)
{DD;
    const bool hasCurves = m_plot ? curvesCount()>0 : false;
    const bool allCurvesFromSameDescriptor = m_plot ? m_plot->model()->allCurvesFromSameDescriptor() : false;

    clearPlotAct->setEnabled(hasCurves);
    savePlotAct->setEnabled(hasCurves);
    copyToClipboardAct->setEnabled(hasCurves);
    printPlotAct->setEnabled(hasCurves);
    //if (playAct) playAct->setEnabled(hasCurves);
    if (m_plot) m_plot->updateActions(filesCount, channelsCount);
    previousDescriptorAct->setEnabled(filesCount>1 && allCurvesFromSameDescriptor);
    nextDescriptorAct->setEnabled(filesCount>1 && allCurvesFromSameDescriptor);
    arbitraryDescriptorAct->setEnabled(filesCount>1 && allCurvesFromSameDescriptor);
    cycleChannelsUpAct->setEnabled(channelsCount>1 && hasCurves);
    cycleChannelsDownAct->setEnabled(channelsCount>1 && hasCurves);
}

void PlotArea::deleteCurvesForDescriptor(FileDescriptor *f, const QVector<int> &indexes)
{DD;
    if (m_plot) m_plot->deleteCurvesForDescriptor(f, indexes);
}

void PlotArea::deleteAllCurves()
{
    resetCycling();
    if (m_plot) m_plot->deleteAllCurves(true);
}

//вызывается при переходе на предыдущую/следующую запись
void PlotArea::replotDescriptor(FileDescriptor *f, int fileIndex)
{DD;
    if (m_plot)
        m_plot->plotCurvesForDescriptor(f, fileIndex);
}

void PlotArea::addCorrection(const QList<FileDescriptor *> &additionalFiles)
{
    if (m_plot && m_plot->curvesCount() > 0) {
        CorrectionDialog correctionDialog(m_plot, this);

        //задаем файлы, которые дополнительно должны быть скорректированы
        //помимо построенных на графике
        correctionDialog.setFiles(additionalFiles);
        correctionDialog.exec();
    }
}

QVector<Channel *> PlotArea::plottedChannels() const
{DD;
    if (m_plot) return m_plot->model()->plottedChannels();
    return QVector<Channel *>();
}

Channel *PlotArea::firstVisible() const
{
    if (!m_plot) return nullptr;
    if (Curve *c = m_plot->model()->firstOf([](Curve*c){return c->isVisible();}))
        return c->channel;
    return nullptr;
}

QMap<FileDescriptor *, QVector<int> > PlotArea::plottedDescriptors() const
{DD;
    QMap<FileDescriptor *, QVector<int> > result;
    if (m_plot) {
        auto list = m_plot->model()->plottedDescriptors();
        for (auto &d : list) result.insert(d, m_plot->model()->plottedIndexesForDescriptor(d));
    }
    return result;
}

int PlotArea::curvesCount(int type) const
{DD;
    return m_plot?m_plot->curvesCount(type):0;
}

void PlotArea::updateLegends()
{DD;
    if (m_plot) m_plot->updateLegends();
}


void PlotArea::dragEnterEvent(QDragEnterEvent *event)
{DD;
    setFocus();

    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData && !m_plot) {
        event->acceptProposedAction();
    }
}

void PlotArea::dropEvent(QDropEvent *event)
{DD;
    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        auto type = getPlotType(myData->channels);
        addPlot(type);
        if (m_plot) m_plot->onDropEvent(true /*plot on left*/, myData->channels);
        event->acceptProposedAction();
    }
}

void PlotArea::resetCycling()
{DD;
    arbitraryDescriptorAct->setChecked(false);
    if (m_plot) m_plot->sergeiMode = false;
}
