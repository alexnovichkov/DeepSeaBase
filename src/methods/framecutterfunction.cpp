#include "framecutterfunction.h"
#include "logging.h"
#include "fileformats/filedescriptor.h"

QStringList getBlocks(double xStep, const QString &fix)
{DD;
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

FrameCutterFunction::FrameCutterFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;
    // default values
    setParameter("FrameCutter/type", 0);
    setParameter("FrameCutter/blockSize", 0);
    setParameter("FrameCutter/percent", 0);
    setParameter("FrameCutter/deltaTime", 0.0);
    setParameter("FrameCutter/triggerMode", 0);
    setParameter("FrameCutter/level", 0.0);
    setParameter("FrameCutter/channel", 0);
    setParameter("FrameCutter/pretrigger", 0.0);
}


QString FrameCutterFunction::name() const
{DD;
    return "FrameCutter";
}

QString FrameCutterFunction::displayName() const
{DD;
    return "Выборка блоков";
}

QString FrameCutterFunction::description() const
{DD;
    return "Разбиение данных на блоки";
}

QVariant FrameCutterFunction::m_getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        if (property == "?/blockSize") {
            return frameCutter.blockSize();
        }
        if (property == "?/zCount") {
            //количество блоков, которые могут потом усредняться
            return frameCutter.getBlocksCount();
        }
        if (property == "?/zStep") {
            return frameCutter.blockSize() * frameCutter.getXStep();
        }

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getParameter(property);
    }

    if (!property.startsWith(name()+"/")) return QVariant();

    return parameters.value(property.section("/",1));
}

DataDescription FrameCutterFunction::getFunctionDescription() const
{DD;
    DataDescription result = AbstractFunction::getFunctionDescription();

    result.put("function.name", "FrameCutter");
    result.put("function.type", 1);
    switch (frameCutter.type()) {
        case 0: result.put("function.frameCutterType", "continuous"); break;
        case 1:
            result.put("function.frameCutterType", "overlap");
            if (frameCutter.blockSize()!=0)
                result.put("function.frameCutterOverlap", 1.0 * frameCutter.getDelta() / frameCutter.blockSize());
            break;
        case 2: result.put("function.frameCutterDelta", frameCutter.getDelta()); break;
        case 3:
            result.put("function.frameCutterTriggerLevel", frameCutter.getLevel());
            result.put("function.frameCutterPretrigger", frameCutter.getPretrigger());
            result.put("function.frameCutterTriggerChannel", frameCutter.getChannel()+1);
            break;
    }

    return result;
}

void FrameCutterFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    parameters.insert(p, val);

    if (p == "type") {
        frameCutter.setType(val.toInt());
    }
    else if (p == "blockSize") {
        //LOG(DEBUG)<<"setting FrameCutter/blocksize as"<<(65536 >> val.toInt());
        //double p = pow(2.0, val.toInt()); DebugPrint(p);
        //int sampleRate = int (1.0/frameCutter.getXStep()); DebugPrint(sampleRate);
        frameCutter.setBlockSize(65536 >> val.toInt());
        emit propertyChanged("?/blockSize", 65536 >> val.toInt());
    }
    else if (p == "xStep") {
        frameCutter.setXStep(val.toDouble());
        // мы должны обновить список blockSize
        emit attributeChanged(this, name()+"/blockSize", getBlocks(frameCutter.getXStep(), ""), "enumNames");
    }
    else if (p == "percent") {
        frameCutter.setDelta(val.toDouble(), FrameCutter::DeltaPercent);
    }
    else if (p == "deltaTime") {
        if (frameCutter.getXStep()!=0.0) {
            frameCutter.setDelta(val.toDouble(), FrameCutter::DeltaTime);
        }
    }
    else if (p == "triggerMode") {
        frameCutter.setMode(val.toInt());
    }
    else if (p == "level") {
        frameCutter.setLevel(val.toDouble());
    }
    else if (p == "triggerChannel") {
        frameCutter.setChannel(val.toInt()-1);
        emit propertyChanged("?/triggerChannel", val.toInt()-1);
    }
    else if (p == "pretrigger") {
        frameCutter.setPretrigger(int(val.toDouble()/frameCutter.getXStep()));
    }
}

bool FrameCutterFunction::propertyShowsFor(const QString &property) const
{DD;
    if (!property.startsWith(name()+"/")) return false;
    QString p = property.section("/",1);

    if (p == "percent" && frameCutter.type() != FrameCutter::Overlap) return false;
    if (p == "deltaTime" && (frameCutter.type() == FrameCutter::Continuous ||
                                    frameCutter.type() == FrameCutter::Overlap)) return false;
    if (p == "triggerMode" && frameCutter.type() != FrameCutter::Trigger) return false;
    if (p == "level" && frameCutter.type() != FrameCutter::Trigger) return false;
    if (p == "triggerChannel" && frameCutter.type() != FrameCutter::Trigger) return false;
    if (p == "pretrigger" && frameCutter.type() != FrameCutter::Trigger) return false;

    return true;
}


QStringList FrameCutterFunction::properties() const
{DD;
    return QStringList()<<
                           "type"<<
                           "blockSize"<<
                           "percent"<<
                           "deltaTime"<<
                           "triggerMode"<<
                           "level"<<
                           "triggerChannel"<<
                           "pretrigger";
}

QString FrameCutterFunction::propertyDescription(const QString &property) const
{DD;
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
    if (property == "triggerChannel") return "{"
           "  \"name\"        : \"triggerChannel\"   ,"
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
           "  \"toolTip\"     : \"Время до триггера, включаемое в блок\","
           "  \"minimum\"     : 0.0," //для int и double
           "  \"values\"      : []" //для enum
           "}";
    return "";
}

void FrameCutterFunction::reset()
{DD;
    frameCutter.reset();
    output.clear();
}

void FrameCutterFunction::resetData()
{DD;
    frameCutter.reset(false);
    AbstractFunction::resetData();
}

bool FrameCutterFunction::compute(FileDescriptor *file)
{DD;
    if (!m_input) return false;

    output.clear();
//    int size = 0;
    //возвращает сразу все данные, склеенные вместе блоками размером ?/blockSize
    if (frameCutter.isEmpty()) {
        if (!m_input->compute(file)) {
            return false;
        }
        QVector<double> data = m_input->getData("input");
//        size = data.size();
        if (data.isEmpty()) {
            return false;
        }
        frameCutter.setSource(data);

        if (frameCutter.type()==FrameCutter::Trigger) {
            // TODO: как установить данные для триггера?
            //m_input->setParameter("", 0);

            frameCutter.setTriggerSource(m_input->getData("triggerInput"));
        }
    }

    output = frameCutter.getAll();
//    LOG(DEBUG) << "after sampling" << output.size() << "with" << frameCutter.getBlocksCount()
//             << "blocks" << "from source of" << size;
    if (output.isEmpty()) return false;

    return true;
}


void FrameCutterFunction::updateProperty(const QString &property, const QVariant &val)
{DD;
    // нам может прийти измененный шаг по оси Х
    if (property == "?/xStep") {
        setParameter(name()+"/xStep", val);
    }
}
