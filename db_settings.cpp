
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include "db_settings.h"

void DbTools::execWithCheck(QSqlQuery& qu) {
    if (!qu.exec())
        qDebug() << qu.lastError().text();
}

void DbTools::execWithCheck(const QString& txt) {
    QSqlQuery qu;
    qu.prepare(txt);
    execWithCheck(qu);
}

static void initTableSettings() {
    static bool already = false;
    if (already) return;

    already = true;

    QSqlQuery qu;

    qu.prepare(
        "create table if not exists SETTINGS ("
        "   name text primary key, "
        "   value text)");

    DbTools::execWithCheck(qu);
}

bool DbSettings::getBool(const QString& name) {
    return getSettings(name) == "TRUE";
}

void DbSettings::setBool(const QString& name, bool value) {
    if (value)
        setSettings(name, "TRUE");
    else
        setSettings(name, "FALSE");
}

QString DbSettings::getSettings(const QString& name) {

    initTableSettings();

    QSqlQuery query;
    query.prepare("select value from SETTINGS where name=?");
    query.addBindValue(name);
    DbTools::execWithCheck(query);

    if (query.next())
        return query.value(0).toString();

    return {};
}

void DbSettings::setSettings(const QString& name, const QString& value) {

    initTableSettings();

    QSqlQuery query;
    query.prepare(
    "replace into SETTINGS (name,value) values(?,?)");
    query.addBindValue(name);
    query.addBindValue(value);
    DbTools::execWithCheck(query);
}

