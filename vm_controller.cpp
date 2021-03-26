#include "vm_controller.h"

#include <QDebug>
#include <QDateTime>

VMController::VMController(QObject *parent)
    : QSerialPort(parent)
{
    this->setBaudRate(QSerialPort::Baud115200);
    this->setDataBits(QSerialPort::Data8);
    this->setParity(QSerialPort::NoParity);
    this->setStopBits(QSerialPort::OneStop);
    this->setFlowControl(QSerialPort::NoFlowControl);

    // initialize single shot timer
    tmr_wait_receive_ = new QTimer(this);
    tmr_wait_receive_->setInterval(3000);
    tmr_wait_receive_->setSingleShot(true);

    // initialize signals and slots
    connect(tmr_wait_receive_, SIGNAL(timeout()), this, SLOT(receiveTimeout()));
    connect(this, SIGNAL(readyRead()), this, SLOT(rxDataReady()));

    // initialize control state
    state_ = IDLE;
}

VMController::~VMController()
{

}

void VMController::setReceiveTimeout(int timeout)
{
    tmr_wait_receive_->setInterval(timeout * 1000);
}

bool VMController::getFirmwareInfos()
{
    read_fw_info_retry_ = 0;
    qDebug() << "[VMC] get firmware information start...";

    // check serial port is opened
    if (isOpened() == false) {
        qDebug() << "[VMC] device open failed";
        return false;
    }

    // create tx data
    QByteArray tx_data;
    tx_data.append(CMD_INFO);
    tx_data.append('\n');

    // change control state
    state_ = WAIT_FW_INFO;

    // set expected length of rx data
    rx_expected_len_ = RX_LEN_INFO;

    // write data
    this->clear();
    this->write(tx_data);
    this->flush();
    //this->waitForBytesWritten(3000);

    // start receive timeout timer
    tmr_wait_receive_->start();
    return true;
}

bool VMController::getTemperatureStatus()
{
    qDebug() << "[VMC] get temperature status start...";

    // check serial port is opened
    if (isOpened() == false) {
        qDebug() << "[VMC] device open failed";
        return false;
    }

    // create tx data
    QByteArray tx_data;
    tx_data.append(CMD_TPAL);
    tx_data.append('\n');

    qDebug() << "[VMC] TX data:" << tx_data;

    // change control state
    state_ = WAIT_TP_INFO;

    // write data
    this->clear();
    this->write(tx_data);
    this->flush();
    //this->waitForBytesWritten(3000);

    // start reading timeout timer
    rx_expected_len_ = RX_LEN_TPAL;
    tmr_wait_receive_->start();
    return true;
}

bool VMController::setCompressorSwitch(bool on_off)
{
    qDebug() << "[VMC] set compressor switch start...";

    // check serial port is opened
    if (isOpened() == false) {
        qDebug() << "[VMC] device open failed";
        return false;
    }

    // create tx data
    QByteArray tx_data;
    QByteArray rx_data;
    tx_data.append((on_off)? CMD_CPON : CMD_CPOFF);
    tx_data.append('\n');

    // change control state
    state_ = WAIT_CP_ONOFF;

    // set expected length of rx data
    rx_expected_len_ = RX_LEN_CP_ONOFF;

    // write data and wait for ready read
    if (writeAndWaitForReadyRead(tx_data, rx_expected_len_, &rx_data)) {

        // emit response
        emit setCompressorSwitchResponse((QString::fromUtf8(rx_data).indexOf("OK") > 0));
        state_ = IDLE;
    }
    else {
        // start receive timeout timer
        tmr_wait_receive_->start();
    }
    return true;
}

bool VMController::setDoorSwitch(int numbering, bool on_off)
{
    qDebug() << "[VMC] set door" << numbering << "switch start...";

    // check serial port is opened
    if (isOpened() == false) {
        qDebug() << "[VMC] device open failed";
        return false;
    }

    // create tx data
    QByteArray tx_data;
    tx_data.append('C');
    tx_data.append(QString::number(numbering).toUtf8());
    tx_data.append((on_off)? "ON" : "OF");
    tx_data.append('\n');

    qDebug() << "[VMC] TX data:" << tx_data;

    // change control state
    state_ = WAIT_DR_ONOFF;

    // write data
    this->clear();
    this->write(tx_data);
    this->flush();
    //this->waitForBytesWritten(3000);

    // start reading timeout timer
    rx_expected_len_ = RX_LEN_DR_ONOFF;
    tmr_wait_receive_->start();
    return true;
}

bool VMController::executeChannel(int ch1_row, int ch1_col, int ch2_row, int ch2_col)
{
    exe_channel_retry_ = 0;
    qDebug() << "[VMC] execute channel" << ch1_row << ch1_col << ch2_row << ch2_col << "start...";

    // check serial port is opened
    if (isOpened() == false) {
        qDebug() << "[VMC] device open failed";
        return false;
    }

    // create tx data
    QByteArray tx_data;
    if (ch2_row < 0 || ch2_col < 0) {
        is_single_ = true;
        tx_data.append("CH");
        tx_data.append(QString::number(ch1_row).toUtf8());
        tx_data.append(QString::number(ch1_col).toUtf8());
        tx_data.append('\n');
        rx_expected_len_ = RX_LEN_CH_SINGLE;
    }
    else {
        is_single_ = false;
        tx_data.append("D");
        tx_data.append(QString::number(ch1_row).toUtf8());
        tx_data.append(QString::number(ch1_col).toUtf8());
        tx_data.append(QString::number(ch2_row).toUtf8());
        tx_data.append(QString::number(ch2_col).toUtf8());
        tx_data.append('\n');
        rx_expected_len_ = RX_LEN_CH_COMBINE;
    }

    qDebug() << "[VMC] TX data:" << tx_data;

    // change control state
    state_ = WAIT_CH_OK;

    // write data
    this->clear();
    this->write(tx_data);
    this->flush();
    //this->waitForBytesWritten(3000);

    // start reading timeout timer
    tmr_wait_receive_->start(120 * 1000);
    return true;
}

bool VMController::executeChannelRetry()
{
    exe_channel_retry_++;
    qDebug() << "[VMC] execute channel retry..." << exe_channel_retry_;

    // check serial port is opened
    if (isOpened() == false) {
        qDebug() << "[VMC] device open failed";
        return false;
    }

    // create tx data
    QByteArray tx_data;
    tx_data.append(CMD_CHRT);
    tx_data.append('\n');

    qDebug() << "[VMC] TX data:" << tx_data;

    // change control state
    state_ = WAIT_CH_RETRY;

    // write data
    this->clear();
    this->write(tx_data);
    this->flush();
    //this->waitForBytesWritten(3000);

    // start reading timeout timer
    rx_expected_len_ = RX_LEN_CH_RETRY;
    tmr_wait_receive_->start(120 * 1000);
    return true;
}

bool VMController::checkCargoState()
{
    qDebug() << "[VMC] check cargo state...";

    // check serial port is opened
    if (isOpened() == false) {
        qDebug() << "[VMC] device open failed";
        return false;
    }

    // create tx data
    QByteArray tx_data;
    tx_data.append(CMD_CARS);
    tx_data.append('\n');

    qDebug() << "[VMC] TX data:" << tx_data;

    // change control state
    state_ = WAIT_CARS;

    // write data
    this->clear();
    this->write(tx_data);
    this->flush();
    //this->waitForBytesWritten(3000);

    // start reading timeout timer
    rx_expected_len_ = RX_LEN_CARS;
    tmr_wait_receive_->start(30 * 1000);
    return true;
}

bool VMController::checkDoorState()
{
    qDebug() << "[VMC] check door state...";

    // check serial port is opened
    if (isOpened() == false) {
        qDebug() << "[VMC] device open failed";
        return false;
    }

    // create tx data
    QByteArray tx_data;
    tx_data.append(CMD_CDOS);
    tx_data.append('\n');

    qDebug() << "[VMC] TX data:" << tx_data;

    // change control state
    state_ = WAIT_CDOS;

    // write data
    this->clear();
    this->write(tx_data);
    this->flush();

    // start reading timeout timer
    rx_expected_len_ = RX_LEN_CDOS;
    tmr_wait_receive_->start(30 * 1000);
    return true;
}

void VMController::receiveTimeout()
{
    if (state_ == WAIT_FW_INFO && read_fw_info_retry_ < 10) {
        read_fw_info_retry_++;
        qDebug() << "[VMC] get firmware information timeout" << read_fw_info_retry_;

        QByteArray tx_data;
        tx_data.append(CMD_INFO);
        tx_data.append('\n');

        this->write(tx_data);
        this->flush();
        tmr_wait_receive_->start();
    }
    else {
        QString err_message = "[VMC] Timeout: ";
        err_message.append(this->readAll());

        emit timeoutWithState(err_message, state_);
        state_ = ERROR_TIMEOUT;
        this->clear();
    }
}

void VMController::rxDataReady()
{
    // wait until the expected length is ready
    if (this->bytesAvailable() < rx_expected_len_) {
        return;
    }

    // stop reading timeout timer
    tmr_wait_receive_->stop();

    // read all data
    QByteArray rx_data = this->readAll();
    QString sz_response = QString::fromUtf8(rx_data);
    qDebug() << "[VMC] RX data:" << rx_data;

    // parse data according current state
    switch (state_) {

    case WAIT_FW_INFO:
        emit getFirmwareInfosResponse(sz_response);
        state_ = IDLE;
        break;

    case WAIT_TP_INFO:
        emit getTemperatureStatusResponse(sz_response);
        state_ = IDLE;
        break;

    case WAIT_CP_ONOFF:
        emit setCompressorSwitchResponse((sz_response.indexOf("OK") > 0));
        state_ = IDLE;
        break;

    case WAIT_DR_ONOFF:
        emit setDoorSwitchResponse((sz_response.indexOf("OK") > 0));
        state_ = IDLE;
        break;

    case WAIT_CH_OK:
        if (sz_response.indexOf("OK") < 0) {
            state_ = ERROR_TIMEOUT;
            emit executeChannelResponse(false, state_);
        }
        else {
            state_ = WAIT_CH_OP;
            rx_expected_len_ = 6;
            tmr_wait_receive_->start(120 * 1000);
        }
        break;

    case WAIT_CH_OP:
        if (sz_response.indexOf("OP") < 0) {
            if (sz_response.indexOf("DOOREE") >= 0) {
                state_ = ERROR_DOOR;
            }
            else {
                QString error_code = (is_single_)? QString(rx_data.mid(4)) : QString(rx_data.mid(5));
                if (error_code == "EE02") {
                    state_ = ERROR_DROP;
                }
                else if (error_code == "EE03") {
                    state_ = ERROR_CARGO;
                }
            }
            emit executeChannelResponse(false, state_);
        }
        else {
            state_ = WAIT_CH_DONE;
            rx_expected_len_ = 8;
            emit executeChannelResponse(true, state_);
            tmr_wait_receive_->start(120 * 1000);
        }
        break;

    case WAIT_CH_DONE:
        if (sz_response.indexOf("DO") < 0) {
            QString error_code = (is_single_)? QString(rx_data.mid(4)) : QString(rx_data.mid(5));
            if (error_code == "EE01") {
                state_ = WARNING_NOT_PICKUP;
            }
            else if (error_code == "EE04") {
                state_ = ERROR_DOOR;
            }
            emit executeChannelResponse(false, state_);
        }
        else {
            state_ = WAIT_CARS;
            emit executeChannelResponse(true, state_);
        }
        break;

    case WAIT_CH_RETRY:
        if (sz_response == "CHRT\r\n") {
            tmr_wait_receive_->start(120 * 1000);
        }
        else if (sz_response.indexOf("DO") < 0) {
            QString error_code = rx_data.mid(4);
            if (error_code == "EE") {
                state_ = WARNING_NOT_PICKUP;
            }
            else if (error_code == "NO") {
                state_ = ERROR_DOOR;
            }
            emit executeChannelResponse(false, state_);
        }
        else {
            state_ = WAIT_CARS;
            emit executeChannelResponse(true, state_);
        }
        break;

    case WAIT_CARS:
        if (sz_response == "CARSOK") {
            tmr_wait_receive_->start(120 * 1000);
        }
        else if (sz_response.indexOf("CAGOOK") < 0) {
            state_ = WARNING_NOT_PICKUP;
            emit executeChannelResponse(false, state_);
        }
        else {
            state_ = WAIT_CDOS;
            emit executeChannelResponse(true, state_);
        }
        break;

    case WAIT_CDOS:
        if (sz_response == "CDOSOK") {
            tmr_wait_receive_->start(120 * 1000);
        }
        else if (sz_response.indexOf("DOOREE") >= 0) {
            state_ = ERROR_DOOR;
            emit executeChannelResponse(false, state_);
        }
        else {
            state_ = IDLE;
            emit executeChannelResponse(true, state_);
        }
        break;

    default:
        break;
    }

    // clear tx and rx data
    this->clear();
}

bool VMController::isOpened()
{
    if (this->isOpen() == false) {
        this->open(QIODevice::ReadWrite);
    }
    return this->isOpen();
}

bool VMController::writeAndWaitForReadyRead(QByteArray tx_data, int expected_len, QByteArray *rx_data)
{
    // disconnect temporarily for blocking waiting
    disconnect(this, SIGNAL(readyRead()), this, SLOT(rxDataReady()));

    qDebug() << "[VMC] TX data:" << tx_data;

    // write data
    this->clear();
    this->write(tx_data);
    this->flush();
    //this->waitForBytesWritten(3000);

    // wait and read all data
    this->waitForReadyRead(3000);
    rx_data->append(this->readAll());

    int count = 0;
    while (rx_data->length() < expected_len && count < 30) {
        this->waitForReadyRead(100);
        rx_data->append(this->readAll());
        count++;
    }

    qDebug() << "[VMC] RX data:" << rx_data->data();

    // recover signal and slot connection
    connect(this, SIGNAL(readyRead()), this, SLOT(rxDataReady()));

    // check data is valid
    if (rx_data->length() != expected_len) {
        qDebug() << "[VMC] RX data's length is invalid" << rx_data->length();
        return false;
    }
    return true;
}
