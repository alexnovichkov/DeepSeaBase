#include "framecutterfunction.h"
#include "logging.h"
#include "fileformats/filedescriptor.h"

QStringList getBlocks(double xStep, const QString &fix)
{
//    double sampleRate = 1.0/xStep;
//    DebugPrint(sampleRate);
    int sampleRate = qRound(1.0 / xStep);
    int maxBlockSize = 65536;
    double delta = double(sampleRate) / double(maxBlockSize);
    double l = 1.0 / sampleRate * maxBlockSize;

    QStringList values;
    for (int i=0; i<10; ++i) {
        values << QString("%1%2 / %3 Гц / %4 c%1")
                  .arg(fix).arg(maxBlockSize).arg(delta, 0,'g').arg(l, 0,'g');
        maxBlockSize = maxBlockSize >> 1;
        delta *= 2.0;
        l /= 2.0;
    }
    return values;
}

FrameCutterFunction::FrameCutterFunction(QObject *parent) :
    AbstractFunction(parent)
{
    // default values
    setProperty("FrameCutter/type", 0);
    setProperty("FrameCutter/blockSize", 0);
    setProperty("FrameCutter/percent", 0);
    setProperty("FrameCutter/deltaTime", 0.0);
    setProperty("FrameCutter/triggerMode", 0);
    setProperty("FrameCutter/level", 0.0);
    setProperty("FrameCutter/channel", 0);
    setProperty("FrameCutter/pretrigger", 0.0);
}


QString FrameCutterFunction::name() const
{
    return "FrameCutter";
}

QString FrameCutterFunction::displayName() const
{
    return "Выборка блоков";
}

QString FrameCutterFunction::description() const
{
    return "Разбиение данных на блоки";
}

QVariant FrameCutterFunction::getProperty(const QString &property) const
{
    if (property.startsWith("?/")) {
        if (property == "?/blockSize") {
            return frameCutter.blockSize();
        }
        if (property == "?/xDelta") {
            int sampleRate = qRound(1.0 / frameCutter.getXStep());
            double delta = double(sampleRate) / double(frameCutter.blockSize());
            return delta;
        }
        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }

    if (!property.startsWith(name()+"/")) return QVariant();

    return parameters.value(property.section("/",1));
}

void FrameCutterFunction::setProperty(const QString &property, const QVariant &val)
{
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    parameters.insert(p, val);

    if (p == "type")
        frameCutter.setType(val.toInt());
    else if (p == "blockSize") {
        qDebug()<<"setting FrameCutter/blocksize as"<<(65536 >> val.toInt());
        //double p = pow(2.0, val.toInt()); DebugPrint(p);
        //int sampleRate = int (1.0/frameCutter.getXStep()); DebugPrint(sampleRate);
        frameCutter.setBlockSize(65536 >> val.toInt());
    }
    else if (p == "xStep") {
        frameCutter.setXStep(val.toDouble());
        // мы должны обновить список blockSize
        emit attributeChanged(name()+"/blockSize", getBlocks(frameCutter.getXStep(), ""), "enumNames");
    }
    else if (p == "percent")
        frameCutter.setDelta(int(1.0 * frameCutter.blockSize() * val.toDouble()));
    else if (p == "deltaTime") {
        if (frameCutter.getXStep()!=0.0)
            frameCutter.setDelta(int(val.toDouble()/frameCutter.getXStep()));
    }
    else if (p == "triggerMode")
        frameCutter.setMode(val.toInt());
    else if (p == "level")
        frameCutter.setLevel(val.toDouble());
    else if (p == "channel")
        frameCutter.setChannel(val.toInt()-1);
    else if (p == "pretrigger")
        frameCutter.setPretrigger(int(val.toDouble()/frameCutter.getXStep()));
}

bool FrameCutterFunction::propertyShowsFor(const QString &property) const
{
    if (!property.startsWith(name()+"/")) return false;
    QString p = property.section("/",1);

    if (p == "percent" && frameCutter.type() != FrameCutter::Overlap) return false;
    if (p == "deltaTime" && (frameCutter.type() == FrameCutter::Continuous ||
                                    frameCutter.type() == FrameCutter::Overlap)) return false;
    if (p == "triggerMode" && frameCutter.type() != FrameCutter::Trigger) return false;
    if (p == "level" && frameCutter.type() != FrameCutter::Trigger) return false;
    if (p == "channel" && frameCutter.type() != FrameCutter::Trigger) return false;
    if (p == "pretrigger" && frameCutter.type() != FrameCutter::Trigger) return false;

    return true;
}


QStringList FrameCutterFunction::properties() const
{
    return QStringList()<<
                           "type"<<
                           "blockSize"<<
                           "percent"<<
                           "deltaTime"<<
                           "triggerMode"<<
                           "level"<<
                           "channel"<<
                           "pretrigger";
}

QString FrameCutterFunction::propertyDescription(const QString &property) const
{
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Способ\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Подряд | Перекрытие | Смещение | Триггер\","
                                   "  \"values\"      : [\"Подряд\", \"Перекрытие\", \"Смещение\", \"Триггер\"],"
                                   "  \"minimum\"     : 0,"
                                   "  \"maximum\"     : 0"
                                   "}";
    if (property == "blockSize") {
        QStringList values = getBlocks(frameCutter.getXStep(), "\"");
        return QString("{"
                       "  \"name\"        : \"blockSize\"   ,"
                       "  \"type\"        : \"enum\"   ,"
                       "  \"displayName\" : \"Размер блока\"   ,"
                       "  \"defaultValue\": 0         ,"
                       "  \"toolTip\"     : \"Размер | ΔF [Гц]| Длина [с]\","
                       "  \"values\"      : [%1]," //для enum
                       "  \"minimum\"     : 0," //для int и double
                       "  \"maximum\"     : 0" //для int и double
                       "}").arg(values.join(","));
    }
    if (property == "percent") return "{"
                                      "  \"name\"        : \"percent\"   ,"
                                      "  \"type\"        : \"int\"   ,"
                                      "  \"displayName\" : \"%\"   ,"
                                      "  \"defaultValue\": 0         ,"
                                      "  \"toolTip\"     : \"0-100%\","
                                      "  \"values\"      : [],"
                                      "  \"minimum\"     : 0,"
                                      "  \"maximum\"     : 99"
                                      "}";
    if (property == "deltaTime") return "{"
           "  \"name\"        : \"deltaTime\"   ,"
           "  \"type\"        : \"double\"   ,"
           "  \"displayName\" : \"Время [с]\"   ,"
           "  \"defaultValue\": 0.0        ,"
           "  \"toolTip\"     : \"Время между блоками\","
           "  \"minimum\"     : 0.0," //для int и double
           "  \"values\"      : []" //для enum
           "}";

    if (property == "triggerMode") return "{"
           "  \"name\"        : \"triggerMode\"   ,"
           "  \"type\"        : \"enum\"   ,"
           "  \"displayName\" : \"Режим триггера\"   ,"
           "  \"defaultValue\": 0        ,"
           "  \"toolTip\"     : \" Больше чем | Меньше чем\","
           "  \"values\"      : [\"Больше чем\", \"Меньше чем\"]" //для enum
           "}";
    if (property == "level") return "{"
           "  \"name\"        : \"level\"   ,"
           "  \"type\"        : \"double\"   ,"
           "  \"displayName\" : \"Уровень триггера\"   ,"
           "  \"defaultValue\": 0.0        ,"
           "  \"toolTip\"     : \"Уровень срабатывания триггера\","
           "  \"values\"      : []" //для enum
           "}";
    if (property == "channel") return "{"
           "  \"name\"        : \"channel\"   ,"
           "  \"type\"        : \"int\"   ,"
           "  \"displayName\" : \"Канал триггера\"   ,"
           "  \"defaultValue\": 1        ,"
           "  \"toolTip\"     : \"Канал триггера\","
           "  \"minimum\"     : 1," //для int и double
           "  \"values\"      : []" //для enum
           "}";
    if (property == "pretrigger") return "{"
           "  \"name\"        : \"pretrigger\"   ,"
           "  \"type\"        : \"double\"   ,"
           "  \"displayName\" : \"Пре-триггер [с]\"   ,"
           "  \"defaultValue\": 0.0        ,"
           "  \"toolTip\"     : \"Время до тригера, включаемое в блок\","
           "  \"minimum\"     : 0.0," //для int и double
           "  \"values\"      : []" //для enum
           "}";
    return "";
}


void FrameCutterFunction::reset()
{
    frameCutter.reset();
    output.clear();
}


QVector<double> FrameCutterFunction::getData(const QString &id)
{
    if (id == "input")
        return output;

    return QVector<double>();
}

bool FrameCutterFunction::compute(FileDescriptor *file)
{
    if (!m_input) return false;

    output.clear();

    bool isEmpty = frameCutter.isEmpty();
    if (isEmpty) {
        if (!m_input->compute(file)) return false;
        QVector<double> data = m_input->getData("input");
        if (data.isEmpty()) return false;
        frameCutter.setSource(data);
        if (frameCutter.type()==FrameCutter::Trigger) {
            // TODO: как установить данные для триггера?
            m_input->setProperty("", 0);

            frameCutter.setTriggerSource(m_input->getData("triggerInput"));
        }
    }

    bool ok;
    output = frameCutter.get(&ok);
    if (!ok || output.isEmpty()) return false;

    return true;
}


void FrameCutterFunction::updateProperty(const QString &property, const QVariant &val)
{
    // нам может прийти измененный шаг по оси Х
    if (property == "?/xStep") {
        setProperty(name()+"/xStep", val);
    }
}
