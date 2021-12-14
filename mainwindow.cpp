
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

    ui->tabWidget->setCurrentWidget(ui->tab_main);

    initDatabase();

    m_current_level = 2;
    m_current_minCount = 4;

    ui->spbox_level->setValue(m_current_level);
    ui->spbox_minCount->setValue(m_current_minCount);
    initTableResults();

    state_play = true;
    clicked_bt_stop_clicked();

    ui->lb_symb->setText("");
    ui->lb_info->setText("");

    auto fnQuit = [this] { QApplication::quit(); };

    new QShortcut(QKeySequence(Qt::Key_Escape), this, fnQuit);
    connect(ui->bt_exit, &QPushButton::clicked, fnQuit);

    new QShortcut(QKeySequence(Qt::Key_Backspace), this,
        this, &MainWindow::clicked_bt_stop_clicked);
    connect(ui->bt_stop, &QPushButton::clicked,
        this, &MainWindow::clicked_bt_stop_clicked);

    new QShortcut(QKeySequence(Qt::Key_Space), this,
        this, &MainWindow::clicked_bt_start_clicked);
    connect(ui->bt_start, &QPushButton::clicked,
        this, &MainWindow::clicked_bt_start_clicked);

    new QShortcut(QKeySequence(Qt::Key_PageUp), this,
        this, &MainWindow::clicked_bt_increase_clicked);
    connect(ui->bt_increase, &QPushButton::clicked,
        this, &MainWindow::clicked_bt_increase_clicked);

    new QShortcut(QKeySequence(Qt::Key_Enter), this,
        this, &MainWindow::clicked_bt_continue_after_error_clicked);
    new QShortcut(QKeySequence(Qt::Key_Return), this,
        this, &MainWindow::clicked_bt_continue_after_error_clicked);
    connect(ui->bt_continue, &QPushButton::clicked,
        this, &MainWindow::clicked_bt_continue_after_error_clicked);

    connect(ui->spbox_level, QOverload<int>::of(&QSpinBox::valueChanged),
    [this](uint level) { m_current_level = level; setQueryResults(); });

    connect(ui->spbox_minCount, QOverload<int>::of(&QSpinBox::valueChanged),
    [this](uint cnt) { m_current_minCount = cnt; setQueryResults(); });

    QTimer* tm = new QTimer(this);
    connect(tm, &QTimer::timeout, this, &MainWindow::timeout_waiting_keypress);
    tm->start(100);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::timeout_waiting_keypress() {

    if (!state_play || state_error)
        return;

    QDateTime dt = QDateTime::currentDateTimeUtc();
    qint64 offs = dt_start_waiting_keyPress.msecsTo(dt);

    if (offs >= 20000)
        fix_pressed_key(0);
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

    QStringList list_info;

    list_info.append("CURRENT_LEVEL: "+QString::number(list_remember_queue.size()));
    list_info.append("");

    for (auto& pr : map_stat_levels) {
        uint szQu = pr.first;
        LevCntr& m = pr.second;

        QString s =
                "lev: " + QString::number(szQu)+
                " "+QString::number(m.c_errors)+
                "/"+QString::number(m.c_passed_steps)+
                "/"+QString::number(m.c_skipped_steps)+
                " reward: "+QString::number(m.probability, 'f', 4);

        list_info.append(s);
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

void MainWindow::make_new_random_symbol() {

    uint index_new_symbol = rand() % abc().size();
    QChar sy = abc().at(index_new_symbol);
    list_remember_queue.push_back(sy);
    ++count_generated_keys;

    dt_start_waiting_keyPress = QDateTime::currentDateTimeUtc();

    QTimer::singleShot(200, this, SLOT(show_new_key_after_delay()));
}

void MainWindow::fix_pressed_key(QChar sy) {

    pressed_symbol = sy;

    uint szQu = list_remember_queue.size();

    current_symbol = list_remember_queue.front();

    state_error =
        pressed_symbol != current_symbol
        || pressed_symbol.unicode() == 0;

    state_skipping = false;

    if (!state_error)
        if (index_pressed_true_keys < count_pressed_for_skip)
            state_skipping = true;

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
        ++index_pressed_true_keys;
        list_remember_queue.erase(list_remember_queue.begin());
        make_new_random_symbol();
        return;
    }

    count_pressed_for_skip = index_pressed_true_keys + szQu;

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

void MainWindow::clicked_bt_start_clicked()
{
    if (state_play)
        return;

    ui->bt_stop->setEnabled(true);
    ui->bt_start->setEnabled(false);
    ui->bt_increase->setEnabled(true);
    ui->bt_continue->setEnabled(false);
    ui->bt_exit->setEnabled(false);

    count_generated_keys = 0;
    index_pressed_true_keys = 0;
    count_pressed_for_skip = 0;

    list_remember_queue.clear();
    state_play = true;
    state_error = false;
    state_skipping = false;
    cntr_wait_for_hidding = 0;
    map_stat_levels.clear();

    srand(time(NULL));
    make_new_random_symbol();

    file_log(path_file_log(), "=== START ===");    
    insert_start();
}

void MainWindow::clicked_bt_stop_clicked()
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

void MainWindow::clicked_bt_increase_clicked()
{
    if (state_play && !state_error)
        make_new_random_symbol();
}

void MainWindow::clicked_bt_continue_after_error_clicked()
{
    if (!state_play || !state_error)
        return;

    ui->lb_symb->setText("");
    ui->lb_symb->setStyleSheet("");
    ui->bt_increase->setEnabled(true);
    ui->bt_continue->setEnabled(false);
    state_error = false;
    dt_start_waiting_keyPress = QDateTime::currentDateTimeUtc();
}

