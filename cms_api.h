#ifndef CMS_API_H
#define CMS_API_H

#include <QObject>

#ifdef _DEV_STAGE_
    #define url_token           "https://api-v1-s.hillcorp.com/api/auth/get-token"
    #define url_temp            "https://api-v1-s.hillcorp.com/api/temperature"
    #define url_pulling         "https://api-v1-s.hillcorp.com/api/machine/pulling-cmd"
    #define url_init            "https://api-v1-s.hillcorp.com/api/machine/pulling-initial"
    #define url_transaction     "https://api-v1-s.hillcorp.com/api/transaction/store"
    #define url_settlement      "https://api-v1-s.hillcorp.com/api/settlement/store"
    #define url_barcode         "https://api-v1-s.hillcorp.com/api/items/barcode"
    #define url_lane_update     "https://api-v1-s.hillcorp.com/api/lane-items/update"
    #define url_version_get     "https://api-v1-s.hillcorp.com/api/version"
    #define url_version_update  "https://api-v1-s.hillcorp.com/api/version/update"
    #define url_event_log       "https://api-v1-s.hillcorp.com/api/event-log/store"
    #define url_machine_log     "https://api-v1-s.hillcorp.com/api/machine/store-log"
    #define url_love_code_query "https://api-v1-s.hillcorp.com/api/love-code/query"
    #define url_vendor_show     "https://api-v1-s.hillcorp.com/api/vendor/show"
#else
    #define url_token           "https://api-v1.hillcorp.com/api/auth/get-token"
    #define url_temp            "https://api-v1.hillcorp.com/api/temperature"
    #define url_pulling         "https://api-v1.hillcorp.com/api/machine/pulling-cmd"
    #define url_init            "https://api-v1.hillcorp.com/api/machine/pulling-initial"
    #define url_transaction     "https://api-v1.hillcorp.com/api/transaction/store"
    #define url_settlement      "https://api-v1.hillcorp.com/api/settlement/store"
    #define url_barcode         "https://api-v1.hillcorp.com/api/items/barcode"
    #define url_lane_update     "https://api-v1.hillcorp.com/api/lane-items/update"
    #define url_version_get     "https://api-v1.hillcorp.com/api/version"
    #define url_version_update  "https://api-v1.hillcorp.com/api/version/update"
    #define url_event_log       "https://api-v1.hillcorp.com/api/event-log/store"
    #define url_machine_log     "https://api-v1.hillcorp.com/api/machine/store-log"
    #define url_love_code_query "https://api-v1.hillcorp.com/api/love-code/query"
    #define url_vendor_show     "https://api-v1.hillcorp.com/api/vendor/show"
#endif
#define url_mohist_token    "https://mohist.hillcorp.com/api/ver1/auth/get-token"
#define url_mohist_ticket   "https://mohist.hillcorp.com/api/ver1/redeem/ticket"

// define event code
#define Event_BOOT                  "00"
#define Event_ERR_CONFIGS           "01"
#define Event_ERR_NETWORK           "02"
#define Event_ERR_INITIAL           "03"
#define Event_ERR_PULLING           "04"
#define Event_ERR_VMC_INIT          "05"
#define Event_ERR_CMS_TOKEN         "06"
#define Event_ERR_SCO               "07"
#define Event_ERR_DROP              "08"
#define Event_ERR_CARGO             "09"
#define Event_ERR_DOOR              "10"
#define Event_ERR_VMC_PORT          "11"
#define Event_ERR_CARD_PORT         "12"
#define Event_ERR_BILL_PORT         "13"
#define Event_ERR_COIN_PORT         "14"
#define Event_ERR_HOPPER_PORT       "15"
#define Event_INIT_FINISHED         "30"

// define event code
#define Log_MASTER_UPDATE_REQUEST   "20"
#define Log_MASTER_UPDATE_SUCCESS   "21"
#define Log_MASTER_UPDATE_FAILED    "22"

// define pulling command code
#define PCMD_READ_SW_VERSION        "10"
#define PCMD_READ_FW_VERSION        "11"
#define PCMD_SET_COMPRESSOR_ON      "20"
#define PCMD_SET_COMPRESSOR_OFF     "21"
#define PCMD_SET_DOOR_EVALVE_ON     "22"
#define PCMD_SET_DOOR_EVALVE_OFF    "23"
#define PCMD_MACHINE_INIT_REQUEST   "30"
#define PCMD_MASTER_UPDATE_START    "31"
#define PCMD_MASTER_UPDATE_SUCCESS  "32"
#define PCMD_MASTER_UPDATE_FAILED   "33"
#define PCMD_EDC_SETTLE_REQUEST     "50"
#define PCMD_REBOOT_REQUEST         "88"
#define PCMD_EDC_REBOOT_REQUEST     "99"

// define payment method
#define PAYMENT_UNKNOWN              "0"
#define PAYMENT_MEMBER               "4"
#define PAYMENT_FUJITSU_SCO          "5"
#define PAYMENT_EASY_CARD           "11"
#define PAYMENT_IPASS               "12"
#define PAYMENT_ICASH               "13"
#define PAYMENT_HAPPY_CASH          "14"
#define PAYMENT_CREDIC_CARD         "15"
#define PAYMENT_CASH                "16"
#define PAYMENT_LINE_PAY            "17"
#define PAYMENT_JCO_PAY             "18"

class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;

class CmsApi : public QObject
{
    Q_OBJECT

public:
    CmsApi(QObject *parent = nullptr);
    ~CmsApi();

    void setDebugEnabled(bool enabled);

   bool getUrlFileData(QString url, QByteArray *out_ba);

   bool getToken(QString machine_code, QByteArray *token);
   bool getVendorInfos(QString machine_code, QByteArray *infos);
   bool getMachineInfos(QString machine_code, QByteArray *infos);
   bool getRemoteCommand(QString machine_code, QByteArray *cmds);
   bool updateMonitoringInfos(QString machine_code, QMap<QString, QByteArray> info_map);
   bool updateTransactionInfos(QString machine_code, QMap<QString, QByteArray> info_map, QByteArray *resp_infos);
   bool updateSettlementInfos(QString machine_code, QMap<QString, QByteArray> info_map);
   bool getBarcodeInfos(QString machine_code, QString barcode, QByteArray *infos);
   bool updateLaneInfos(QString machine_code, QMap<QString, QByteArray> info_map);
   bool getVersionInfos(QString machine_code, QByteArray *infos);
   bool updateVersionInfos(QString machine_code, QString fw_ver, QString sw_ver);
   bool updateEventInfos(QString machine_code, QString event_code);
   bool updateMachineLog(QString machine_code, QString log_code, QMap<QString, QString> parameters);
   bool queryLoveCode(QString machine_code, QString love_code, QByteArray *infos);

   bool getMohistToken(QString machine_code, QByteArray *token);
   bool useMohistVoucher(QString machine_code, QString voucher_barcode, bool unused = false);

private:
   bool setRequestHeader(QString machine_code, QNetworkRequest* request);
   bool postAndWaitForReply(QNetworkRequest request, QByteArray request_data, QByteArray *reply_data);

private:
    QNetworkAccessManager *network_manager_;

    bool debug_enabled_ = true;
};

#endif // CMS_API_H
