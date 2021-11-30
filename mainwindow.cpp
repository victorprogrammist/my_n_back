
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QKeyEvent>
#include <QTimer>
#include <QShortcut>
#include <QScreen>

#include "pathAppData.h"

double bernoulli_integral_inverse(
        uint k, uint n,
        double p_quantile, bool dir_right_to_left = false,
        double er = 0.00001);

QString app_path();
void file_log(const QString& fn, const QString& s);

static QString path_file_log() {
    return pathAppData()+"/log.log";
}

static const QString& abc() {
    static QString r =
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "=-[];',."
            ;
    return r;
}

static const QString& _abc() {
    static QString r = "abcdefghijklmnopqrstuvwxyz";
    return r;
}

void make();

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto screen = QGuiApplication::primaryScreen();
    auto r1 = screen->availableGeometry();
    auto sz = geometry().size();
    setGeometry(
        r1.width()/2 - sz.width()/2,
        r1.height()/2 - sz.height()/2,
        sz.width(), sz.height());

    initDatabase();
    initTableResults();

    state_play = true;
    on_bt_stop_clicked();

    ui->lb_symb->setText("");
    ui->lb_info->setText("");

    auto fnQuit = [this] { QApplication::quit(); };

    new QShortcut(QKeySequence(Qt::Key_Escape), this, fnQuit);
    connect(ui->bt_exit, &QPushButton::clicked, fnQuit);

    new QShortcut(QKeySequence(Qt::Key_Backspace), this,
        [this] { on_bt_stop_clicked(); });

    new QShortcut(QKeySequence(Qt::Key_Space), this,
        [this] { on_bt_start_clicked(); });

    new QShortcut(QKeySequence(Qt::Key_PageUp), this,
        [this] { on_bt_increase_clicked(); });

    new QShortcut(QKeySequence(Qt::Key_Enter), this,
        [this] { on_bt_continue_clicked(); });

    new QShortcut(QKeySequence(Qt::Key_Return), this,
        [this] { on_bt_continue_clicked(); });

    connect(ui->spbox_level, QOverload<int>::of(&QSpinBox::valueChanged),
    [this](uint level) { m_current_level = level; setQueryResults(); });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::recalc_probability() {

    uint szQu = list_remember_queue.size();
    LevCntr& lev = map_stat_levels[szQu];

    uint n = lev.c_passed_steps;
    uint e = lev.c_errors;

    if (!n)
        lev.probability = 0;
    else
        lev.probability =
            bernoulli_integral_inverse(
                n - e, n, 0.1);
}

void MainWindow::show_stat_info() {

    if (map_stat_levels.empty()) {
        ui->lb_info->setText("---");
        return;
    }

    vector<double> list_rew;

    QStringList list_info;
    for (auto& pr : map_stat_levels) {
        uint szQu = pr.first;
        LevCntr& m = pr.second;

        QString s =
                "lev: " + QString::number(szQu)+
                " "+QString::number(m.c_errors)+
                "/"+QString::number(m.c_passed_steps)+
                "/"+QString::number(m.c_skipped_steps)+
                " rew: "+QString::number(m.probability, 'f', 4);

        list_info.append(s);
        list_rew.insert(list_rew.begin(), m.probability);
    }

    QString info = list_info.join("\n");
    file_log(path_file_log(), "\n"+info);

    ui->lb_info->setText(info);
}

void MainWindow::hide_new_key_after_delay() {

    if (!state_play)
        return;

    if (state_error || !cntr_wait_for_hidding) {
        cntr_wait_for_hidding = 0;
        return;
    }

    if (--cntr_wait_for_hidding == 0)
        ui->lb_symb->setText("");
}

void MainWindow::show_new_key_after_delay() {

    if (!state_play || state_error)
        return;

    QChar sy = list_remember_queue.back();

    ui->lb_symb->setText(sy);

    ++cntr_wait_for_hidding;
    QTimer::singleShot(1000, this, SLOT(hide_new_key_after_delay()));

    show_stat_info();
}

void MainWindow::make_new_random_key() {

    uint index_new_symbol = rand() % abc().size();
    QChar sy = abc().at(index_new_symbol);
    list_remember_queue.push_back(sy);
    ++count_generated_steps;

    QTimer::singleShot(200, this, SLOT(show_new_key_after_delay()));
}

void MainWindow::fix_pressed_key(QChar sy) {

    pressed_symbol = sy;

    uint szQu = list_remember_queue.size();

    current_symbol = list_remember_queue.front();
    state_error = pressed_symbol != current_symbol;

    state_skipping = false;
    if (!state_error)
        state_skipping = steps_for_skipping.count(index_current_step);

    steps_for_skipping.erase(index_current_step);

    file_log(
             path_file_log(),
             "STEP:level:"+QString::number(szQu)
             +",skipping:" + (state_skipping ? "true" : "false")
             +",key_pressed:" +(state_error ? "error" : "success")
             +",pressed:"+pressed_symbol
             +",must_be:"+current_symbol
    );

    LevCntr& lev = map_stat_levels[szQu];

    if (state_skipping)
        ++lev.c_skipped_steps;
    else
        ++lev.c_passed_steps;

    if (state_error)
        ++lev.c_errors;

    recalc_probability();
    insert_step();

    if (!state_error) {
        ++index_current_step;
        list_remember_queue.erase(list_remember_queue.begin());
        make_new_random_key();
        return;
    }

    for (uint ii = 0; ii < szQu; ++ii)
        steps_for_skipping.insert(index_current_step + ii);

    QString msg_symbols = "";
    for (QChar sy: list_remember_queue)
        msg_symbols += sy;

    ui->lb_symb->setText(msg_symbols);
    ui->lb_symb->setStyleSheet(
                "QLabel { background-color : red; }");

    ui->bt_increase->setEnabled(false);
    ui->bt_continue->setEnabled(true);

    show_stat_info();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {

    if (!state_play || state_error)
        return;

    QString s = event->text();
    if (s.size() != 1)
        return;

    QChar sy = s.front();
    if (!abc().contains(sy))
        return;

    fix_pressed_key(sy);
}

void MainWindow::on_bt_start_clicked()
{
    if (state_play)
        return;

    ui->bt_stop->setEnabled(true);
    ui->bt_start->setEnabled(false);
    ui->bt_increase->setEnabled(true);
    ui->bt_continue->setEnabled(false);
    ui->bt_exit->setEnabled(false);

    count_generated_steps = 0;
    index_current_step = 0;

    list_remember_queue.clear();
    state_play = true;
    state_error = false;
    state_skipping = false;
    cntr_wait_for_hidding = 0;
    map_stat_levels.clear();

    srand(time(NULL));
    make_new_random_key();

    file_log(path_file_log(), "=== START ===");    
    insert_start();
}

void MainWindow::on_bt_stop_clicked()
{
    if (!state_play)
        return;

    state_play = false;
    state_error = false;
    state_skipping = false;
    ui->bt_stop->setEnabled(false);
    ui->bt_start->setEnabled(true);
    ui->bt_increase->setEnabled(false);
    ui->bt_continue->setEnabled(false);
    ui->bt_exit->setEnabled(true);
    ui->lb_symb->setText("");
    ui->lb_symb->setStyleSheet("");
}

void MainWindow::on_bt_increase_clicked()
{
    if (state_play && !state_error)
        make_new_random_key();
}

void MainWindow::on_bt_continue_clicked()
{
    if (!state_play || !state_error)
        return;

    ui->lb_symb->setText("");
    ui->lb_symb->setStyleSheet("");
    ui->bt_increase->setEnabled(true);
    ui->bt_continue->setEnabled(false);
    state_error = false;
}

