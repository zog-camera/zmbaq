#ifndef LOGGERSETTINGS_H
#define LOGGERSETTINGS_H

#include <QObject>
#include "mbytearray.h"
#include "zmbaq_common.h"
#include <QJsonObject>

class QFile;

class LoggerSettings : public QObject
{
    Q_OBJECT
public:
    explicit LoggerSettings(QObject *parent = 0);
    ~LoggerSettings();

    static LoggerSettings* instance();
    const QJsonObject& get_settings() const;
    std::string logs_dirname() const;

    int rowsmax() const;

    void set_disk_write(bool onoff, std::string path = "");
    bool disk_write_enabled() const;

    SHP(QFile) make_logfile(const std::string& tag);

signals:
    void runtime_block_feeds();
    void runtime_unblock_feeds();

public slots:

private:

    static std::shared_ptr<LoggerSettings> m_instance;
    static std::mutex m_mutex;
    QJsonObject settings;
    bool runtime_block;

};

#endif // LOGGERSETTINGS_H
