#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTimer>

#ifdef _WIN32
#define dir_log         "D:/Qt Projects/_HillEver/ivm_temp_minitor/log"
#else
#define dir_log         "/home/dev/ivm_temp_minitor/log"
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// forward declaration
class CmsApi;
class VMController;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void pbtn_open_clicked();
    void pbtn_send_clicked();
    void pbtn_clear_clicked();
    void spinBox_valueChanged(int value);
    void vmc_send();
    void vmc_ready_read();
    void watch_pulling();

private:
    Ui::MainWindow *ui;

    QTimer *tmr_auto_send_;
    QTimer *tmr_pulling_watch_;

    // web api manager
    CmsApi *cms_api_;

    // external device
    VMController *vm_controller_;
};
#endif // MAIN_WINDOW_H
