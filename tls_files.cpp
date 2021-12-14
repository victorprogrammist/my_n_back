
#include <QFile>
#include <QDateTime>

void file_write(const QString& fn, const QString& data, bool append) {
    if (data.isEmpty()) return;

    QFile file(fn);
    if (append) {
        file.open(QIODevice::Append | QIODevice::Unbuffered);
    } else {
        file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered);
    }

    file.write(data.toUtf8());

    file.size();
    file.flush();
    file.close();
}

void file_append(const QString& fn, const QString& s) {
    file_write(fn, s, true);
}

void file_log(const QString& fn, const QString& s) {

    QString ss =
            QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") +
            ": " + s + "\r\n";

    file_append(fn, ss);
}
