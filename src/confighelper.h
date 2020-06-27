#ifndef CONFIGHELPER_H
#define CONFIGHELPER_H

#include <QIODevice>
#include <QSettings>

class ConfigHelper : public QSettings
{
    Q_OBJECT

   public:
    ConfigHelper();

    static bool readJsonFile(QIODevice &device, QSettings::SettingsMap &map);
    static bool writeJsonFile(QIODevice &device,
                              const QSettings::SettingsMap &map);

   private:
    QSettings *settings;
};

#endif  // CONFIGHELPER_H
