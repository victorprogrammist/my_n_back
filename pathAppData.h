
#ifndef __PATH_APP_DATA_H__
#define __PATH_APP_DATA_H__

#include <QStandardPaths>
#include <QDir>

inline QString pathAppData() {
    QString res =
        QStandardPaths::writableLocation(
            QStandardPaths::StandardLocation::AppDataLocation);

    QDir dir(res);
    if (!dir.exists())
        dir.mkpath(".");

    return res;
}

#endif
