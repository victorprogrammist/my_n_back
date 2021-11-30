#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlQueryModel>
#include <QSqlQuery>

#include <set>

using std::vector;
using std::map;
using std::set;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void keyPressEvent(QKeyEvent* event) override;
    void fix_pressed_key(QChar sy);
    void recalc_probability();
    void show_stat_info();
    void make_new_random_key();

    void initDatabase();
    void insert_start();
    void insert_step();

    bool state_play = false;
    bool state_error = false;
    bool state_skipping = false;
    QChar current_symbol;
    QChar pressed_symbol;
    uint cntr_wait_for_hidding = 0;

    vector<QChar> list_remember_queue;
    set<uint> steps_for_skipping;
    uint index_current_step = 0;
    uint count_generated_steps = 0;

    uint id_current_game = 0;

    struct LevCntr {
        uint c_passed_steps = 0;
        uint c_skipped_steps = 0; // символы, которые были показаны после ошибки - не засчитываются
        uint c_errors = 0;
        double probability = 0;
    };

    map<uint,LevCntr> map_stat_levels;

    void setQueryResults();
    void initTableResults();

    QSqlQueryModel model_results;
    QSqlQuery query_results;
    uint m_current_level = 1;

private slots:
    void on_bt_start_clicked();

    void hide_new_key_after_delay();
    void show_new_key_after_delay();

    void on_bt_stop_clicked();

    void on_bt_increase_clicked();

    void on_bt_continue_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
