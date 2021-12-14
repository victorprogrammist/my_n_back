#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
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
    void recalc_score();
    void show_stat_info();
    void make_new_random_symbol();

    void initDatabase();
    void insert_start();
    void insert_step(uint asTimeMarker);

    const QString& current_abc() const;
    QChar get_current_symbol() const;

    bool state_play = false;
    bool state_error = false;
    bool state_skipping = false;
    uint state_game_type = 0;
    QChar current_symbol;
    QChar pressed_symbol;
    uint cntr_wait_for_hidding = 0;
    QDateTime dt_start_waiting_keyPress;

    vector<QChar> list_remember_queue;
    uint index_pressed_true_keys = 0;
    uint count_generated_keys = 0;
    uint count_pressed_for_skip = 0; // because it was showed

    uint id_current_game = 0;

    struct LevCntr {
        uint c_passed_steps = 0;
        uint c_skipped_steps = 0; // символы, которые были показаны после ошибки - не засчитываются
        uint c_errors = 0;
        double score = 0;
    };

    map<uint,LevCntr> map_stat_levels;

    void setQueryResults();
    void initTableResults();

    QSqlQueryModel model_results;
    QSqlQuery query_results;
    uint m_current_level;
    uint m_current_minCount;

private slots:
    void clicked_bt_start_clicked();

    void hide_new_key_after_delay();
    void show_new_key_after_delay();
    void timeout_waiting_keypress();
    void change_game_type();
    void after_change_rb_game_type();

    void clicked_bt_stop_clicked();

    void clicked_bt_increase_clicked();

    void clicked_bt_continue_after_error_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
