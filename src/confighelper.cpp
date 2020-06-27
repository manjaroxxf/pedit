#include "confighelper.h"

#include <QDir>
#include <QJsonDocument>

ConfigHelper::ConfigHelper()
{
    QString configFile;
#ifdef Q_OS_WIN
    configFile = App.applicationDirPath() + "/config.json";
#else
    QDir configDir = QDir::homePath() + "/.config/notepanda";
    configFile = configDir.absolutePath() + "/config.json";
    if (!configDir.exists()) {
        configDir.mkpath(configDir.absolutePath());
    }
#endif

    const QSettings::Format JsonFormat =
        QSettings::registerFormat("json", readJsonFile, writeJsonFile);
    settings = new QSettings(configFile, JsonFormat, this);
}

bool ConfigHelper::readJsonFile(QIODevice &device, QSettings::SettingsMap &map)
{
    QJsonParseError error;
    map = QJsonDocument::fromJson(device.readAll(), &error).toVariant().toMap();
    return error.error == QJsonParseError::NoError;
}

bool ConfigHelper::writeJsonFile(QIODevice &device,
                                 const QSettings::SettingsMap &map)
{
    const auto json = QJsonDocument::fromVariant(map).toJson();
    return device.write(json) == json.size();
}
