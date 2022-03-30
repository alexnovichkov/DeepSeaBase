#include "plotarea.h"

#include <QtWidgets>
#include "plot/plot.h"
#include "spectrogram.h"
#include "octaveplot.h"
#include "timeplot.h"
#include "plot/curve.h"
#include "app.h"
#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "plot/pointlabel.h"
#include "plotmodel.h"
#include "channelsmimedata.h"
#include <QAxObject>

PlotArea::PlotArea(int index, QWidget *parent)
    : ads::CDockWidget(QString("График %1").arg(index), parent)
{
    setObjectName(QString("График %1").arg(index));
    setAcceptDrops(true);
    setAsCurrentTab();
    setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);
    setFeature(ads::CDockWidget::CustomCloseHandling, true);
    setFeature(ads::CDockWidget::DockWidgetFocusable, true);

    autoscaleXAct = new QAction("Автомасштабирование по оси X", this);
    autoscaleXAct->setIcon(QIcon(":/icons/autoscale-x.png"));
    autoscaleXAct->setCheckable(true);
    bool autoscale = App->getSetting("autoscale-x", true).toBool();
    connect(autoscaleXAct, &QAction::toggled, [this](bool toggled){
        if (m_plot) m_plot->toggleAutoscale(0 /* x axis */,toggled);
        App->setSetting("autoscale-x", toggled);
    });
    autoscaleXAct->setChecked(autoscale);
    if (m_plot) m_plot->toggleAutoscale(0 /* x axis */, autoscale);

    autoscaleYAct = new QAction("Автомасштабирование по оси Y", this);
    autoscaleYAct->setIcon(QIcon(":/icons/autoscale-y-main.png"));
    autoscaleYAct->setCheckable(true);
    autoscale = App->getSetting("autoscale-y", true).toBool();
    connect(autoscaleYAct, &QAction::toggled, [this](bool toggled){
        if (m_plot) m_plot->toggleAutoscale(1 /* y axis */,toggled);
        App->setSetting("autoscale-y", toggled);
    });
    autoscaleYAct->setChecked(autoscale);
    if (m_plot) m_plot->toggleAutoscale(1 /* x axis */, autoscale);


    autoscaleYSlaveAct = new QAction("Автомасштабирование по правой оси Y", this);
    autoscaleYSlaveAct->setIcon(QIcon(":/icons/autoscale-y-slave.png"));
    autoscaleYSlaveAct->setCheckable(true);
    autoscale = App->getSetting("autoscale-y-slave", true).toBool();
    connect(autoscaleYSlaveAct, &QAction::toggled, [this](bool toggled){
        if (m_plot) m_plot->toggleAutoscale(2 /* y slave axis */,toggled);
        App->setSetting("autoscale-y-slave", toggled);
    });
    autoscaleYSlaveAct->setChecked(autoscale);
    if (m_plot) m_plot->toggleAutoscale(2 /* x axis */, autoscale);

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
    previousDescriptorAct->setShortcut(Qt::CTRL + Qt::Key_Up);
    previousDescriptorAct->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    previousDescriptorAct->setToolTip(QString("%1 <font color=gray size=-1>%2</font>")
                          .arg("Предыдущая запись")
                          .arg(previousDescriptorAct->shortcut().toString()));
    connect(previousDescriptorAct, &QAction::triggered, [this](bool checked){emit descriptorRequested(-1, checked);});

    nextDescriptorAct = new QAction("Следущая запись", this);
    nextDescriptorAct->setIcon(QIcon(":/icons/rplus.png"));
    nextDescriptorAct->setShortcut(Qt::CTRL + Qt::Key_Down);
    nextDescriptorAct->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    nextDescriptorAct->setToolTip(QString("%1 <font color=gray size=-1>%2</font>")
                          .arg("Следущая запись")
                          .arg(nextDescriptorAct->shortcut().toString()));
    connect(nextDescriptorAct, &QAction::triggered, [this](bool checked){emit descriptorRequested(1, checked);});

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
        if (m_plot) m_plot->deleteAllCurves();
    });

    savePlotAct = new QAction(QString("Сохранить график..."), this);
    savePlotAct->setIcon(QIcon(":/icons/picture.ico"));
    connect(savePlotAct, &QAction::triggered, [=](){
        if (m_plot) m_plot->savePlot();
    });

    switchCursorAct = new QAction(QString("Показать/скрыть курсор"), this);
    switchCursorAct->setIcon(QIcon(":/icons/cursor.png"));
    switchCursorAct->setCheckable(true);
    switchCursorAct->setObjectName("simpleCursor");
    bool pickerEnabled = App->getSetting("pickerEnabled", true).toBool();
    switchCursorAct->setChecked(pickerEnabled);
    connect(switchCursorAct, &QAction::triggered, [=](){
        if (m_plot) m_plot->switchCursor();
    });

    trackingCursorAct = new QAction(QString("Включить курсор дискрет"), this);
    trackingCursorAct->setIcon(QIcon(":/icons/tracking.png"));
    trackingCursorAct->setCheckable(true);
    trackingCursorAct->setObjectName("trackingCursor");
    connect(trackingCursorAct, &QAction::triggered, [=](){
        if (m_plot) m_plot->switchTrackingCursor();
    });

    copyToClipboardAct = new QAction(QString("Копировать в буфер обмена"), this);
    copyToClipboardAct->setIcon(QIcon(":/icons/clipboard.png"));
    connect(copyToClipboardAct, &QAction::triggered, [=](){
        if (m_plot) m_plot->copyToClipboard();
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
    scaleToolBar->addAction(switchCursorAct);
    scaleToolBar->addAction(interactionModeAct);
    scaleToolBar->addAction(trackingCursorAct);

    plotsLayout = new QGridLayout;
    infoLabel = new QLabel("- Перетащите сюда каналы, чтобы построить их графики\n"
                                "- Если зажать Ctrl, то будут построены графики для всех\n"
                                "  выделенных файлов", this);
    QFont font;
    //font.setBold(true);
    font.setPointSize(12);
    infoLabel->setFont(font);
    //infoLabel->setF(QColor(150,150,150,150));
    //infoLabel->setBackgroundRole(QColor(255,255,255,150));
    plotsLayout->addWidget(infoLabel,0,0, Qt::AlignCenter);
    auto w = new QWidget;
    //w->setBackgroundRole(QPalette::Light);
    w->setLayout(plotsLayout);
    setWidget(w);
}

Plot *PlotArea::plot()
{
    return m_plot;
}

void PlotArea::addPlot(Plot::PlotType type)
{
    if (m_plot) {
        plotsLayout->removeWidget(m_plot);
        delete m_plot;
    }
    else {
        infoLabel->hide();
    }
    switch (type) {
        case Plot::PlotType::Spectrogram: {
            m_plot = new Spectrogram(this);
            setIcon(QIcon(":/icons/spectrocurve.png"));
            break;
        }
        case Plot::PlotType::Octave: {
            m_plot = new OctavePlot(this);
            setIcon(QIcon(":/icons/barcurve.png"));
            break;
        }
        case Plot::PlotType::Time: {
            m_plot = new TimePlot(this);
            setIcon(QIcon(":/icons/timecurve.png"));
            if (m_plot->playAct()) {
                playAct = m_plot->playAct();
                toolBar()->addAction(playAct);
            }
            break;
        }
        case Plot::PlotType::General: {
            m_plot = new Plot(type, this);
            setIcon(QIcon(":/icons/linecurve.png"));
            break;
        }
    }


    plotsLayout->addWidget(m_plot,1,0);

    connect(m_plot, SIGNAL(curvesCountChanged()), this, SIGNAL(curvesCountChanged()));
    connect(m_plot, SIGNAL(channelPlotted(Channel*)), this, SIGNAL(channelPlotted(Channel*)));
    connect(m_plot, SIGNAL(curveDeleted(Channel*)), this, SIGNAL(curveDeleted(Channel*)));
    connect(m_plot, SIGNAL(needPlotChannels(bool,QVector<Channel*>)), this, SIGNAL(needPlotChannels(bool,QVector<Channel*>)));
    connect(m_plot, SIGNAL(focusThisPlot()), this, SLOT(setFocus()));
    connect(m_plot, SIGNAL(trackingPanelCloseRequested()), trackingCursorAct, SLOT(toggle()));

    bool autoscale = App->getSetting("autoscale-x", true).toBool();
    m_plot->toggleAutoscale(0 /* x axis */, autoscale);
    autoscale = App->getSetting("autoscale-y", true).toBool();
    m_plot->toggleAutoscale(1 /* y axis */, autoscale);
    autoscale = App->getSetting("autoscale-y-slave", true).toBool();
    m_plot->toggleAutoscale(2 /* y slave axis */, autoscale);

    emit curvesCountChanged();
}

void PlotArea::update()
{
    if (m_plot) m_plot->update();
}

//QList<Curve *> PlotArea::curves() const
//{
//    if (plot) return plot->curves;
//    return QList<Curve *>();
//}

void setLineColor(QAxObject *obj, int color)
{DD;
    QAxObject *format = obj->querySubObject("Format");
    QAxObject *formatLine = format->querySubObject("Line");
    formatLine->setProperty("Visible",true);
    QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
    formatLineForeColor->setProperty("ObjectThemeColor", color);

    delete formatLineForeColor;
    delete formatLine;
    delete format;
}

void setAxis(QAxObject *xAxis, const QString &title)
{DD;
    xAxis->dynamicCall("SetHasTitle(bool)",true);
    QAxObject *axisTitle = xAxis->querySubObject("AxisTitle()");
    axisTitle->setProperty("Text", title);
    xAxis->setProperty("MinorTickMark", /*xlOutside*/3);
    xAxis->dynamicCall("SetHasMajorGridlines(bool)", true);
    QAxObject *xGridLines = xAxis->querySubObject("MajorGridlines()");
    QAxObject *xGridFormat = xGridLines->querySubObject("Format");
    QAxObject *xGridFormatLine = xGridFormat->querySubObject("Line");
    xGridFormatLine->setProperty("Visible",true);
    xGridFormatLine->setProperty("DashStyle",7);

    QAxObject *format = xAxis->querySubObject("Format");
    QAxObject *formatLine = format->querySubObject("Line");
    formatLine->setProperty("Visible",true);
    QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
    formatLineForeColor->setProperty("ObjectThemeColor", 13);

    delete formatLineForeColor;
    delete formatLine;
    delete format;
    delete xGridFormatLine;
    delete xGridFormat;
    delete xGridLines;
    delete axisTitle;
}

void PlotArea::exportToExcel(bool fullRange, bool dataOnly)
{DD;
    static QAxObject *excel = 0;

   // QList<Curve*> &curves = m_plot->curves;

    const auto size = m_plot->model()->size();

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
//     const double step = channel->xStep();
//     const bool zeroStepDetected = step<1e-9;

     const bool writeToSeparateColumns = !allChannelsHaveSameXStep || !allChannelsHaveSameLength;

     double minX = channel->data()->xMin();
     double maxX = channel->data()->xMax();

     Range range = m_plot->xRange();

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

     if (!excel) {
         //excel = new QAxObject("Excel.Application",this);
         excel = new QAxObject("{00024500-0000-0000-c000-000000000046}&",this);
     }
     if (!excel) return;
     //qDebug()<<excel->generateDocumentation();

     excel->setProperty("Visible", true);

     //получаем рабочую книгу
     QAxObject * workbooks = excel->querySubObject("WorkBooks");
     QAxObject * workbook = excel->querySubObject("ActiveWorkBook");
     if (!workbook) {
         workbooks->dynamicCall("Add");
     }
     workbook = excel->querySubObject("ActiveWorkBook");

     // получаем список листов и добавляем новый лист
     QAxObject *worksheets = workbook->querySubObject("Sheets");
     worksheets->dynamicCall("Add()");
     QAxObject * worksheet = workbook->querySubObject("ActiveSheet");

     // записываем название файла и описатели
     if (allChannelsFromOneFile) {
         QStringList descriptions = descriptor->dataDescription().twoStringDescription();
         while (descriptions.size()<2) descriptions << "";

         QAxObject *cells = worksheet->querySubObject("Cells(Int,Int)", 1, 1);
         if (cells) cells->setProperty("Value", descriptor->fileName());

         cells = worksheet->querySubObject("Cells(Int,Int)", 2, 2);
         if (cells) cells->setProperty("Value", descriptions.constFirst());

         cells = worksheet->querySubObject("Cells(Int,Int)", 3, 2);
         if (cells) cells->setProperty("Value", descriptions.at(1));

         delete cells;
     }
     else {
         for (int i=0; i<size; ++i) {
             Curve *curve = m_plot->model()->curve(i);
             QStringList descriptions = curve->channel->descriptor()->dataDescription().twoStringDescription();
             while (descriptions.size()<2) descriptions << "";

             QAxObject *cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 1, 2+i)
                                                       : worksheet->querySubObject("Cells(Int,Int)", 1, 2+i*2);
             cells->setProperty("Value", curve->channel->descriptor()->fileName());

             cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 2, 2+i)
                                            : worksheet->querySubObject("Cells(Int,Int)", 2, 2+i*2);
             if (cells) cells->setProperty("Value", descriptions.constFirst());

             cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 3, 2+i)
                                            : worksheet->querySubObject("Cells(Int,Int)", 3, 2+i*2);
             if (cells) cells->setProperty("Value", descriptions.at(1));

             delete cells;
         }
     }

     // записываем название канала
     for (int i=0; i<size; ++i) {
         Curve *curve = m_plot->model()->curve(i);
         QAxObject *cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 4, 2+i)
                                                   : worksheet->querySubObject("Cells(Int,Int)", 4, 2+i*2);
         cells->setProperty("Value", curve->title());
         delete cells;
     }

     QVector<int> selectedSamples;

     // если все каналы имеют одинаковый шаг по х, то в первый столбец записываем
     // данные х
     // если каналы имеют разный шаг по х, то для каждого канала отдельно записываем
     // по два столбца
     // если шаг по х нулевой, то предполагаем октаву, размножаем данные для графика
     if (!writeToSeparateColumns) {
         const int numCols = size;
         const bool zeroStep = qFuzzyIsNull(channel->data()->xStep());

         QList<QVariant> cellsList;
         QList<QVariant> rowsList;
         for (int i = 0; i < samplesCount; ++i) {
             double val = channel->data()->xValue(i);
             if (!fullRange && (val < range.min || val > range.max) ) continue;

             cellsList.clear();

             if (zeroStep && exportPlots) {//размножаем каждое значение на 2
                 double f1 = i==0 ? val/pow(10.0,0.05):sqrt(val*channel->data()->xValue(i-1));
                 double f2 = i==samplesCount-1?val*pow(10.0,0.05):sqrt(val*channel->data()->xValue(i+1));
                 //первый ряд: (f1, Li)
                 cellsList << f1;
                 for (int j = 0; j < numCols; ++j) {
                     cellsList << m_plot->model()->curve(j)->channel->data()->yValue(i);
                 }
                 rowsList << QVariant(cellsList);
                 cellsList.clear();
                 //второй ряд: (f2, Li)
                 cellsList << f2;
                 for (int j = 0; j < numCols; ++j) {
                     cellsList << m_plot->model()->curve(j)->channel->data()->yValue(i);
                 }
                 rowsList << QVariant(cellsList);
                 cellsList.clear();
             }
             else {
                 cellsList << val;
                 for (int j = 0; j < numCols; ++j) {
                     cellsList << m_plot->model()->curve(j)->channel->data()->yValue(i);
                 }
                 rowsList << QVariant(cellsList);
             }
         }
         int numRows = rowsList.size();

         QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1);
         QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + numRows - 1, 1 + numCols);
         QAxObject* range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );

         range->setProperty("Value", QVariant(rowsList) );

         // выделяем диапазон, чтобы он автоматически использовался для построения диаграммы
         Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 4, 1);
         Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 4 + numRows, 1 + numCols);
         range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );
         range->dynamicCall("Select (void)");

         delete range;
         delete Cell1;
         delete Cell2;
     }
     else {
         for (int i=0; i<size; ++i) {
             Curve *curve = m_plot->model()->curve(i);
             Channel *ch = curve->channel;
             bool zeroStep = qFuzzyIsNull(ch->data()->xStep());

             QList<QVariant> cellsList;
             QList<QVariant> rowsList;
             for (int j = 0; j < ch->data()->samplesCount(); j++) {
                 double val = ch->data()->xValue(j);
                 if (!fullRange && (val < range.min || val > range.max) ) continue;

                 cellsList.clear();
                 if (zeroStep && exportPlots) {//размножаем каждое значение на 2
                     double f1 = j==0 ? val/pow(10.0,0.05):sqrt(val*ch->data()->xValue(j-1));
                     double f2 = j==ch->data()->samplesCount()-1?val*pow(10.0,0.05):sqrt(val*ch->data()->xValue(j+1));
                     //первый ряд: (f1, Li)
                     cellsList << f1 << ch->data()->yValue(j);
                     rowsList << QVariant(cellsList);
                     cellsList.clear();
                     //второй ряд: (f2, Li)
                     cellsList << f2 << ch->data()->yValue(j);
                     rowsList << QVariant(cellsList);
                     cellsList.clear();
                 }
                 else {
                     cellsList << val << ch->data()->yValue(j);
                     rowsList << QVariant(cellsList);
                 }
             }
             int numRows = rowsList.size();
             selectedSamples << numRows;

             QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1+i*2);
             QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + numRows-1, 2 + i*2);
             QAxObject* range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );

             range->setProperty("Value", QVariant(rowsList) );

             delete Cell1;
             delete Cell2;
             delete range;
         }
     }

     if (exportPlots) {
         QAxObject *charts = workbook->querySubObject("Charts");
         charts->dynamicCall("Add()");
         QAxObject *chart = workbook->querySubObject("ActiveChart");
         chart->setProperty("ChartType", 75);
         QAxObject * series = chart->querySubObject("SeriesCollection");

         // отдельно строить кривые нужно, только если у нас много пар столбцов с данными
         if (writeToSeparateColumns) {
             int seriesCount = series->property("Count").toInt();

             // удаляем предыдущие графики, созданные по умолчанию
             bool ok=true;
             while ((seriesCount>0) && ok) {
                 QAxObject * serie = series->querySubObject("Item (int)", 1);
                 if (serie) {
                     serie->dynamicCall("Delete()");
                     delete serie;
                     seriesCount--;
                 }
                 else ok=false;
             }

             for (int i=0; i<size; ++i) {
                 QAxObject * serie = series->querySubObject("NewSeries()");
                 if (serie) {
                     //xvalues
                     QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1+i*2);
                     QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + selectedSamples.at(i)-1, 1 + i*2);
                     if (Cell1 && Cell2) {
                         QAxObject * xvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
                         if (xvalues)
                             serie->setProperty("XValues", xvalues->asVariant());
                         delete xvalues;
                     }
                     delete Cell1;
                     delete Cell2;

                     //yvalues
                     Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 2+i*2);
                     Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + selectedSamples.at(i)-1, 2 + i*2);
                     if (Cell1 && Cell2) {
                         QAxObject * yvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
                         if (yvalues)
                             serie->setProperty("Values", yvalues->asVariant());
                         delete yvalues;
                     }
                     delete Cell1;
                     delete Cell2;
                 }
                 delete serie;
             }
         }

         // перемещаем графики на дополнительную вертикальную ось,
         // если они были там в программе
         // и меняем название кривой
         int seriesCount = series->property("Count").toInt();
         bool addRightAxis = false;
         for ( int i=0; i<seriesCount; ++i) {
             Curve *curve = m_plot->model()->curve(i);
             QAxObject * serie = series->querySubObject("Item (int)", i+1);
             if (serie) {
                 if (curve->yAxis()==QwtAxis::YRight) {
                     serie->setProperty("AxisGroup", 2);
                     addRightAxis = true;
                 }
                 serie->setProperty("Name", curve->channel->name());
             }
             delete serie;
         }
         if (addRightAxis) {
             QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&,const QVariant&)", 2,2);
             if (yAxis) setAxis(yAxis, stripHtml(m_plot->axisTitleText(m_plot->yRightAxis)));
             delete yAxis;
         }


         // добавляем подписи осей
         QAxObject *xAxis = chart->querySubObject("Axes(const QVariant&)", 1);
         if (xAxis) {
             setAxis(xAxis, stripHtml(m_plot->axisTitleText(QwtAxis::XBottom)));
             xAxis->setProperty("MaximumScale", range.max);
             xAxis->setProperty("MinimumScale", int(range.min/10)*10);
//             if (zeroStepDetected) {
//                 xAxis->setProperty("ScaleType", "xlLogarithmic");
//                 xAxis->setProperty("LogBase", 2);
//             }
         }
         delete xAxis;

         QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&)", 2);
         if (yAxis) {
             setAxis(yAxis, stripHtml(m_plot->axisTitleText(QwtAxis::YLeft)));
             yAxis->setProperty("CrossesAt", -1000);
         }
         delete yAxis;

         // рамка вокруг графика
         QAxObject *plotArea = chart->querySubObject("PlotArea");
         if (plotArea) setLineColor(plotArea, 13);
         delete plotArea;

         // цвета графиков
         for (int i = 0; i< size; ++i) {
             Curve *curve = m_plot->model()->curve(i);
             QAxObject * serie = series->querySubObject("Item(int)", i+1);
             if (serie) {
                 QAxObject *format = serie->querySubObject("Format");
                 QAxObject *formatLine = format->querySubObject("Line");
                 if (formatLine) {
                     formatLine->setProperty("Weight", m_plot->model()->curve(i)->pen().width());
                     Qt::PenStyle lineStyle = m_plot->model()->curve(i)->pen().style();
                     int msoLineStyle = 1;
                     switch (lineStyle) {
                         case Qt::NoPen:
                         case Qt::SolidLine: msoLineStyle = 1; break;
                         case Qt::DashLine: msoLineStyle = 4; break;
                         case Qt::DotLine: msoLineStyle = 3; break;
                         case Qt::DashDotLine: msoLineStyle = 5; break;
                         case Qt::DashDotDotLine: msoLineStyle = 6; break;
                         default: break;
                     }
                     formatLine->setProperty("DashStyle", msoLineStyle);

                     QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
                     QColor color = m_plot->model()->curve(i)->pen().color();
                     //меняем местами красный и синий, потому что Excel неправильно понимает порядок
                     int red = color.red();
                     color.setRed(color.blue());
                     color.setBlue(red);
                     if (formatLineForeColor) formatLineForeColor->setProperty("RGB", color.rgb());
                     delete formatLineForeColor;
                 }

                 foreach(PointLabel *label, curve->labels) {
                     QAxObject* point = serie->querySubObject("Points(QVariant)", label->point()+1);
                     QVariantList options = {0, 0, 0, 0, 0, -1, 0, 0, 0, 0};

                     point->dynamicCall("ApplyDataLabels()", options);
                     QAxObject* dataLabel = point->querySubObject("DataLabel");
                     dataLabel->setProperty("ShowCategoryName", -1);
                     dataLabel->setProperty("ShowValue", 0);
                     dataLabel->setProperty("Position", 0);
                     delete dataLabel;
                     delete point;
                 }

                 delete formatLine;
                 delete format;
             }
             delete serie;
         }

         QAxObject *chartArea = chart->querySubObject("ChartArea");
         chartArea->querySubObject("Format")->querySubObject("Line")->setProperty("Visible", 0);
         delete chartArea;

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
     }

//    QFile file1("chartArea.html");
//    file1.open(QIODevice::WriteOnly | QIODevice::Text);
//    QTextStream out(&file1);
//    out << chartArea->generateDocumentation();
//    file1.close();


     delete worksheet;
     delete worksheets;
     delete workbook;
     delete workbooks;
}

void PlotArea::updateActions(int filesCount, int channelsCount)
{
    const bool hasCurves = m_plot ? curvesCount()>0 : false;
    const bool allCurvesFromSameDescriptor = m_plot ? m_plot->model()->allCurvesFromSameDescriptor() : false;

    clearPlotAct->setEnabled(hasCurves);
    savePlotAct->setEnabled(hasCurves);
    copyToClipboardAct->setEnabled(hasCurves);
    printPlotAct->setEnabled(hasCurves);
    if (playAct) playAct->setEnabled(hasCurves);
    previousDescriptorAct->setEnabled(filesCount>1 && allCurvesFromSameDescriptor);
    nextDescriptorAct->setEnabled(filesCount>1 && allCurvesFromSameDescriptor);
    arbitraryDescriptorAct->setEnabled(filesCount>1 && allCurvesFromSameDescriptor);
    cycleChannelsUpAct->setEnabled(channelsCount>1 && hasCurves);
    cycleChannelsDownAct->setEnabled(channelsCount>1 && hasCurves);
}

void PlotArea::deleteCurvesForDescriptor(FileDescriptor *f)
{
    if (m_plot) m_plot->deleteCurvesForDescriptor(f);
}

//вызывается при переходе на предыдущую/следующую запись
void PlotArea::replotDescriptor(FileDescriptor *f, int fileIndex)
{
    if (m_plot && m_plot->sergeiMode) {
        m_plot->deleteAllCurves(false);
        m_plot->plotCurvesForDescriptor(f, fileIndex);
    }
}

QVector<Channel *> PlotArea::plottedChannels() const
{
    if (m_plot) return m_plot->model()->plottedChannels();
    return QVector<Channel *>();
}

QVector<FileDescriptor *> PlotArea::plottedDescriptors() const
{
    if (m_plot) return m_plot->model()->plottedDescriptors();
    return QVector<FileDescriptor *>();
}

int PlotArea::curvesCount(int type) const
{
    return m_plot?m_plot->curvesCount(type):0;
}

void PlotArea::updateLegends()
{
    if (m_plot) m_plot->updateLegends();
}


void PlotArea::dragEnterEvent(QDragEnterEvent *event)
{
    setFocus();

    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData && !m_plot) {
        event->acceptProposedAction();
    }
}

void PlotArea::dropEvent(QDropEvent *event)
{
    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        onDropEvent(myData->channels);
        event->acceptProposedAction();
    }
}

void PlotArea::onDropEvent(const QVector<Channel *> &channels)
{
    auto type = Plot::PlotType::General;
    if (!channels.isEmpty()) {
        if (channels.first()->type() == Descriptor::TimeResponse)
            type = Plot::PlotType::Time;
        if (channels.first()->data()->blocksCount()>1)
            type = Plot::PlotType::Spectrogram;
        if (channels.first()->data()->xValuesFormat() == DataHolder::XValuesNonUniform)
            type = Plot::PlotType::Octave;
    }
    addPlot(type);
    m_plot->onDropEvent(true, channels);
}
