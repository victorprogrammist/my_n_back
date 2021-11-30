
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QTextCodec>
#include <QDateTime>
#include <assert.h>

QTextCodec* default_codec() {
    return QTextCodec::codecForName("cp1251");
}

QString from_default_codec_to_unicode(const QByteArray& ar) {
    return default_codec()->toUnicode(ar);
}

QByteArray from_unicode_to_default_codec(const QString& s) {
    return default_codec()->fromUnicode(s);
}

QString app_path() {
    QString sp = QDir::separator();
    QString s = QApplication::applicationDirPath().replace("/", sp).replace("\\", sp);
    if (s.right(1) != sp) s += sp;
    return s;
}

bool file_exists(const QString& fn) {
    return QFile::exists(fn);
}

bool file_is_dir(const QString& fn) {
    return file_exists(fn) && QDir(fn).exists();
}

//*********************************
QByteArray file_read_raw(const QString& fn) {
    QFile file(fn);

    if (file.exists() == false) throw "file not found";

    if (file.open(QIODevice::ReadOnly) == false) {
        throw "file not openned";
    }

    return file.read(file.size());
}

QString file_read(const QString& fn) {
    return from_default_codec_to_unicode(file_read_raw(fn));
}

//*********************************
void file_write_raw(const QString& fn, const char* data, int size, bool append) {
    assert( data != nullptr );

    if (size < 0) size = strlen(data);

    QFile file(fn);
    if (append) {
        file.open(QIODevice::Append | QIODevice::Unbuffered);
    } else {
        file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered);
    }

    file.write(data, size);

    file.size();
    file.flush();
    file.close();
}

void file_write(const QString& fn, const QString& s, bool append) {
    QByteArray qba = from_unicode_to_default_codec(s);
    file_write_raw(fn, qba.constData(), qba.length(), append);
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
