
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>

#include "pathAppData.h"

QString textQueryResuls() {
    return

    "select"
    "  dateTime(t1.dateTime, 'localtime') as dateTime,"
    "  t2.countPassedSteps as stepsForRew,"
    "  t2.countErrors,"
    "  round(t2.probability, 3) as rew,"
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
    "  and "
    "  t2.countPassedSteps >= :minCount"

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
    qu.exec();

    model_results.setQuery(qu);
}

void MainWindow::initTableResults() {

    auto* tb = ui->tb_results;

    tb->setModel(&model_results);
    setQueryResults();

    tb->setColumnWidth(0, 200);
}

void MainWindow::initDatabase() {

    const QString driver = "QSQLITE";

    if (!QSqlDatabase::isDriverAvailable(driver))
        return;

    auto db = QSqlDatabase::addDatabase(driver);

    QString s = pathAppData() + "/database.sqlite";
    ui->lb_AppData->setText(s);
    db.setDatabaseName(s);

    if (!db.open())
        return;

    QSqlQuery(
    "create table if not exists"
    "   STARTS ("
    "       id integer primary key,"
    "       dateTime text"
    ")");

    QSqlQuery(
    "create table if not exists"
    "   STEPS ("
    "       id integer primary key,"
    "       idStart integer not null,"
    "       dateTime text,"
    "       queueLength integer,"
    "       hasError bool,"
    "       hasSkipping bool,"
    "       currentSymbol integer,"
    "       pressedSymbol integer,"
    "       probability double,"
    "       countPassedSteps integer,"
    "       countSkippedSteps integer,"
    "       countErrors integer"
    ")");
}

void MainWindow::insert_start() {
    QSqlQuery q("insert into STARTS (dateTime) values (?)");
    q.addBindValue(QDateTime::currentDateTimeUtc());
    q.exec();
    id_current_game = q.lastInsertId().toUInt();
}

void MainWindow::insert_step() {

    uint szQu = list_remember_queue.size();
    auto& lev = map_stat_levels.at(szQu);

    QSqlQuery q(
    "insert into STEPS ("
    "   idStart,dateTime,queueLength,hasError,hasSkipping,"
    "   currentSymbol, pressedSymbol, probability, "
    "   countPassedSteps, countSkippedSteps, countErrors "
    "   ) values (?,?,?,?,?,?,?,?,?,?,?)");

    q.addBindValue(id_current_game);
    q.addBindValue(QDateTime::currentDateTimeUtc());
    q.addBindValue(szQu);
    q.addBindValue(state_error);
    q.addBindValue(state_skipping);
    q.addBindValue(current_symbol.unicode());
    q.addBindValue(pressed_symbol.unicode());
    q.addBindValue(lev.probability);
    q.addBindValue(lev.c_passed_steps);
    q.addBindValue(lev.c_skipped_steps);
    q.addBindValue(lev.c_errors);
    q.exec();
}
