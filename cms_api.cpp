#include "cms_api.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QEventLoop>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

CmsApi::CmsApi(QObject *parent)
    : QObject(parent)
{
    network_manager_ = new QNetworkAccessManager(this);
}

CmsApi::~CmsApi()
{
}

void CmsApi::setDebugEnabled(bool enabled)
{
    debug_enabled_ = enabled;
}

bool CmsApi::getUrlFileData(QString url, QByteArray *out_ba)
{
    QTimer tmr;
    QEventLoop loop;

    QNetworkRequest request(url);
    QNetworkReply *reply = network_manager_->get(request);

    connect(&tmr, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    tmr.start(10000);
    loop.exec();

    if (reply == nullptr) {
        return false;
    }
    if (reply->error() != QNetworkReply::NoError) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] getUrlFileData Error:" << reply->error();
        delete reply;
        return false;
    }

    QByteArray reply_data = reply->readAll();
    out_ba->clear();
    out_ba->append(reply_data.data(), reply_data.size());
    delete reply;
    return true;
}

bool CmsApi::getToken(QString machine_code, QByteArray *token)
{
    if (debug_enabled_)
        qDebug() << "[CmsApi] getToken start...";

    QUrl service_url = QUrl(url_token);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("machine_code", machine_code);
    request_data = QJsonDocument(request_obj).toJson();

    // set request header
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json" );
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    QEventLoop loop;
    QNetworkReply *reply = nullptr;

    // post request and wait for reply
    reply = network_manager_->post(request, request_data);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // check reply status
    if (reply == nullptr) {
        return false;
    }
    if (reply->error() != QNetworkReply::NoError) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] getToken Error:" << reply->error();
        delete reply;
        return false;
    }

    // check reply data is valid
    reply_data.append(reply->readAll());
    if (QString(reply_data).indexOf('{') >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] getToken Error:" << QString(reply_data);
        delete reply;
        return false;
    }

    token->clear();
    token->append(reply_data.data(), reply_data.size());
    if (debug_enabled_)
        qDebug() << "[CmsApi] getToken success ( size:" << token->size() << ")";
    delete reply;
    return true;
}

bool CmsApi::getVendorInfos(QString machine_code, QByteArray *infos)
{
    QUrl service_url = QUrl(url_vendor_show);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] getVendorInfos Error:" << QString(reply_data);
        return false;
    }
    infos->clear();
    infos->append(reply_data.data(), reply_data.size());

    if (debug_enabled_)
        qDebug() << "[CmsApi] get vendor infos:" << infos->data();
    return true;
}

bool CmsApi::getMachineInfos(QString machine_code, QByteArray *infos)
{
    QUrl service_url = QUrl(url_init);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("machine_code", machine_code);
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] getMachineInfos Error:" << QString(reply_data);
        return false;
    }

    infos->clear();
    infos->append(reply_data.data(), reply_data.size());
    if (debug_enabled_)
        qDebug() << "[CmsApi] get machine infos:" << infos->data();
    return true;
}

bool CmsApi::getRemoteCommand(QString machine_code, QByteArray *cmds)
{
    QUrl service_url = QUrl(url_pulling);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("machine_code", machine_code);
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] getRemoteCommand Error:" << QString(reply_data);
        return false;
    }

    cmds->clear();
    cmds->append(reply_data.data(), reply_data.size());
    if (debug_enabled_)
        qDebug() << "[CmsApi] get remote command:" << cmds->data();
    return true;
}

bool CmsApi::updateMonitoringInfos(QString machine_code, QMap<QString, QByteArray> info_map)
{
    QUrl service_url = QUrl(url_temp);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("machine_code", machine_code);
    QList<QString> keys = info_map.keys();
    foreach (QString key, keys) {
        request_obj.insert(key, QString::fromUtf8(info_map.value(key)));
    }
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] updateMonitoringInfos Error:" << QString(reply_data);
        return false;
    }
    if (debug_enabled_)
        qDebug() << "[CmsApi] monitoring infos updated";
    return true;
}

bool CmsApi::updateTransactionInfos(QString machine_code, QMap<QString, QByteArray> info_map, QByteArray *resp_infos)
{
    QUrl service_url = QUrl(url_transaction);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("machine_code", machine_code);
    QList<QString> keys = info_map.keys();
    foreach (QString key, keys) {
        request_obj.insert(key, QString::fromUtf8(info_map.value(key)));
    }
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] updateTransactionInfos Error:" << QString(reply_data);
        return false;
    }
    resp_infos->clear();
    resp_infos->append(reply_data.data(), reply_data.size());

    if (debug_enabled_)
        qDebug() << "[CmsApi] transaction infos updated";
    return true;
}

bool CmsApi::updateSettlementInfos(QString machine_code, QMap<QString, QByteArray> info_map)
{
    QUrl service_url = QUrl(url_settlement);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    QList<QString> keys = info_map.keys();
    foreach (QString key, keys) {
        request_obj.insert(key, QString::fromUtf8(info_map.value(key)));
    }
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] updateSettlementInfos Error:" << QString(reply_data);
        return false;
    }
    if (debug_enabled_)
        qDebug() << "[CmsApi] settlement infos updated";
    return true;
}

bool CmsApi::getBarcodeInfos(QString machine_code, QString barcode, QByteArray *infos)
{
    QUrl service_url = QUrl(url_barcode);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("barcode", barcode);
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] getBarcodeInfos Error:" << QString(reply_data);
        return false;
    }

    infos->clear();
    infos->append(reply_data.data(), reply_data.size());
    if (debug_enabled_)
        qDebug() << "[CmsApi] get barcode infos:" << infos->data();
    return true;
}

bool CmsApi::updateLaneInfos(QString machine_code, QMap<QString, QByteArray> info_map)
{
    QUrl service_url = QUrl(url_lane_update);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    QList<QString> keys = info_map.keys();
    foreach (QString key, keys) {
        QString info_value = QString::fromUtf8(info_map.value(key));
        QStringList info_list = info_value.split(QChar(','));
        QJsonObject infos_obj;
        for (int i = 0; i < info_list.count(); i++) {
            int mid_index = info_list.at(i).indexOf(QChar(':'));
            if (mid_index > 0) {
                QString info_key = info_list.at(i).left(mid_index);
                info_value = info_list.at(i).mid(mid_index + 1);
                infos_obj.insert(info_key, info_value);
            }
        }
        request_obj.insert(key, infos_obj);
    }

    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] updateLaneInfos Error:" << QString(reply_data);
        return false;
    }
    if (debug_enabled_)
        qDebug() << "[CmsApi] lane infos updated";
    return true;
}

bool CmsApi::getVersionInfos(QString machine_code, QByteArray *infos)
{
    QUrl service_url = QUrl(url_version_get);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] getVersionInfos Error:" << QString(reply_data);
        return false;
    }
    infos->clear();
    infos->append(reply_data.data(), reply_data.size());

    if (debug_enabled_)
        qDebug() << "[CmsApi] get version infos:" << infos->data();
    return true;
}

bool CmsApi::updateVersionInfos(QString machine_code, QString fw_ver, QString sw_ver)
{
    QUrl service_url = QUrl(url_version_update);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("fw_version", fw_ver);
    request_obj.insert("sw_version", sw_ver);
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] updateVersionInfos Error:" << QString(reply_data);
        return false;
    }
    if (debug_enabled_)
        qDebug() << "[CmsApi] version infos updated";
    return true;
}

bool CmsApi::updateEventInfos(QString machine_code, QString event_code)
{
    QUrl service_url = QUrl(url_event_log);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("event_code", event_code);
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] updateEventInfos Error:" << QString(reply_data);
        return false;
    }
    if (debug_enabled_)
        qDebug() << "[CmsApi] event updated" << event_code;
    return true;
}

bool CmsApi::updateMachineLog(QString machine_code, QString log_code, QMap<QString, QString> parameters)
{
    if (debug_enabled_)
        qDebug() << "[CmsApi] machine log update start...";

    QUrl service_url = QUrl(url_machine_log);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("log_code", log_code);
    QJsonObject parameter_obj;
    foreach(QString para_name, parameters.keys()) {
        parameter_obj.insert(para_name, parameters.value(para_name));
    }
    if (parameter_obj.isEmpty() == false) {
        request_obj.insert("parameter", parameter_obj);
    }
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] updateMachineLog Error:" << QString(reply_data);
        return false;
    }
    if (debug_enabled_)
        qDebug() << "[CmsApi] machine log updated" << log_code;
    return true;
}

bool CmsApi::queryLoveCode(QString machine_code, QString love_code, QByteArray *infos)
{
    QUrl service_url = QUrl(url_love_code_query);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    if (setRequestHeader(machine_code, &request) == false) {
        return false;
    }

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("love_code", love_code);
    request_data = QJsonDocument(request_obj).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // check reply data is valid
    if (QString(reply_data).indexOf("errors") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] query love code Error:" << QString(reply_data);
        return false;
    }

    // return the name of organization
    QJsonDocument json_data = QJsonDocument::fromJson(reply_data);
    QJsonObject root_obj = json_data.object();
    infos->clear();
    infos->append(root_obj.value("name").toString().toUtf8());

    return true;
}

bool CmsApi::getMohistToken(QString machine_code, QByteArray *token)
{
    if (debug_enabled_)
        qDebug() << "[CmsApi] getMohistToken start...";

    QUrl service_url = QUrl(url_mohist_token);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request parameters
    QJsonObject request_obj;
    request_obj.insert("machine_code", machine_code);
    request_data = QJsonDocument(request_obj).toJson();

    // set request header
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json" );
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    QEventLoop loop;
    QNetworkReply *reply = nullptr;

    // post request and wait for reply
    reply = network_manager_->post(request, request_data);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // check reply status
    if (reply == nullptr) {
        return false;
    }
    if (reply->error() != QNetworkReply::NoError) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] getMohistToken Error:" << reply->error();
        delete reply;
        return false;
    }

    // check reply data is valid
    reply_data.append(reply->readAll());
    if (QString(reply_data).indexOf("error") >= 0) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] getMohistToken Error:" << QString(reply_data);
        delete reply;
        return false;
    }

    // parser command
    QJsonDocument json_data = QJsonDocument::fromJson(reply_data);
    QJsonObject obj_root = json_data.object();

    token->clear();
    token->append(obj_root.value("access_token").toString().toUtf8());
    if (debug_enabled_)
        qDebug() << "[CmsApi] getMohistToken success ( size:" << token->size() << ")";
    delete reply;
    return true;
}

bool CmsApi::useMohistVoucher(QString machine_code, QString voucher_barcode, bool unused)
{
    QUrl service_url = QUrl(url_mohist_ticket);
    QNetworkRequest request(service_url);

    QByteArray request_data;
    QByteArray reply_data;

    // set request header
    QByteArray token;
    QByteArray bearer;

    if (getMohistToken(machine_code, &token) == false) {
        return false;
    }

    bearer.append("Bearer ");
    bearer.append(token);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"), bearer);

    // set request parameters
    QJsonArray request_array;
    QJsonObject request_obj;
    request_obj.insert("store_sernum", "NGN");
    request_obj.insert("qr_no", voucher_barcode);
    if (unused)
        request_obj.insert("module", "Unused");
    else
        request_obj.insert("module", "Used");
    request_obj.insert("used_date", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    request_obj.insert("usedName", "");
    request_array.insert(0, request_obj);
    request_data = QJsonDocument(request_array).toJson();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(request_data.size()));

    // post and wait for reply
    if (postAndWaitForReply(request, request_data, &reply_data) == false) {
        return false;
    }

    // parser command
    QJsonDocument reply_json_data = QJsonDocument::fromJson(reply_data);
    QJsonArray reply_array = reply_json_data.array();
    QJsonObject reply_obj = reply_array.at(0).toObject();
    QString reply_result = reply_obj.value("ReturnCode").toString();
    QString reply_message = reply_obj.value("ReturnMsg").toString();
    if (reply_result != "0000") {
        if (debug_enabled_)
            qDebug() << "[CmsApi] useMohistVoucher failed:" << reply_message;
        return false;
    }
    if (debug_enabled_)
        qDebug() << "[CmsApi] mohist voucher" << voucher_barcode << ((unused)? "unused" : "used");
    return true;
}

bool CmsApi::setRequestHeader(QString machine_code, QNetworkRequest* request)
{
    QByteArray token;
    QByteArray bearer;

    if (getToken(machine_code, &token) == false) {
        return false;
    }

    bearer.append("Bearer ");
    bearer.append(token);

    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request->setRawHeader(QByteArray("Authorization"), bearer);
    return true;
}

bool CmsApi::postAndWaitForReply(QNetworkRequest request, QByteArray request_data, QByteArray *reply_data)
{
    QEventLoop loop;
    QNetworkReply *reply = nullptr;

    // post request and wait for reply
    reply = network_manager_->post(request, request_data);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // check reply status
    if (reply == nullptr) {
        return false;
    }
    if (reply->error() != QNetworkReply::NoError) {
        if (debug_enabled_)
            qDebug() << "[CmsApi] QNetworkReply Error:" << reply->error();
        delete reply;
        return false;
    }

    // return reply data
    reply_data->clear();
    reply_data->append(reply->readAll());
    delete reply;
    return true;
}
