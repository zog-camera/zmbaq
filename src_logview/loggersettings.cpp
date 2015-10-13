#include "loggersettings.h"
#include <QDir>
#include <QFile>
#include <QDateTime>


std::shared_ptr<LoggerSettings> LoggerSettings::m_instance;
std::mutex LoggerSettings::m_mutex;

LoggerSettings::LoggerSettings(QObject *parent) : QObject(parent)
{
    runtime_block = false;
    set_disk_write(true);
    settings["rowsmax"] = 5000;
}

LoggerSettings::~LoggerSettings()
{

}

int LoggerSettings::rowsmax() const
{
   return settings["rowsmax"].toInt();
}


LoggerSettings* LoggerSettings::instance()
{
    LoggerSettings* ptr = m_instance.get();
    if (nullptr == ptr)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ptr = m_instance.get();

        if (nullptr == ptr)
        {
            m_instance = std::make_shared<LoggerSettings>();
        }
        ptr = m_instance.get();
    }
    return ptr;
}

const QJsonObject& LoggerSettings::get_settings() const
{
   return settings;
}

void LoggerSettings::set_disk_write(bool onoff, MByteArray path)
{
    QJsonObject disk_o;
    disk_o["logsdir"] = path.empty()? QDir::currentPath() : path.to_qstring();
    disk_o["status"] = QJsonValue(onoff);
    settings["disk"] = QJsonValue(disk_o);
}

MByteArray LoggerSettings::logs_dirname() const
{
    auto jo = settings["disk"].toObject();
    return jo["logsdir"].toString();
}

bool LoggerSettings::disk_write_enabled() const
{
   auto val = settings["disk"].toObject();
   if (!val.isEmpty())
   {
      return val["status"].toBool();
   }
   return false;
}


SHP(QFile) LoggerSettings::make_logfile(const MByteArray& tag)
{
    QDateTime tm  = QDateTime::currentDateTime();
    MByteArray fname = logs_dirname();
    fname += "/log_";
    fname += tag;
    fname += "_";
    fname += tm.toString();
    fname += ".txt";
    auto fptr = std::make_shared<QFile>(make_qlatin1_depend(fname));
    fptr->open(QFile::WriteOnly);
    return fptr;
}
