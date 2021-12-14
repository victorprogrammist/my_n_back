
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>

#include "pathAppData.h"
#include "db_settings.h"

QString textQueryResuls() {
    return

    "select"
    "  dateTime(t1.dateTime, 'localtime') as dateTime,"
    "  t2.countPassedSteps as steps,"
    "  t2.countErrors as err,"
    "  round(t2.score, 3) as SCORE,"
    "  round("
    "   1. - "
    "   cast(t2.countErrors as double) /"
    "   cast(t2.countPassedSteps as double)"
    "  , 2) as probability"

    " from ("

    " select"
    "  t1.id as idStart,"
    "  max(t2.id) as idLastStep"

    " from"
    "  STARTS as t1"
    "  inner join STEPS as t2"
    "  on t1.id = t2.idStart"

    " where"
    "  (:level = 0 or t2.queueLength = :level)"
    "  and t2.countPassedSteps >= :minCount"
    "  and t1.gameType = :gameType"

    " group by"
    "  t1.id"
    "  ) as t0"
    "  inner join STARTS as t1 on t1.id = t0.idStart"
    "  inner join STEPS as t2 on t2.id = t0.idLastStep"

    " order by t1.dateTime desc";
}

void MainWindow::setQueryResults() {

    QSqlQuery& qu = query_results;

    if (!qu.isActive()) {
        qu = QSqlQuery();
        qu.prepare(textQueryResuls());
    }

    qu.bindValue(":level", m_current_level);
    qu.bindValue(":minCount", m_current_minCount);
    qu.bindValue(":gameType", state_game_type);
    DbTools::execWithCheck(qu);

    model_results.setQuery(qu);
}

void MainWindow::initTableResults() {

    auto* tb = ui->tb_results;

    tb->setModel(&model_results);
    setQueryResults();

    tb->setColumnWidth(0, 200);
}

static bool table_exists(const QString& name) {

    QSqlQuery qu;
    qu.prepare(
    "select name from sqlite_master"
    " where type = 'table' and name = ?");
    qu.addBindValue(name);
    DbTools::execWithCheck(qu);

    return qu.next();
}

static int db_version() {
    QString v = DbSettings::getSettings("db_version");

    if (!v.isEmpty())
        return v.toInt();

    if (table_exists("STARTS"))
        return 1;

    return 0;
}

static void set_db_version(int v) {
    DbSettings::setSettings("db_version", QString::number(v));
}

void MainWindow::initDatabase() {

    const QString driver = "QSQLITE";

    if (!QSqlDatabase::isDriverAvailable(driver))
        return;

    auto db = QSqlDatabase::addDatabase(driver);

    QString s = pathAppData() + "/database.sqlite";
    ui->lb_AppData->setText(s);
    db.setDatabaseName(s);

    connect(ui->bt_open_path_with_database, &QPushButton::clicked,
        [this] { QDesktopServices::openUrl(pathAppData()); });

    if (!db.open())
        return;

    if (db_version() == 1) {
        DbTools::execWithCheck("alter table STARTS add gameType integer");
        DbTools::execWithCheck("update STARTS set gameType = 1");
        DbTools::execWithCheck("alter table STEPS rename probability to score");
        DbTools::execWithCheck("alter table STEPS add asTimeMarker integer");
        DbTools::execWithCheck("update STEPS set asTimeMarker = 0");
    }

    set_db_version(2);

    DbTools::execWithCheck(
    "create table if not exists"
    "   STARTS ("
    "       id integer primary key,"
    "       gameType integer,"
    "       dateTime text"
    "   )");

    DbTools::execWithCheck(
    "create table if not exists"
    "   STEPS ("
    "       id integer primary key,"
    "       idStart integer not null,"
    "       asTimeMarker integer,"
    "       dateTime text,"
    "       queueLength integer,"
    "       hasError bool,"
    "       hasSkipping bool,"
    "       currentSymbol integer,"
    "       pressedSymbol integer,"
    "       score double,"
    "       countPassedSteps integer,"
    "       countSkippedSteps integer,"
    "       countErrors integer"
    "   )");
}

void MainWindow::insert_start() {

    QSqlQuery q("insert into STARTS (dateTime,gameType) values (?,?)");
    q.addBindValue(QDateTime::currentDateTimeUtc());
    q.addBindValue(state_game_type);
    DbTools::execWithCheck(q);
    id_current_game = q.lastInsertId().toUInt();
}

void MainWindow::insert_step(uint asTimeMarker) {

    uint szQu = list_remember_queue.size();
    const auto& lev = map_stat_levels[szQu];

    QSqlQuery q;
    q.prepare(
    "insert into STEPS ("
    "   idStart, dateTime, queueLength, hasError, hasSkipping,"
    "   currentSymbol, pressedSymbol, score,"
    "   countPassedSteps, countSkippedSteps, countErrors,"
    "   asTimeMarker"
    "   ) values (?,?,?,?,?,?,?,?,?,?,?,?)");

    q.addBindValue(id_current_game);
    q.addBindValue(QDateTime::currentDateTimeUtc());
    q.addBindValue(szQu);
    q.addBindValue(state_error);
    q.addBindValue(state_skipping);
    q.addBindValue(current_symbol.unicode());
    q.addBindValue(pressed_symbol.unicode());
    q.addBindValue(lev.score);
    q.addBindValue(lev.c_passed_steps);
    q.addBindValue(lev.c_skipped_steps);
    q.addBindValue(lev.c_errors);
    q.addBindValue(asTimeMarker);
    DbTools::execWithCheck(q);
}
