
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

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
    return pathAppData()+"/log-2.log";
}

static const QString& abc_english() {
    static QString r =
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "=-[];',."
            ;
    return r;
}

static const QString& abc_russian() {
    static QString r =
            "йцукенгшщзхъфывапролджэячсмитьбю."
            "0123456789-="
            ;
    return r;
}

static const QString& abc_numbers() {
    static QString r = "0123456789";
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

    state_game_type = 1;
    ui->rb_english->setChecked(true);
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

    new QShortcut(QKeySequence(Qt::Key_Tab), this,
        this, &MainWindow::change_game_type);

    connect(ui->spbox_level, QOverload<int>::of(&QSpinBox::valueChanged),
    [this](uint level) { m_current_level = level; setQueryResults(); });

    connect(ui->spbox_minCount, QOverload<int>::of(&QSpinBox::valueChanged),
    [this](uint cnt) { m_current_minCount = cnt; setQueryResults(); });

    QTimer* tm = new QTimer(this);
    connect(tm, &QTimer::timeout, this, &MainWindow::timeout_waiting_keypress);
    tm->start(100);

    connect(ui->rb_english, &QPushButton::clicked,
        [this] { after_change_rb_game_type(); });
    connect(ui->rb_russian, &QPushButton::clicked,
        [this] { after_change_rb_game_type(); });
    connect(ui->rb_arithmetic, &QPushButton::clicked,
        [this] { after_change_rb_game_type(); });
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
        fix_pressed_key(QChar(0));
}

void MainWindow::recalc_score() {

    uint szQu = list_remember_queue.size();
    LevCntr& lev = map_stat_levels[szQu];

    uint n = lev.c_passed_steps;
    uint e = lev.c_errors;

    if (!n)
        lev.score = 0;
    else
        lev.score =
            bernoulli_integral_inverse(
                n - e, n, 0.1);
}

void MainWindow::show_stat_info() {

    QStringList list_info;

    list_info.append("\nCURRENT_LEVEL: "+QString::number(list_remember_queue.size()));

    for (auto& pr : map_stat_levels) {
        uint szQu = pr.first;
        LevCntr& m = pr.second;

        if (!m.c_passed_steps)
            continue;

        QString s =
                " type:" + QString::number(state_game_type)+
                " lev:" + QString::number(szQu)+
                " er:"+QString::number(m.c_errors)+
                " stp:"+QString::number(m.c_passed_steps)+
                " skip:"+QString::number(m.c_skipped_steps)+
                " score: "+QString::number(m.score, 'f', 4);

        list_info.append(s);
    }

    QString info = list_info.join("\n");
    file_log(path_file_log(), info);

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

const QString& MainWindow::current_abc() const {

    if (state_game_type <= 1)
        return abc_english();
    else if (state_game_type <= 2)
        return abc_russian();
    else
        return abc_numbers();
}

QChar MainWindow::get_current_symbol() const {

    if (state_game_type <= 2)
        return list_remember_queue.front();

    uint zero = QChar('0').unicode();

    uint sum = 0;
    for (const QChar& sy: list_remember_queue)
        sum += sy.unicode() - zero;

    QString s = QString::number(sum);
    sum = 0;
    for (const QChar& sy: s)
        sum += sy.unicode() - zero;

    return QChar( (sum % 10) + zero );
}

void MainWindow::make_new_random_symbol() {

    const QString& _abc = current_abc();

    uint index_new_symbol = rand() % _abc.size();
    QChar sy = _abc.at(index_new_symbol);
    list_remember_queue.push_back(sy);
    ++count_generated_keys;

    file_log(path_file_log(), QString("NEW_RAND_SYMBOL:")+sy);

    dt_start_waiting_keyPress = QDateTime::currentDateTimeUtc();

    QTimer::singleShot(200, this, SLOT(show_new_key_after_delay()));
}

void MainWindow::fix_pressed_key(QChar sy) {

    pressed_symbol = sy;

    uint szQu = list_remember_queue.size();

    current_symbol = get_current_symbol();

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

    recalc_score();
    insert_step(0);

    if (!state_error) {
        ++index_pressed_true_keys;
        list_remember_queue.erase(list_remember_queue.begin());
        make_new_random_symbol();
        return;
    }

    if (state_game_type <= 2)
        count_pressed_for_skip = index_pressed_true_keys + szQu;
    else
        count_pressed_for_skip = index_pressed_true_keys + 1;

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
    if (!current_abc().contains(sy))
        return;

    fix_pressed_key(sy);
}

void MainWindow::clicked_bt_start_clicked()
{
    if (state_play)
        return;

    if (ui->tabWidget->currentWidget() != ui->tab_main)
        return;

    ui->bt_stop->setEnabled(true);
    ui->bt_start->setEnabled(false);
    ui->bt_increase->setEnabled(true);
    ui->bt_continue->setEnabled(false);
    ui->bt_exit->setEnabled(false);
    ui->wg_game_type->setEnabled(false);

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
    file_log(path_file_log(), "=== START ===");

    make_new_random_symbol();

    insert_start();
    insert_step(1);
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
    ui->wg_game_type->setEnabled(true);
}

void MainWindow::clicked_bt_increase_clicked()
{
    if (state_play && !state_error) {
        make_new_random_symbol();
        insert_step(2);
    }
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

    insert_step(3);
}

void MainWindow::after_change_rb_game_type() {
    if (state_play) return;

    if (ui->rb_english->isChecked())
        state_game_type = 1;

    else if (ui->rb_russian->isChecked())
        state_game_type = 2;

    else if (ui->rb_arithmetic->isChecked())
        state_game_type = 3;

    else {
        state_game_type = 1;
        ui->rb_english->setChecked(true);
    }

    setQueryResults();
}

void MainWindow::change_game_type() {
    if (state_play) return;

    auto fn = [this](auto* rb1, auto* rb2) -> bool {
        if (!rb1->isChecked()) return false;
        rb1->setChecked(false);
        rb2->setChecked(true);
        after_change_rb_game_type();
        return true;
    };

    auto* rb1 = ui->rb_english;
    auto* rb2 = ui->rb_russian;
    auto* rb3 = ui->rb_arithmetic;

    if (!fn(rb1, rb2))
        if (!fn(rb2, rb3))
            if (!fn(rb3, rb1))
                after_change_rb_game_type();
}



