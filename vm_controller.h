#ifndef VM_CONTROLLER_H
#define VM_CONTROLLER_H

#include <QSerialPort>
#include <QTimer>
#include <QString>

#define CMD_INFO    "VMIF"
#define CMD_TPAL    "TPAL"
#define CMD_CPON    "CPON"
#define CMD_CPOFF   "CPOF"
#define CMD_CHRT    "CHRT"
#define CMD_CARS    "CARS"
#define CMD_CDOS    "CDOS"

#define RX_LEN_INFO         9
#define RX_LEN_TPAL        81
#define RX_LEN_CP_ONOFF     6
#define RX_LEN_DR_ONOFF     6
#define RX_LEN_CH_SINGLE    6
#define RX_LEN_CH_COMBINE   7
#define RX_LEN_CH_RETRY     6
#define RX_LEN_CARS         6
#define RX_LEN_CDOS         6

class VMController : public QSerialPort
{
    Q_OBJECT

public:
    enum State {
        IDLE,
        ERROR_TIMEOUT,
        ERROR_DROP,
        ERROR_CARGO,
        ERROR_DOOR,
        WARNING_NOT_PICKUP,
        WAIT_FW_INFO,
        WAIT_TP_INFO,
        WAIT_CP_ONOFF,
        WAIT_DR_ONOFF,
        WAIT_CH_OK,
        WAIT_CH_OP,
        WAIT_CH_DONE,
        WAIT_CH_RETRY,
        WAIT_CARS,
        WAIT_CDOS
    };

public:
    VMController(QObject *parent = nullptr);
    ~VMController();

    void setReceiveTimeout(int timeout);

    bool getFirmwareInfos();
    bool getTemperatureStatus();
    bool setCompressorSwitch(bool on_off);
    bool setDoorSwitch(int numbering, bool on_off);
    bool executeChannel(int ch1_row, int ch1_col, int ch2_row = -1, int ch2_col = -1);
    bool executeChannelRetry();
    bool checkCargoState();
    bool checkDoorState();

Q_SIGNALS:
    void timeoutWithState(QString err_msg, int state);
    void getFirmwareInfosResponse(QString infos);
    void getTemperatureStatusResponse(QString status);
    void setCompressorSwitchResponse(bool result);
    void setDoorSwitchResponse(bool result);
    void executeChannelResponse(bool result, int state);

private slots:
    void receiveTimeout();
    void rxDataReady();

private:
    bool isOpened();
    bool writeAndWaitForReadyRead(QByteArray tx_data, int expected_len, QByteArray *rx_data);

private:
    State state_;

    bool is_single_;
    int rx_expected_len_;
    int read_fw_info_retry_;
    int exe_channel_retry_;
    QTimer *tmr_wait_receive_;
};

#endif // VM_CONTROLLER_H
