
#ifndef __DB_SETTINGS_H__
#define __DB_SETTINGS_H__

#include <QString>

struct DbTools {
    static void execWithCheck(QSqlQuery& qu);
    static void execWithCheck(const QString& qu);
};

struct DbSettings {

    static bool getBool(const QString& name);
    static void setBool(const QString& name, bool value);

    static QString getSettings(const QString& name);
    static void setSettings(const QString& name, const QString& value);
};

#endif
