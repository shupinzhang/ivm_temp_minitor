#include "main_window.h"
#include "ui_main_window.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTime>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QProcess>
#include <QDebug>

#include "cms_api.h"
#include "vm_controller.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    tmr_auto_send_ = new QTimer(this);
    tmr_auto_send_->setInterval(ui->spinBox_auto_interval->value() * 60 * 1000);
    connect(tmr_auto_send_, SIGNAL(timeout()), this, SLOT(vmc_send()));

    tmr_pulling_watch_ = new QTimer(this);
    tmr_pulling_watch_->setInterval(10 * 1000);
    connect(tmr_pulling_watch_, SIGNAL(timeout()), this, SLOT(watch_pulling()));

    // initialize comboBox
    foreach (QSerialPortInfo port_info, QSerialPortInfo::availablePorts()) {
        ui->cbBox_port->addItem(port_info.portName());
    }

    // initialize other widgets
    ui->pbtn_open->setText("Open");
    ui->pbtn_send->setEnabled(false);
    ui->lineEdit_cmd->setEnabled(false);
    connect(ui->pbtn_open, SIGNAL(clicked()), this, SLOT(pbtn_open_clicked()));
    connect(ui->pbtn_send, SIGNAL(clicked()), this, SLOT(pbtn_send_clicked()));
    connect(ui->pbtn_clear, SIGNAL(clicked()), this, SLOT(pbtn_clear_clicked()));
    connect(ui->pbtn_set, SIGNAL(clicked()), this, SLOT(pbtn_set_clicked()));
    connect(ui->spinBox_auto_interval, SIGNAL(valueChanged(int)), this, SLOT(spinBox_valueChanged(int)));

    // create cms api object
    cms_api_ = new CmsApi(this);

    // initialize external device
    vm_controller_ = new VMController(this);
    connect(vm_controller_, SIGNAL(readyRead()), this, SLOT(vmc_ready_read()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pbtn_open_clicked()
{
    // do nothing when serial port is invalid
    if (ui->cbBox_port->currentIndex() < 0) {
        return;
    }

    // open the chosen serial port
    if (ui->pbtn_open->text() == "Open") {

        // set the properties of serial port
        vm_controller_->setPortName(ui->cbBox_port->currentText());

        // change the status of widgets when serial port is opened
        if (vm_controller_->open(QIODevice::ReadWrite)) {
            vm_controller_->setRequestToSend(true);
            ui->pbtn_open->setText("Close");
            ui->pbtn_send->setEnabled(true);
            ui->chBox_auto_enable->setEnabled(false);
            ui->spinBox_auto_interval->setEnabled(false);
            ui->lineEdit_cmd->setEnabled(true);
        }
    }
    // close the chosen serial port
    else {
        tmr_auto_send_->stop();

        vm_controller_->clear();
        vm_controller_->close();

        // change the status of widgets when serial port is closed
        ui->pbtn_open->setText("Open");
        ui->pbtn_send->setEnabled(false);
        ui->chBox_auto_enable->setEnabled(true);
        ui->spinBox_auto_interval->setEnabled(true);
        ui->lineEdit_cmd->clear();
        ui->lineEdit_cmd->setEnabled(false);
    }
}

void MainWindow::pbtn_send_clicked()
{
    if (vm_controller_->isOpen()) {

        vmc_send();

        if (ui->chBox_auto_enable->isChecked()) {
            tmr_auto_send_->start();
            ui->pbtn_send->setEnabled(false);
            ui->lineEdit_cmd->setEnabled(false);
        }
    }
}

void MainWindow::pbtn_clear_clicked()
{
    ui->textBrowser->clear();
}

void MainWindow::spinBox_valueChanged(int value)
{
    tmr_auto_send_->setInterval(value * 60 * 1000);
}

void MainWindow::vmc_send()
{
    // create tx data
    QByteArray tx_data;
    tx_data.append(ui->lineEdit_cmd->text().toUtf8());
    tx_data.append('\n');

    // update textBrowser
    ui->textBrowser->append(QString("%1 TX: %2").arg(QTime::currentTime().toString("hh:mm:ss"))
                                                .arg(ui->lineEdit_cmd->text()));

    // send tx data
    vm_controller_->clear();
    vm_controller_->write(tx_data);
    vm_controller_->waitForBytesWritten(500);
}

void MainWindow::vmc_ready_read()
{
    // read all data
    QByteArray rx_data = vm_controller_->readAll();

    // update textBrowser
    ui->textBrowser->append(QString("%1 RX(%2): %3").arg(QTime::currentTime().toString("hh:mm:ss"))
                                                    .arg(QString::number(rx_data.length()))
                                                    .arg(QString::fromUtf8(rx_data)));

    QString today = QDateTime::currentDateTime().toString("yyyyMMdd");

    QFile log_file;
    log_file.setFileName(QString("%1/ivm_temp_%2.txt").arg(dir_log).arg(today));

    // open the file logging
    if (log_file.exists() == false) {
        log_file.open(QFile::ReadWrite | QFile::Text);
    }
    else {
        log_file.open(QFile::Append | QFile::Text);
    }

    // open stream file writes
    QTextStream out(&log_file);

    // write the date of recording
    out << QDateTime::currentDateTime().toString("hh:mm:ss")
        << ", TP01 " << rx_data.mid( 4, 5)
        << ", TP02 " << rx_data.mid(13, 5)
        << ", TP03 " << rx_data.mid(22, 5)
        << ", TP04 " << rx_data.mid(31, 5)
        << ", TP05 " << rx_data.mid(40, 5)
        << ", TP06 " << rx_data.mid(49, 5)
        << ", TP07 " << rx_data.mid(58, 5)
        << ", TP08 " << rx_data.mid(67, 5) << "\n";

    // clear the buffered data
    out.flush();
    log_file.close();

    // update to cloud
    QMap<QString, QByteArray> infos;
    infos.insert("temperature_1", rx_data.mid( 4, 5));
    infos.insert("temperature_2", rx_data.mid(13, 5));
    infos.insert("temperature_3", rx_data.mid(22, 5));
    infos.insert("temperature_4", rx_data.mid(31, 5));
    infos.insert("temperature_5", rx_data.mid(40, 5));
    infos.insert("temperature_6", rx_data.mid(49, 5));
    infos.insert("temperature_7", rx_data.mid(58, 5));
    infos.insert("temperature_8", rx_data.mid(67, 5));
    infos.insert("cp",            rx_data.mid(74, 1));
    infos.insert("fn",            rx_data.mid(77, 1));
    infos.insert("door",          rx_data.mid(80, 1));

    if (machine_code_.isEmpty())
        qDebug()<< "Pls set machine code";
    else {
        cms_api_->updateMonitoringInfos(machine_code_, infos);
    }
}

void MainWindow::watch_pulling()
{
    if (!machine_code_.isEmpty()) {
        QByteArray cmds;
        bool result = cms_api_->getRemoteCommand(machine_code_, &cmds);
        if (result == false) {
            qDebug() << "[PULLING] pulling cmd: failed.";
            return;
        }

        // parser command
        QJsonDocument intdata = QJsonDocument::fromJson(cmds);
        QJsonObject intobj = intdata.object();
        QString pulling_cmd = intobj.value("cmd_code").toString();

        qDebug() << "[PULLING] pulling cmd:" << pulling_cmd;

        // skip empty command
        if (pulling_cmd.isEmpty()) {
            return;
        }

        // stop pulling timer
        tmr_pulling_watch_->stop();

        // reboot request
        if (pulling_cmd == PCMD_REBOOT_REQUEST ) {
            qDebug() << "[PULLING] IPC Reboot Request";
            QThread::sleep(5);
            QProcess::execute("sudo reboot");
            return;
        }

        // normal request
        if (pulling_cmd == PCMD_SET_COMPRESSOR_ON) {
            if (vm_controller_ != nullptr)
                vm_controller_->setCompressorSwitch(true);
        }
        else if (pulling_cmd == PCMD_SET_COMPRESSOR_OFF) {
            if (vm_controller_ != nullptr)
                vm_controller_->setCompressorSwitch(false);
        }

        // start pulling timer
        tmr_pulling_watch_->start();
    }
}

void MainWindow::pbtn_set_clicked()
{
    machine_code_ = ui->lineEdit_machine_code->text();
    tmr_pulling_watch_->start();
}
