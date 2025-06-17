#include "apnsslctx.h"
#include "apnsslmng.h"
#include <openssl/bio.h>
#include <openssl/pkcs12.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <sys/types.h>
#include <utime.h>
#include <netdb.h>
#include <sstream>

#include <arpa/inet.h>
#include <curl/curl.h>
#include <boost/filesystem.hpp>

#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/timer/timer.hpp>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <IceUtil/UUID.h>

#include "nghttp2_apns.h"
#include "SendApnsWorkThread.h"
#include "EnhancedApnSrv.h"

#include "jwt.h"
#include "jwtinfo.pb.h"

#include "MetricMonitor.h"

#define PUSH_USE_CERT       0
#define PUSH_USE_JWT        1
#define MAX_JWT_EXPIRE_TIME 3000

using namespace rapidjson;

extern void alert_no_return(int code, const char *desc);

#define APNS_ROLE_MASTER 0
#define APNS_ROLE_SLAVE  1

#define _U_
static const char nghttp_err_str[][40] = {
    { "NGHTTP2_NO_ERROR" },
    { "NGHTTP2_PROTOCOL_ERROR" },
    { "NGHTTP2_INTERNAL_ERROR" },
    { "NGHTTP2_FLOW_CONTROL_ERROR" },
    { "NGHTTP2_SETTINGS_TIMEOUT" },
    { "NGHTTP2_STREAM_CLOSED" },
    { "NGHTTP2_FRAME_SIZE_ERROR" },
    { "NGHTTP2_REFUSED_STREAM" },
    { "NGHTTP2_CANCEL" },
    { "NGHTTP2_COMPRESSION_ERROR" },
    { "NGHTTP2_CONNECT_ERROR" },
    { "NGHTTP2_ENHANCE_YOUR_CALM" },
    { "NGHTTP2_INADEQUATE_SECURITY" },
    { "NGHTTP2_HTTP_1_1_REQUIRED" }
};

static void DealFailApnNode(ApnNode *pNode, CApnSsl *pA, const string &reason)
{
    CSendApnsWorkThread *pworker = pA->worker();
    log4cxx::LoggerPtr __JPUSH_SPEC_LOGGER__ = pA->GetLogHandler();
    if(pNode->cSendRet >= SERVER_ERR)
    {
        if(pNode->cSendRet == 1)
        {
            //pworker->PushNodeToUidList(pNode);
            MetricMonitor::incResendNotRspMsg();
        }
        JPSLOG_INFO("SERVER_ERR Resend |m:" << pNode->stPload()->_msgid << "|u:" << pNode->uiToUid << "|ret:" << pNode->cSendRet);
    }
    else
    {
        if(pNode->cSendRet == ERR_CERT)
        {
            MetricMonitor::incrInvalidCertMsg();
            pworker->updateCertStatus(pNode->stPload()->_strAppKey, pNode->stPload()->_ucPlatform, -1, pNode->stPload()->_pushMethod);
            JPSLOG_WARN("Add invalid cert:ERR_CERT, |a:" << pNode->stPload()->_strAppKey << "|m:" << pNode->stPload()->_msgid
                        << "|p:" << (int)pNode->stPload()->_ucPlatform << "|pm:" << (int)pNode->stPload()->_pushMethod);
            if((reason.find(STR_BADCERTIFICATE) != string::npos) || (reason.find(STR_BADCERTIFICATEENVIRONMENT) != string::npos))
            {
                pA->BeginPunish();
            }
            else if(reason.find(STR_EXPIRED_PROVIDER_TOKEN) != string::npos)
            {
                pA->SetRecvExpiredAuthKey(true);
            }
        }

        pworker->sync2PushResultResponse(pNode, REPORT_RESP);
        pworker->sync2MsgCycleMqResponse(pNode, reason);
        if(pNode->cSendRet == INACTIVE_TOKEN
           || (pNode->cSendRet == BAD_REQUEST
               && (reason.find(STR_DEVICETOKENNOTFORTOPIC) != string::npos || reason.find(STR_BADDEVICETOKEN) != string::npos)))
        {
            if (pNode->_apns_push_type != push_live_activity
                || reason.find(STR_BADDEVICETOKEN) == string::npos)
                pworker->reportInvalidToken(pNode);

            if(pNode->_apns_push_type == push_notification)
            {
                CEnhancedApnSrv *pSrv = pworker->getReportMQHandle();
                pNode->opBadgeType = (char)NEED_DEL;
                pSrv->updateBadge(pNode);
            }
        }
        else
        {
            if(pNode->opBadgeType == (char)INCR_OVER && pNode->_apns_push_type != push_live_activity)
            {
                CEnhancedApnSrv *pSrv = pworker->getReportMQHandle();
                pNode->opBadgeType = (char)NEED_SET;
                pSrv->updateBadge(pNode);
            }
        }
    }
}

static int on_header_callback(nghttp2_session *session,
                              const nghttp2_frame *frame,
                              const uint8_t *name, size_t namelen,
                              const uint8_t *value, size_t valuelen,
                              uint8_t flags, void *user_data)
{
    CApnSsl *pA = (CApnSsl *)user_data;
    log4cxx::LoggerPtr __JPUSH_SPEC_LOGGER__ = pA->GetLogHandler();
    ApnNode *pNode = (ApnNode *)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if(frame->hd.type == NGHTTP2_HEADERS)
    {
        JPSLOG_DEBUG("HEADERS For stream-id:" << frame->hd.stream_id << " " << name << "->" << value);
        if(0 == strncmp((const char *)name, APNS_ID_HEADER, strlen(APNS_ID_HEADER)))
        {
            JPSLOG_DEBUG("Receive response for apns-id:" << value);
        }
        else if (0 == strncmp((const char *)name, "apns-unique-id", strlen("apns-unique-id")))
        {
            // resp. apns-unique-id字段 该字段仅适用于开发环境
            JPSLOG_DEBUG("Receive response for apns-unique-id:" << value << " len:" << valuelen);
            if (valuelen < APNS_UNIQUE_ID_LEN + 1)
            {
                memset(pNode->strRspUniqueId, 0, APNS_UNIQUE_ID_LEN+1);
                strncpy(pNode->strRspUniqueId, (char*)value, valuelen);
                pNode->strRspUniqueId[APNS_UNIQUE_ID_LEN] = '\0';
                JPSLOG_DEBUG("Receive response for apns-unique-id:" << pNode->strRspUniqueId << " platform:" << pNode->stPload()->_ucPlatform);

                if (!pNode->stPload()->_ucPlatform) {
                    CSendApnsWorkThread *pworker = pA->worker();
                    pNode->cSendRet = 200;
                    pworker->sync2PushResultResponse(pNode, REPORT_RESP);
                    pworker->sync2MsgCycleMqResponse(pNode, "success");
                }
            }
        }
        else if(0 == strncmp((const char *)name, ":status", strlen(":status")))
        {
            int code = atoi((const char *)value);
            CSendApnsWorkThread *pworker = pA->worker();
            pNode->cSendRet = code;
            pA->IncRecvCnt();
            CEnhancedApnSrv *pSrv = pworker->getReportMQHandle();
            pNode->recived_from_apns = IceUtil::Time::now().toMilliSeconds();
            MetricMonitor::appleRspTime(pNode->recived_from_apns - pNode->send_aps_finished);
            MetricMonitor::totalTime(pNode->recived_from_apns - pNode->recived_from_mq);
            if(pNode->_apns_push_type & push_live_activity) MetricMonitor::incrSendLARsp();
            else if(pNode->_apns_push_type & push_voip) MetricMonitor::incrSendVoipRsp();
            else MetricMonitor::incrSendNormalRsp();
            if(code == 200)
            {
                JPSLOG_INFO("RESPONSE SUCC, |t:" << pworker->getThreadIndex() << "|aid:" << pNode->uiApnId << "|m:" << pNode->stPload()->_msgid
                            << "|u:" << pNode->uiToUid << "|s:" << pA->GetSslId() << "|ret:" << code << "|rtime:" << pNode->recived_from_apns - pNode->send_aps_finished
                            << "ms|ttime:" << pNode->recived_from_apns - pNode->recived_from_mq << "ms");
                if(pNode->opBadgeType == (char)NEED_SET)
                {
                    pSrv->updateBadge(pNode);
                }


                //pSrv->ReturnPayload(pNode);
            }
            else
            {
                JPSLOG_INFO("RESPONSE FAIL, |t:" << pworker->getThreadIndex() << "|aid:" << pNode->uiApnId << "|m:" << pNode->stPload()->_msgid
                            << "|u:" << pNode->uiToUid << "|s:" << pA->GetSslId() << "|ret:" << code << "|rtime:" << pNode->recived_from_apns - pNode->send_aps_finished
                            << "ms|ttime:" << pNode->recived_from_apns - pNode->recived_from_mq << "ms");
            }
        }
    }
    else
    {
        //bug: should check pNode is NULL?
        JPSLOG_FATAL("!!!Impossible Deal header type|" << (int)frame->hd.type << "| stream id|" << frame->hd.stream_id << "for |m:" << pNode->stPload()->_msgid << "|a:" << pNode->stPload()->_strAppKey << "|u:" << pNode->uiToUid);
    }

    return 0;
}

static int on_frame_recv_callback(nghttp2_session *session,
                                  const nghttp2_frame *frame, void *user_data)
{
    CApnSsl *pA = (CApnSsl *)user_data;
    log4cxx::LoggerPtr __JPUSH_SPEC_LOGGER__ = pA->GetLogHandler();
    ApnNode *pNode = NULL;
    switch(frame->hd.type)
    {
        case NGHTTP2_SETTINGS:
            for(int i = 0; i < frame->settings.niv; ++i)
            {
                if(NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS == frame->settings.iv[i].settings_id)
                {
                    //setting concurrent streams count
                    JPSLOG_TRACE("ssl|" << pA->GetSslId() << "|max concurrent streams|" << frame->settings.iv[i].value << "|appkey|" << pA->getAppkey());
                    pA->setRemoteStreams(frame->settings.iv[i].value);
                    //break;
                }
                else
                {
                    JPSLOG_TRACE("ssl|" << pA->GetSslId() << "|SETTING|" << frame->settings.iv[i].settings_id << "->" << frame->settings.iv[i].value);
                }
            }
            break;
        case NGHTTP2_HEADERS:
            JPSLOG_DEBUG("ssl|" << pA->GetSslId() << "|HEADERS|" << frame->headers.cat << "| stream id| " << frame->hd.stream_id);
            if(frame->headers.cat == NGHTTP2_HCAT_RESPONSE)
            {
                pNode = (ApnNode *)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
                if(pNode != NULL)
                {
                    CSendApnsWorkThread *pworker = (CSendApnsWorkThread *)pA->worker();
                    CEnhancedApnSrv *pSrv = pworker->getReportMQHandle();
                    long nowTime = IceUtil::Time::now().toMilliSeconds();
                    JPSLOG_DEBUG("Begin receive response |aid:" << pNode->uiApnId << "|m:" << pNode->stPload()->_msgid
                                 << "|u:" << pNode->uiToUid << "|s:" << pA->GetSslId() << "|rtime:" << nowTime - pNode->send_aps_finished
                                 << "ms|ttime:" << nowTime - pNode->recived_from_mq << "ms");
                }
            }
            break;
        case NGHTTP2_RST_STREAM:
            JPSLOG_INFO("ssl|" << pA->GetSslId() << "|RST_STREAM for stream id|" << frame->hd.stream_id << "|appkey|" << pA->getAppkey());
            break;
        case NGHTTP2_GOAWAY:
            //need to resend?
            {
                string reason((const char *)(frame->goaway.opaque_data), frame->goaway.opaque_data_len);
                JPSLOG_INFO("ssl|" << pA->GetSslId() << "|GOAWAY for stream id|" << frame->hd.stream_id << "|last frame id|"
                             << frame->goaway.last_stream_id << "|appkey|" << pA->getAppkey() << "|reason|" << reason);
                pNode = (ApnNode *)nghttp2_session_get_stream_user_data(session, frame->goaway.last_stream_id);
                if(pNode != NULL)
                {
                    //need resend after last stream!
                    pA->ResendUnDeal(pNode);
                }
                else
                {
                    JPSLOG_INFO("ssl|" << pA->GetSslId() << "|goaway frame opaque_data|" << frame->goaway.opaque_data
                                 << "|stream id|" << frame->hd.stream_id << "|appkey|" << pA->getAppkey());
                }
            }
            if(frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
            {
                //the frame is the last frame from the remote peer in this stream.
                //----->rv = nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR);
                JPSLOG_TRACE("ssl|" << pA->GetSslId() << "|Get last stream id|" << frame->hd.stream_id);
            }
            break;
        case NGHTTP2_DATA:
            JPSLOG_TRACE("ssl|" << pA->GetSslId() << "|DATA for stream id|" << frame->hd.stream_id);
            break;
        case NGHTTP2_WINDOW_UPDATE:
            JPSLOG_TRACE("ssl|" << pA->GetSslId() << "|WINDOW_UPDATE stream id|" << frame->hd.stream_id << "|size|" << frame->hd.length << "|appkey|" << pA->getAppkey());
            break;
        default:
            JPSLOG_FATAL("ssl|" << pA->GetSslId() << "|Undeal frame type|" << (int)frame->hd.type << "|stream id|" << frame->hd.stream_id);
    }
    return 0;
}

static int on_data_chunk_recv_callback(nghttp2_session *session,
                                       uint8_t flags, int32_t stream_id,
                                       const uint8_t *data, size_t len,
                                       void *user_data)
{
    CApnSsl *pA = (CApnSsl *)user_data;
    log4cxx::LoggerPtr __JPUSH_SPEC_LOGGER__ = pA->GetLogHandler();
    if(len)
    {
        ApnNode *pNode = (ApnNode *)nghttp2_session_get_stream_user_data(session, stream_id);
        if(pNode != NULL)
        {
            CSendApnsWorkThread *pworker = (CSendApnsWorkThread *)pA->worker();
            if(pNode->cSendRet != 200)
            {
                string reason((const char *)data, len);
                CEnhancedApnSrv *pSrv = pworker->getReportMQHandle();
                long nowTime = IceUtil::Time::now().toMilliSeconds();
                JPSLOG_INFO("RESPONSE FAIL, |t:" << pworker->getThreadIndex() << "|aid:" << pNode->uiApnId << "|m:" << pNode->stPload()->_msgid
                            << "|u:" << pNode->uiToUid << "|s:" << pA->GetSslId() << "|ret:" << pNode->cSendRet << "|reason:" << reason << "|rtime:"
                            << nowTime - pNode->send_aps_finished << "ms|ttime:" << nowTime - pNode->recived_from_mq << "ms");
                DealFailApnNode(pNode, pA, reason);
                //pSrv->ReturnPayload(pNode);
            }
        }
        else
        {
            JPSLOG_FATAL("Parse json data:|" << string((const char *)data, len) << "stream id" << stream_id << " sslid" << pA->GetSslId());
        }
    }
    else
    {
        JPSLOG_FATAL("Get invalid chuck data for stream_id->" << stream_id);
    }

    return 0;
}

static int on_stream_close_callback(nghttp2_session *session, int32_t stream_id,
                                    uint32_t error_code _U_,
                                    void *user_data _U_)
{
    CApnSsl *pA = (CApnSsl *)user_data;
    log4cxx::LoggerPtr __JPUSH_SPEC_LOGGER__ = pA->GetLogHandler();

    pA->DecLocalStreams();

    return 0;
}

ssize_t
data_prd_read_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf,
                       size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
{
    *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    CApnSsl *pA = (CApnSsl *)(user_data);
    log4cxx::LoggerPtr __JPUSH_SPEC_LOGGER__ = pA->GetLogHandler();
    ApnNode *pstApnNode = (ApnNode *)nghttp2_session_get_stream_user_data(session, stream_id);

    CSendApnsWorkThread *pWorker = pA->worker();
    CEnhancedApnSrv *pSrv = pWorker->getReportMQHandle();
    int payloadLen = 0;
    string appkey(pstApnNode->stPload()->_strAppKey);


    Document doc;
    auto &allocator = doc.GetAllocator();
    doc.Parse(pstApnNode->stPload()->_payload);
    if(doc.HasParseError() || !doc.IsObject())
    {
        JPSLOG_ERROR("Parse Payload Err for |m:" << pstApnNode->stPload()->_msgid << "|u:"
                         << pstApnNode->uiToUid << "|payload:" << (const char *)pstApnNode->stPload()->_payload);

        auto len = strlen(pstApnNode->stPload()->_payload);
        memcpy((char *)buf, pstApnNode->stPload()->_payload, len);
        buf[len] = '\0';
        return len;
    }

    if(pSrv->IsAddTokenAppList(appkey))
    {
        doc.AddMember("deviceToken", Value().SetString(pstApnNode->strDeviceToken, allocator), allocator);
    }

    if(!pSrv->IsNoArriveAppList(appkey))
    {
        doc.AddMember("_j_business", Value().SetInt(pSrv->GetPushUserType()), allocator);
        doc.AddMember("_j_uid", Value().SetUint64(pstApnNode->uiToUid), allocator);
    }

    if (push_live_activity == pstApnNode->_apns_push_type)
    {
        int now = (int)time(NULL);
        pSrv->parseLiveActivityPayload(pstApnNode, doc);//aps
        Value& la = doc["_j_live_activity"];
        if (la.IsObject())
            la.RemoveMember("activity_id");

        doc.RemoveMember("aps");
        doc.AddMember("aps", la, allocator);
        
        doc.RemoveMember("_j_voip");
        doc.RemoveMember("_j_live_activity");
    }
    else if(pstApnNode->_apns_push_type == push_voip)  // voip
    {
        doc.RemoveMember("aps");
        doc.RemoveMember("_j_live_activity");
    }
    else // ios notification
    {
        doc.RemoveMember("_j_voip");
        doc.RemoveMember("_j_live_activity");
        pSrv->parseUmsPayLoad(pstApnNode, doc);//aps
        pSrv->parseUmsGeoPayLoad(pstApnNode, doc);//geo

        if(pstApnNode->opBadgeType == (char)INCR_OVER)
        {
            rapidjson::Pointer("/aps/badge").Set(doc, pstApnNode->newBadgeValue);
        }
        else
        {
            //
        }
    }

    if(pstApnNode->stPload()->_has_msgdata)
    {
        doc.AddMember("_j_data_", pstApnNode->stPload()->getMsgData(), allocator);
    }

    StringBuffer sbuff;
    Writer<StringBuffer> writer(sbuff);
    doc.Accept(writer);

    memcpy(buf, sbuff.GetString(), sbuff.GetSize());
    payloadLen = sbuff.GetSize();
    buf[payloadLen] = '\0';

    JPSLOG_DEBUG("send apns data, |m:" << pstApnNode->stPload()->_msgid << "|u:" << pstApnNode->uiToUid
                 << "|pt:" << (int)pstApnNode->_apns_push_type << "|payload:" << (const char *)buf);


    return payloadLen;
}

long msec_delta(const struct timeval &start, const struct timeval &end);

CApnSsl::CApnSsl(string sAppKey, int iPlatform, unsigned char push_method, CApnNodeList *pList, CApnSslMng *pSslMng, size_t hostSize, size_t curHostIndex, const char *logger_name)
    : m_pApnSslMng(pSslMng)
    , m_iSocket(-1)
    , m_iConnected(0)
    , m_uiCertFilestamp(0)
    , m_iCertFileInvalid(0)
    , m_uiSslId(0)
    , m_uiLastSendApnId(0)
    , m_uiLastConnectedTime(0)
    , m_uiSendCnt(0)
    , m_uiRecvCnt(0)
    , m_local_streams(0)
    , m_remote_streams(MAX_CURRENT_STREAMS)
    , m_have_recv_goaway(false)
    , m_sAppkey(sAppKey)
    , m_iPlatform(iPlatform)
    , m_uiPunishmentEndTime(0)
    , m_uiLastCheckCertTime(0)
    , m_pApnNodeList(pList)
    , m_pid(getpid())
    , m_isused(0)
    , m_verify_open(0)
    , m_maxHostSize(hostSize)
    , m_curHostIndex(curHostIndex)
    , m_firstPush(true)
    , m_worker(NULL)
{
    m_stream_id = 1;
    m_session = NULL;
    m_curApnNode = NULL;
    m_response_index = 0;
    m_uiLastRecvApnId = 0;
    m_iRet = 0;
    m_push_method = push_method;
    m_strUuid = IceUtil::generateUUID();

    if(PUSH_USE_CERT == push_method)
    {
        string sSslKey = string(sAppKey + (iPlatform ? "_dis" : "_dev"));
        m_sKeyFilePath = string(m_pApnSslMng->GetCertFileDir());
        m_sKeyFilePath += sSslKey + "_certkey.pem";
    }
    else
    {
        m_sKeyFilePath = string(m_pApnSslMng->GetCertFileDir());
        m_sKeyFilePath += sAppKey + "_authTokenkey.file";
        m_last_get_authKey_time = 0;
        m_recv_authKey_expired = false;
    }

    time(&m_lastUsedTime);
    m_lastSendTime = m_lastUsedTime;
    JPLOG_INIT_OBJ_LOGGER(logger_name);
}

CApnSsl::~CApnSsl()
{
    if(m_iConnected)
    {
        SslDisconnect(0);
    }
}
CSendApnsWorkThread *CApnSsl::worker()
{
    return m_worker;
}

void CApnSsl::SetWorker(CSendApnsWorkThread *worker)
{
    if(this->m_worker == NULL)
    {
        this->m_worker = worker;
    }
}

void CApnSsl::setup_nghttp2_callbacks(nghttp2_session_callbacks *callbacks)
{
    nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback);
}

int CApnSsl::set_nghttp2_session_info()
{
    int rv;
    nghttp2_session_callbacks *callbacks;

    rv = nghttp2_session_callbacks_new(&callbacks);
    if(rv != 0)
    {
        JPSLOG_FATAL("nghttp2_session_callbacks_new err:" << rv << " appkey:" << m_sAppkey);
        return -1;
    }

    setup_nghttp2_callbacks(callbacks);

    nghttp2_option *option = NULL;
    nghttp2_option_new(&option);
    nghttp2_option_set_peer_max_concurrent_streams(option, MAX_CURRENT_STREAMS);

    rv = nghttp2_session_client_new3(&m_session, callbacks, this, option, NULL);
    if(rv != 0)
    {
        JPSLOG_FATAL("nghttp2_session_client_new3 err:" << rv << " appkey:" << m_sAppkey);
        return -1;
    }
    nghttp2_session_callbacks_del(callbacks);

    nghttp2_option_del(option);
    m_remote_streams = MAX_CURRENT_STREAMS;
    m_local_streams = 0;
    m_remote_windows_size = MAX_APNS_WINDOWS_SIZE;

    return 0;
}

int CApnSsl::InitNghttp2Session()
{
    int rv;

    rv = set_nghttp2_session_info();
    if(rv < 0)
    {
        JPSLOG_FATAL("set_nghttp2_session_info err:" << rv << " appkey:" << m_sAppkey);
        return -1;
    }

    rv = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, NULL, 0);
    if(rv != 0)
    {
        JPSLOG_FATAL("nghttp2_submit_settings err:" << rv << " appkey:" << m_sAppkey);
        return -1;
    }

    return 0;
}

#define RESULT_BUF_SIZE 128
static size_t ReportCallback(char *ptr, size_t size, size_t nmemb, void *retRes)
{
    size_t real_size = nmemb * size;
    int left_size = RESULT_BUF_SIZE - strlen((char *)retRes);

    if(real_size > left_size)
    {
        return 0;
    }

    strncat((char *)retRes, ptr, real_size);

    return real_size;
}

int CApnSsl::ReportCert(long code)
{
    CURL *curl;
    CURLcode res;
    if(!m_pApnSslMng->GetCertReportURL().length())
    {
        return 1;
    }

    curl = curl_easy_init();
    if(curl)
    {
        char url[256] = { 0 };
        char retRes[RESULT_BUF_SIZE] = { 0 };
        //production url: https://api.srv.jpush.cn/v1/portal/certificate
        //dev url: https://5566ua.com:8900/v1/portal/certificate
        snprintf(url, sizeof(url), "%s/%s/cert/%d/errCode/%ld/update", m_pApnSslMng->GetCertReportURL().c_str(), m_sAppkey.c_str(), m_iPlatform, code);
        //curl_easy_setopt(curl, CURLOPT_URL, "http://example.com");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        /* example.com is redirected, so we tell libcurl to follow redirection */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, m_pApnSslMng->GetCertReportTimeOut());
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReportCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)retRes);
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);

        /* Check for errors */
        if(res != CURLE_OK)
        {
            JPSLOG_FATAL("Report Cert failed: curl_easy_perform() failed |" << curl_easy_strerror(res) << "|Url|" << url);
        }
        else if(NULL == strstr(retRes, "success"))
        {
            JPSLOG_INFO("Report Cert failed |" << retRes << "|Url|" << url);
        }
        else
        {
            JPSLOG_INFO("Report Cert Success for|" << url);
        }
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return 0;
}

static long getCertError(long *ssl_err_reason, unsigned long *ssl_err_code)
{
    long cert_err_code = 0;
    *ssl_err_code = ERR_get_error();

    switch(((*ssl_err_reason) = ERR_GET_REASON(*ssl_err_code)))
    {
        case SSL_R_SSLV3_ALERT_CERTIFICATE_EXPIRED: /*,define in <openssl/ssl.h> "sslv3 alert certificate expired"},*/
            cert_err_code = X509_V_ERR_CERT_HAS_EXPIRED;
            break;
        case SSL_R_SSLV3_ALERT_CERTIFICATE_REVOKED: /*,"sslv3 alert certificate revoked"},*/
            cert_err_code = X509_V_ERR_CERT_REVOKED;
            break;
    }
    return cert_err_code;
}

int CApnSsl::LoadVerfy()
{
    if(!m_pSslCtx)
    {
        JPSLOG_FATAL("Error set verify for cert failed!invalide SSL_CTX!");
        return 0;
    }
    if(!m_pApnSslMng->GetCertReportURL().length())
    {
        JPSLOG_FATAL("Empty Cert report URL: " << m_pApnSslMng->GetCertReportURL());
        return 0;
    }
    /*
    SSL_CTX_set_verify(m_pSslCtx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_verify_depth(m_pSslCtx, 3);
    if (!SSL_CTX_set_default_verify_paths(m_pSslCtx))
    {
        JPSLOG_FATAL("["<<__FUNCTION__<<"] " <<<< "error set verify for cert failed, load verify location failed!");
        return -1;
    }
    */
    m_verify_open = 1;
    JPSLOG_TRACE("Load verify locations success, report is url: " << m_pApnSslMng->GetCertReportURL().c_str());
    return 0;
}

int CApnSsl::VerifyReport()
{
    unsigned long ssl_err_code = 0;
    return VerifyReport(&ssl_err_code);
}

int CApnSsl::VerifyReport(unsigned long *ssl_err_code)
{
    if(!m_verify_open)
        return 0;
    //man verify to lookup the errocode description!
    long ssl_err_reason = 0;
    long cert_err_code = getCertError(&ssl_err_reason, ssl_err_code);
    //code = SSL_get_verify_result(m_pSsl);
    JPSLOG_INFO("Cert-Verify-INFO, appkey|" << m_sAppkey << "|platform|" << m_iPlatform << "|errCode|" << cert_err_code << "|retCode|" << ssl_err_reason);
    if(ssl_err_reason == SSL_R_SSLV3_ALERT_CERTIFICATE_UNKNOWN)
    {
        BeginPunish();
        return SSL_R_SSLV3_ALERT_CERTIFICATE_UNKNOWN;
    }
    else if(ssl_err_reason == SSL_R_TLSV1_ALERT_INTERNAL_ERROR)
    {
        BeginPunish();
        return SSL_R_SSLV3_ALERT_CERTIFICATE_UNKNOWN;
    }

    if(cert_err_code == X509_V_ERR_CERT_HAS_EXPIRED || cert_err_code == X509_V_ERR_CERT_REVOKED)
    {
        m_worker->updateCertStatus(m_sAppkey, m_iPlatform, -1, m_push_method);
        JPSLOG_WARN("Add invalid cert:ERR_CERT, appKey|" << m_sAppkey << "|platform|" << m_iPlatform << "|pushmethod|" << (int)m_push_method);
        if(ReportCert(cert_err_code))
        {
            JPSLOG_FATAL("Cert-Verify-ERROR appkey |" << m_sAppkey << "|platform|" << m_iPlatform << "|errCode|" << cert_err_code);
        }
        BeginPunish();
        return cert_err_code;
    }
    return 0;
}

size_t CApnSsl::setNextHostIndex()
{
    m_pApnSslMng->setNextApsHostIndex(m_iPlatform);
    m_curHostIndex = m_pApnSslMng->getApsHostIndex(m_iPlatform);
    return m_curHostIndex;
}

void CApnSsl::setHostIndex()
{
    m_curHostIndex = m_pApnSslMng->getApsHostIndex(m_iPlatform);
}

void CApnSsl::BeginPunish()
{
    CEnhancedApnSrv *pSrv = m_worker->getReportMQHandle();
    m_uiPunishmentEndTime = pSrv->getTimeNowS() + m_pApnSslMng->GetPunishmentTimeVal();
    JPSLOG_INFO("Ssl punish appkey|" << m_sAppkey << "|platform|" << m_iPlatform << "|untill|" << m_uiPunishmentEndTime);
}

static bool is_certificate_expired(X509 *x509, time_t check)
{
    if(X509_cmp_time(X509_get_notAfter(x509), &check) <= 0)
    {
        return true;
    }
    return false;
}

static bool is_expired_less_than_N_days(X509 *x509, int N)
{
    static const time_t SEC_PER_DAY = 60 * 60 * 24;
    time_t check = (SEC_PER_DAY * N);
    if(X509_cmp_time(X509_get_notAfter(x509), &check) <= 0)
    {
        return true;
    }
    return false;
}

static X509 *read_x509_certificate(const char *path)
{
    BIO *bio = NULL;
    X509 *x509 = NULL;
    if(NULL == (bio = BIO_new_file(path, "r")))
    {
        return NULL;
    }
    x509 = PEM_read_bio_X509_AUX(bio, NULL, NULL, NULL);
    BIO_free(bio);
    return x509;
}

bool is_certificate_expired(const char *path, time_t check, string &notAfter)
{
    X509 *x509 = NULL;
    if(NULL == (x509 = read_x509_certificate(path)))
    {
        return true;
    }
    notAfter = string((const char *)X509_get_notAfter(x509)->data);
    if(is_certificate_expired(x509, check))
    {
        X509_free(x509);
        return true;
    }
    X509_free(x509);
    return false;
}

int CApnSsl::SslCtxCreate(int *conn_error)
{
    /* Create an SSL_METHOD structure (choose an SSL/TLS protocol version) */
    m_pSslMethod = (SSL_METHOD *)SSLv23_client_method();

    /* Create an SSL_CTX structure */
    m_pSslCtx = SSL_CTX_new(m_pSslMethod);
    if(!m_pSslCtx)
    {
        *conn_error = errno;
        JPSLOG_FATAL("New SSL Context fail for" << m_sAppkey);
        return -11;
    }
    /*load verify conf*/
    LoadVerfy();
    /* Load the client certificate into the SSL_CTX structure */
    X509 *x509 = NULL;

    if(access(m_sKeyFilePath.c_str(), F_OK) == -1)
    {
        JPSLOG_FATAL("Access Certification fail for " << m_sAppkey);
        return -2;
    }

    if(NULL == (x509 = read_x509_certificate(m_sKeyFilePath.c_str())))
    {
        return -12;
    }
    CEnhancedApnSrv *pSrv = m_worker->getReportMQHandle();
    time_t now = pSrv->getTimeNowS();
    notAfter_ = string((const char *)X509_get_notAfter(x509)->data);
    if(is_certificate_expired(x509, now))
    {
        X509_free(x509);
        return -18;
    }

    if(is_expired_less_than_N_days(x509, 3))
    {
        JPSLOG_WARN("Certificate would Expired less than 3 days, appkey|" << m_sAppkey << "|notAfter|" << notAfter_);
    }

    //begin add by chenh.
    if(x509 && x509->name)
    {
        JPSLOG_TRACE("appkey|" << m_sAppkey << "|x509->name|" << x509->name);
        //name is char*
        m_apns_topic = x509->name;
        string::size_type sidx = m_apns_topic.find(COMM_PUSH_SERVICE_PREFFIX);
        if(sidx != string::npos)
        {
            sidx += COMM_PUSH_SERVICE_PREFFIX_LEN;
            string::size_type eidx = m_apns_topic.find('/', sidx);
            if(eidx != string::npos)
            {
                eidx -= sidx;
            }
            m_apns_topic = m_apns_topic.substr(sidx, eidx);
            JPSLOG_TRACE("appkey|" << m_sAppkey << "|parse x509 name to topic|" << m_apns_topic << "|");
        }
    }
    //End add by chenh.

    int ret = SSL_CTX_use_certificate(m_pSslCtx, x509);
    X509_free(x509);
    if(ret != 1)
    {
        *conn_error = errno;
        JPSLOG_FATAL("Cannot use Certificate File|" << m_sKeyFilePath << "|error|"
                     << ERR_error_string(ERR_get_error(), NULL) << "|ret|" << ret << "|appkey|" << m_sAppkey);
        if(m_pSslCtx)
        {
            SSL_CTX_free(m_pSslCtx);
            m_pSslCtx = NULL;
        }
        BeginPunish();
        return -13;
    }

    /* Load the private-key corresponding to the client certificate */
    if(SSL_CTX_use_PrivateKey_file(m_pSslCtx, m_sKeyFilePath.c_str(),
                                   SSL_FILETYPE_PEM)
       <= 0)
    {
        *conn_error = errno;
        JPSLOG_FATAL("Cannot use Certificate File|" << m_sKeyFilePath << "|error|"
                     << ERR_error_string(ERR_get_error(), NULL) << "|appkey|" << m_sAppkey);
        if(m_pSslCtx)
        {
            SSL_CTX_free(m_pSslCtx);
            m_pSslCtx = NULL;
        }
        BeginPunish();
        return -14;
    }

    /* Check if the client certificate and private-key matches */
    if(!SSL_CTX_check_private_key(m_pSslCtx))
    {
        *conn_error = errno;
        JPSLOG_FATAL("Private key does not match the certificate public key|" << ERR_error_string(ERR_get_error(), NULL) << "|appkey|" << m_sAppkey);
        if(m_pSslCtx)
        {
            SSL_CTX_free(m_pSslCtx);
            m_pSslCtx = NULL;
        }
        BeginPunish();
        return -15;
    }
    SSL_CTX_set_next_proto_select_cb(m_pSslCtx, client_select_next_proto_cb, NULL);
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
    SSL_CTX_set_alpn_protos(m_pSslCtx, (const unsigned char *)NGHTTP2_H2_ALL_ALPN, NGHTTP2_H2_ALL_ALPN_LEN);
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
    return 0;
}

int CApnSsl::GetAuthTokenByKeyInfo(string &apns_authKey, string &apns_keyId, string &apns_teamId)
{
    jwt_t *jwtObject = NULL;
    int iRet = 0;

    iRet = jwt_new(&jwtObject);
    if(iRet != 0)
    {
        JPSLOG_FATAL("jwt_new Err, iRet|" << iRet << "|appkey|" << m_sAppkey);
        return -1;
    }

    CEnhancedApnSrv *pSrv = m_worker->getReportMQHandle();
    time_t nowTime = pSrv->getTimeNowS();
    jwt_add_grant(jwtObject, "iss", (const char *)(apns_teamId.c_str()));
    jwt_add_grant_int(jwtObject, "iat", (long)nowTime);
    jwt_set_alg(jwtObject, JWT_ALG_ES256, (const unsigned char *)(apns_authKey.c_str()), apns_authKey.length());

    char *pAuthToken = jwt_encode_str(jwtObject, 1, (const char *)(apns_keyId.c_str()));
    if(NULL == pAuthToken)
    {
        JPSLOG_FATAL("jwt_encode_str Err, apns_authKey|" << apns_authKey << "|apns_keyId|" << apns_keyId
                     << "|apns_teamId|" << apns_teamId << "|appkey|" << m_sAppkey);
        jwt_free(jwtObject);
        return -2;
    }

    m_authKey.clear();
    m_authKey.assign("bearer ");
    m_authKey.append(pAuthToken);

    free(pAuthToken);
    jwt_free(jwtObject);

    return 0;
}

int CApnSsl::GetAuthTokenByKeyFile()
{
    fstream input(m_sKeyFilePath.data(), ios::in | ios::binary);
    JPushJwtTokenInfo::JwtInfo jwt_info;

    jwt_info.ParseFromIstream(&input);
    string strApnsAuthKey = jwt_info.apns_authkey();
    string strApnsKeyId = jwt_info.apns_keyid();
    string strApnsTeamId = jwt_info.apns_teamid();
    m_apns_topic = jwt_info.bundleid();

    int iRet = 0;
    iRet = GetAuthTokenByKeyInfo(strApnsAuthKey, strApnsKeyId, strApnsTeamId);
    if(iRet < 0)
    {
        JPSLOG_FATAL("GetAuthToken By Jwt Err, KeyFile|" << m_sKeyFilePath << "|appkey|" << m_sAppkey);
        return -1;
    }

    CEnhancedApnSrv *pSrv = m_worker->getReportMQHandle();
    m_last_get_authKey_time = pSrv->getTimeNowS();

    return 0;
}

int CApnSsl::SslJwtTokenCreate(int *conn_error)
{
    /* Create an SSL_METHOD structure (choose an SSL/TLS protocol version) */
    m_pSslMethod = (SSL_METHOD *)SSLv23_client_method();

    /* Create an SSL_CTX structure */
    m_pSslCtx = SSL_CTX_new(m_pSslMethod);
    if(!m_pSslCtx)
    {
        *conn_error = errno;
        JPSLOG_FATAL("New SSL Context fail for " << m_sAppkey);
        return -11;
    }
    /*load verify conf*/
    LoadVerfy();

    if(access(m_sKeyFilePath.data(), F_OK) == -1)
    {
        JPSLOG_FATAL("Access JwtTokenFile fail for" << m_sAppkey);
        return -2;
    }

    if(GetAuthTokenByKeyFile() == -1)
    {
        return -2;
    }

    SSL_CTX_set_next_proto_select_cb(m_pSslCtx, client_select_next_proto_cb, NULL);
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
    SSL_CTX_set_alpn_protos(m_pSslCtx, (const unsigned char *)NGHTTP2_H2_ALL_ALPN, NGHTTP2_H2_ALL_ALPN_LEN);
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
    return 0;
}

int CApnSsl::epollProcess(int sockfd, int flag, int timeout)
{
    const int MAX_EVENTS = 1;

    int epollfd = epoll_create(1);
    if(epollfd == -1)
        return -1;

    struct epoll_event event;
    event.events = (uint32_t) (flag | EPOLLET);
    event.data.fd = sockfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
        close(epollfd);
        return -1;
    }

    struct epoll_event eventArr[MAX_EVENTS];
    int ret = epoll_wait(epollfd, eventArr, MAX_EVENTS, timeout*1000);
    if(ret == -1) { // wait error
        epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, &event);
        close(epollfd);
        return -1;
    }
    if (0 == ret) { // timeout
        epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, &event);
        close(epollfd);
        return 0;
    }

    int events = eventArr[0].events;
    if (events & (EPOLLHUP | EPOLLERR)) {
        ret = -2;
    } else if (events & EPOLLIN || events & EPOLLOUT) {
        ret = 1;
    }
    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, &event);
    close(epollfd);
    return ret;
}

int CApnSsl::SslSockCreate(int *conn_error)
{
    /* Set up a TCP socket */
    m_iSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(-1 == m_iSocket)
    {
        *conn_error = errno;
        JPSLOG_FATAL("Get Socket error|" << strerror(errno) << "|appkey|" << m_sAppkey);
        if(m_pSslCtx)
        {
            SSL_CTX_free(m_pSslCtx);
            m_pSslCtx = NULL;
        }
        return -16;
    }

    //set sock to non-block
    int flags = 1; //nonblock reusaddr
    if(ioctl(m_iSocket, FIONBIO, &flags) && ((flags = fcntl(m_iSocket, F_GETFL, 0)) < 0 || fcntl(m_iSocket, F_SETFL, flags | O_NONBLOCK) < 0))
    {
        JPSLOG_FATAL("Set non block socket error|" << strerror(errno) << "|appkey|" << m_sAppkey);
        close(m_iSocket);
        m_iSocket = -1;
        if(m_pSslCtx)
        {
            SSL_CTX_free(m_pSslCtx);
            m_pSslCtx = NULL;
        }
        return -17;
    }
    return 0;
}

int CApnSsl::ApnsDNSParse(int *conn_error)
{
    return 0;
}

int CApnSsl::SslConnect(int *conn_error, int re_send)
{
    *conn_error = 0;
    if(m_iConnected)
    {
        JPSLOG_TRACE("Ssl already connected");
        return 0;
    }

    if(m_sAppkey.empty())
    {
        JPSLOG_FATAL("Appkey Not Set, connect fail");
        return -1;
    }

    CEnhancedApnSrv *pSrv = m_worker->getReportMQHandle();
    unsigned int uiTimeNow = pSrv->getTimeNowS();

    int iRet = 0;

    boost::timer::cpu_timer timer;

    if(uiTimeNow < m_uiPunishmentEndTime)
    {
        *conn_error = errno;
        JPSLOG_FATAL("punished from|" << uiTimeNow << "|to|" << m_uiPunishmentEndTime << "|appkey|" << m_sAppkey);
        return -3;
    }

    int ret = 0;

    if(PUSH_USE_CERT == m_push_method)
    {
        ret = SslCtxCreate(conn_error);
    }
    else
    {
        ret = SslJwtTokenCreate(conn_error);
    }

    if(ret != 0)
    {
        return ret;
    }
    ret = SslSockCreate(conn_error);
    if(ret != 0)
    {
        return ret;
    }

    struct sockaddr_in stServerAddr;
    memset(&stServerAddr, '\0', sizeof(stServerAddr));
    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_port = htons(m_pApnSslMng->GetApsPort(m_iPlatform, m_curHostIndex)); /* Server Port number */

    struct hostent hostInfo, *pHostInfo = NULL; // = gethostbyname(m_pApnSslMng->GetApsHost(m_iPlatform));
#define HOST_BASE_BUF_LEN 1024
    char hostBuf[HOST_BASE_BUF_LEN] = { '\0' };
    int host_len = HOST_BASE_BUF_LEN;
    char *pHostBuf = hostBuf; //NULL;
    int h_err = 0;
    int base_step = 2;
    long start = pSrv->getTimeNowMs();
    int hret = gethostbyname_r(m_pApnSslMng->GetApsHost(m_iPlatform, m_curHostIndex), &hostInfo,
                               hostBuf, host_len, &pHostInfo, &h_err);
    long end = pSrv->getTimeNowMs();
    int spend = end - start;
    if(spend >= 1000)
    {
        JPSLOG_WARN("Parse DNS|" << m_pApnSslMng->GetApsHost(m_iPlatform, m_curHostIndex) << "| time spend exceed 1 s spend|" << spend << " ms|");
    }
    JPSLOG_TRACE("Total dns number is " << m_maxHostSize);

    while(1)
    {
        if(0 == hret && NULL != pHostInfo)
        {
            break;
        }
        else if(ERANGE == hret) //buf is not enough, should not happend ,1024 is enough!
        {
            JPSLOG_FATAL("Parse DNS|" << m_pApnSslMng->GetApsHost(m_iPlatform, m_curHostIndex) << " : " << strerror(h_err) << " , buf is not enough, buflen " << host_len << "");
            sleep(1);
            if(hostBuf != pHostBuf)
            {
                delete[] pHostBuf;
            }
            host_len = HOST_BASE_BUF_LEN * base_step;
            pHostBuf = new char[host_len];
            while(pHostBuf == NULL)
            {
                sleep(1);
                pHostBuf = new char[host_len];
            }
            start = pSrv->getTimeNowMs();
            hret = gethostbyname_r(m_pApnSslMng->GetApsHost(m_iPlatform, m_curHostIndex), &hostInfo,
                                   pHostBuf, host_len, &pHostInfo, &h_err);
            end = pSrv->getTimeNowMs();
            spend = end - start;
            if(spend >= 1000)
            {
                JPSLOG_FATAL("Parse DNS|" << m_pApnSslMng->GetApsHost(m_iPlatform, m_curHostIndex) << "| time spend exceed 1 s spend|" << spend << " ms|");
            }
            ++base_step;
        }
        else
        {
            JPSLOG_FATAL("Parse DNS|" << m_pApnSslMng->GetApsHost(m_iPlatform, m_curHostIndex) << " : " << strerror(h_err) << " , Reresolving;" << "dns time waste: " << spend << " ms");
            usleep(1000);
            setNextHostIndex();
            start = pSrv->getTimeNowMs();
            hret = gethostbyname_r(m_pApnSslMng->GetApsHost(m_iPlatform, m_curHostIndex), &hostInfo,
                                   pHostBuf, host_len, &pHostInfo, &h_err);
            end = pSrv->getTimeNowMs();
            spend = end - start;
            if(spend >= 1000)
            {
                JPSLOG_FATAL("Parse DNS|" << m_pApnSslMng->GetApsHost(m_iPlatform, m_curHostIndex) << "| time spend exceed 1 s spend|" << spend << " ms|");
            }
            continue;
        }
    }

    int next = 0;
    int iSocket = m_iSocket;
    char ipApns[32] = {0};
    for(; pHostInfo->h_addr_list[next]; ++next)
    {
        iSocket = m_iSocket;
        stServerAddr.sin_addr.s_addr = *(in_addr_t *)pHostInfo->h_addr_list[next];
        stServerAddr.sin_port = htons(m_pApnSslMng->GetApsPort(m_iPlatform, m_curHostIndex)); /* Server Port number */
        memset(ipApns, 0, sizeof(ipApns));
        inet_ntop(AF_INET,&stServerAddr.sin_addr, ipApns, sizeof(ipApns));
        /* Establish a TCP/IP connection to the SSL client */
        ret = connect(m_iSocket, (struct sockaddr *)&stServerAddr, sizeof(stServerAddr));

        if(ret < 0)
        {
            int err = errno;
            errno = 0;
            if(err != EINPROGRESS)
            {
                *conn_error = err;
                JPSLOG_FATAL("Connect Fail[Ret=" << ret << ",ErrNo=" << err << "] for appkey[" << m_sAppkey << "]");
                close(m_iSocket);
                m_iSocket = -1;
                if(SslSockCreate(conn_error))
                    break;
                continue;
            }
            int error = 0;
            socklen_t len = sizeof(error);
            int flag = (EPOLLIN|EPOLLOUT);
            ret = epollProcess(m_iSocket, flag, m_pApnSslMng->GetSockConnectTimeout());
            if (ret > 0)
            {
                if(getsockopt(m_iSocket, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
                {
                    ret = -3;
                }
                if(error)
                {
                    sleep(1);
                    ret = -4;
                }
            }
            if (ret <= 0)
            {
                *conn_error = errno;
                err = errno;
                JPSLOG_FATAL("Connect Fail epollProcess:|" << ret
                             << "| sockerror:" << error<< "| errno|" << err <<"| appkey|" << m_sAppkey
                             << "| ip|" << ipApns << "| socket|" << m_iSocket << "| uuid|" << m_strUuid << "|");
                close(m_iSocket);
                m_iSocket = -1;
                if(SslSockCreate(conn_error))
                {
                    break;
                }
                continue;
            }

            JPSLOG_DEBUG("non bolck socket connect succ" << " for appkey[" << m_sAppkey << "]");
        }
        ret = 0;
        break;
    }

    if(NULL == pHostInfo->h_addr_list[next] || ret < 0)
    {
        JPSLOG_FATAL("nonblock socket connect failed!" << " for appkey[" << m_sAppkey << "]");
        if(m_iSocket >= 0)
            close(m_iSocket);
        m_iSocket = -1;
        if(m_pSslCtx)
        {
            SSL_CTX_free(m_pSslCtx);
            m_pSslCtx = NULL;
        }
        if(hostBuf != pHostBuf)
        {
            delete[] pHostBuf;
        }
        return -42;
    }
    if(hostBuf != pHostBuf)
    {
        delete[] pHostBuf;
    }
    *conn_error = 0;
    /* An SSL structure is created */
    m_pSsl = SSL_new(m_pSslCtx);
    if(!m_pSsl)
    {
        *conn_error = errno;
        JPSLOG_FATAL("Could not get SSL Socket" << " for appkey[" << m_sAppkey << "]");
        close(m_iSocket);
        m_iSocket = -1;
        if(m_pSslCtx)
        {
            SSL_CTX_free(m_pSslCtx);
            m_pSslCtx = NULL;
        }
        return -27;
    }

    /* Assign the socket into the SSL structure (SSL and socket without BIO) */
    SSL_set_fd(m_pSsl, m_iSocket);

    /* Perform SSL Handshake on the SSL client */
    ret = SSL_connect(m_pSsl);

    while(1 != ret)
    {
        if(-1 == ret)
        {
            ret = SSL_get_error(m_pSsl, ret);
            JPSLOG_DEBUG("non bolck ssl continue: " << ret << " for appkey[" << m_sAppkey << "]");
            int flag = 0;
            if(SSL_ERROR_WANT_READ == ret)
            {
                flag |= EPOLLIN;
            }
            else if(SSL_ERROR_WANT_WRITE == ret)
            {
                flag |= EPOLLOUT;
            }
            else
            {
                *conn_error = errno;
                JPSLOG_FATAL("ssl Connect Fail: ret " << ret << " : err " << SSL_get_error(m_pSsl, ret) << " :errno " << errno << " for appkey[" << m_sAppkey << "]");
                VerifyReport();
                SSL_free(m_pSsl);
                close(iSocket);
                m_pSsl = NULL;
                m_iSocket = -1;
                if(m_pSslCtx)
                {
                    SSL_CTX_free(m_pSslCtx);
                    m_pSslCtx = NULL;
                }
                if(errno != 32 && errno != 104)
                    BeginPunish();
                return -31;
            }

            int err;
            ret = epollProcess(m_iSocket, flag, m_pApnSslMng->GetSslConnectTimeout());
            if(ret == 0)
            {
                err = errno;
                *conn_error = err;
                JPSLOG_FATAL("ssl Connect Fail epollProcess|" << ret << "| errno|" << err << "| for appkey|" << m_sAppkey
                                 << "| socket|" << m_iSocket << "| uuid|" << m_strUuid << "|" );
                SSL_free(m_pSsl);
                close(m_iSocket);
                m_pSsl = NULL;
                m_iSocket = -1;
                if(m_pSslCtx)
                {
                    SSL_CTX_free(m_pSslCtx);
                    m_pSslCtx = NULL;
                }
                return -32;
            }
            else if(ret < 0)
            {
                err = errno;
                *conn_error = errno;
                JPSLOG_FATAL("ssl Connect Fail epollProcess|" << ret << "| errno|" << err << "| for appkey|" << m_sAppkey
                                 << "| socket|" << m_iSocket << "| uuid|" << m_strUuid << "|" );
                VerifyReport();
                SSL_free(m_pSsl);
                close(m_iSocket);
                m_pSsl = NULL;
                m_iSocket = -1;
                if(m_pSslCtx)
                {
                    SSL_CTX_free(m_pSslCtx);
                    m_pSslCtx = NULL;
                }
                return -33;
            }
            ret = SSL_connect(m_pSsl);
        }
        else
        {
            int ssl_error = SSL_get_error(m_pSsl, ret);
            unsigned long ssl_err_code = 0;
            int cert_err_code = VerifyReport(&ssl_err_code);
            JPSLOG_FATAL("ssl connnect fail: ret " << ret << " : " << ssl_error << " : errno " << errno << ", ssl_err_code:" << ssl_err_code<< " for appkey:" << m_sAppkey);
            if(cert_err_code)
            {
                *conn_error = cert_err_code;
            }
            else if(ssl_error == SSL_ERROR_SYSCALL && ssl_err_code == 0 && ret == 0) //man SSL_get_error
            {
                SSL_free(m_pSsl);
                close(iSocket);
                m_pSsl = NULL;
                m_iSocket = -1;
                if(m_pSslCtx)
                {
                    SSL_CTX_free(m_pSslCtx);
                    m_pSslCtx = NULL;
                }
                return -36;
            }
            SSL_free(m_pSsl);
            close(iSocket);
            m_pSsl = NULL;
            m_iSocket = -1;
            if(m_pSslCtx)
            {
                SSL_CTX_free(m_pSslCtx);
                m_pSslCtx = NULL;
            }
            if(errno != 32 && cert_err_code != 0)
                BeginPunish();
            return -34;
        }
    }

    VerifyReport();

    const unsigned char *next_proto = NULL;
    unsigned int next_proto_len = 0;
    SSL_get0_next_proto_negotiated(m_pSsl, &next_proto, &next_proto_len);
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
    if(next_proto == nullptr)
    {
        SSL_get0_alpn_selected(m_pSsl, &next_proto, &next_proto_len);
    }
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L

    if(next_proto == nullptr)
    {
        JPSLOG_FATAL("tls shake err, next_proto is nullptr!" << " for appkey:" << m_sAppkey);
        SSL_free(m_pSsl);
        close(iSocket);
        m_pSsl = NULL;
        m_iSocket = -1;
        if(m_pSslCtx)
        {
            SSL_CTX_free(m_pSslCtx);
        }
        return -53;
    }
    size_t len = next_proto_len;
    const char *np = reinterpret_cast<const char*>(next_proto);
    string tmp(np, len);
    JPSLOG_TRACE("ALPN support protocol [" << tmp << "]" << " for appkey[" << m_sAppkey << "]");
    if(false == check_h2_is_selected(next_proto, next_proto_len))
    {
        JPSLOG_FATAL("tls shake err, next_proto is not http-2!" << " for appkey[" << m_sAppkey << "]");
        SSL_free(m_pSsl);
        close(iSocket);
        m_pSsl = NULL;
        m_iSocket = -1;
        if(m_pSslCtx)
        {
            SSL_CTX_free(m_pSslCtx);
        }
        return -53;
    }
    //Begin add by chenh.
    if(InitNghttp2Session())
    {
        SSL_free(m_pSsl);
        close(iSocket);
        m_pSsl = NULL;
        m_iSocket = -1;
        if(m_pSslCtx)
        {
            SSL_CTX_free(m_pSslCtx);
            m_pSslCtx = NULL;
        }

        iRet = INIT_HTTP2_SESSION_ERR;
        JPSLOG_FATAL("InitNghttp2Session err, iRet:" << iRet << " for appkey[" << m_sAppkey << "]");
        return iRet;
    }
    //reset recvbuf

    m_uiSslId = m_pApnNodeList->GetNewSslId();
    m_iConnected = 1;
    m_uiLastSendApnId = 0;
    m_uiLastRecvApnId = 0;
    m_uiSendCnt = 0;
    m_uiRecvCnt = 0;
    m_firstPush = true;
    m_have_recv_goaway = false;
    m_recv_authKey_expired = false;
    m_pApnSslMng->ApnSslDidConnected(this);


    //add client ip/client_port/serverip/server_port for sslId.
    struct sockaddr_in guest;
    struct sockaddr_in serv;
    char guest_ip[20] = { 0 };
    char serv_ip[20] = { 0 };

    socklen_t guest_len = sizeof(guest);
    socklen_t serv_len = sizeof(serv);

    getsockname(m_iSocket, (struct sockaddr *)&guest, &guest_len);
    getpeername(m_iSocket, (struct sockaddr *)&serv, &serv_len);

    inet_ntop(AF_INET, &guest.sin_addr, guest_ip, sizeof(guest_ip));
    inet_ntop(AF_INET, &serv.sin_addr, serv_ip, sizeof(serv_ip));

    JPSLOG_DEBUG("non bolck ssl connect done, ssl|" << m_uiSslId << "|appKey|" << m_sAppkey << "|platform|" << m_iPlatform
                 << "|serverip|" << serv_ip << "|serverport|" << ntohs(serv.sin_port) << "|clientip|" << guest_ip << "|client_port|" << ntohs(guest.sin_port));

    return 0;
}

int CApnSsl::ReadResponseDataFromSock(int iForce)
{
    if(!m_iConnected)
    {
        JPSLOG_FATAL("Impossible For ssl sock |" << m_iSocket << "| ssl|" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "| read resposne but ssl not connected! ");
        return 0;
    }
    int rv = 0;
    int recvl = 0;
    uint8_t buf[65536]; //max stream data for http2!
    static int buflen = sizeof(buf);
    int total = 0, rvtotal = 0;
    int roffset = 0;
    int woffset = 0;
    for(;;)
    {
        rv = SSL_read(m_pSsl, buf + woffset, buflen - woffset);
        if(rv > 0)
        {
            rvtotal += rv;
            recvl = nghttp2_session_mem_recv(m_session, buf + roffset, woffset + rv - roffset);
            if(recvl < 0)
            {
                JPSLOG_FATAL("Impossible undeal For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "| read resposne data failed! ret " << recvl << " expect recv len " << woffset + rv);
                rv = -1;
                break;
            }
            else if(recvl + roffset != rv + woffset)
            {
                roffset += recvl;
                woffset += rv;
                JPSLOG_FATAL("Impossible undeal For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "| read resposne data failed! ret " << recvl << " expect recv len " << rv << " offset " << woffset);
            }
            else
            {
                roffset = 0;
                woffset = 0;
            }
            total += recvl;
            JPSLOG_TRACE("For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "| read total data length: " << total << " read length " << rvtotal);
        }
        else if(rv < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                rv = 0;
                JPSLOG_DEBUG("For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "Get EAGAIN OR EWOULDBLOCK!");
                if(rvtotal != total)
                {
                    JPSLOG_FATAL("!!!BUG!!!!  For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "| read total data length: " << total << " except length " << rvtotal);
                    rv = -2;
                }
            }
            else if(errno == EINTR)
            {
                JPSLOG_FATAL("For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "| Get a EINTR continue");
                continue;
            }
            else //if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                rv = SSL_get_error(m_pSsl, rv);
                //judge if ret is a fatal error
                if(SSL_ERROR_WANT_READ == rv || SSL_ERROR_WANT_WRITE == rv)
                {
                    JPSLOG_FATAL("For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "| read resposne data ret " << rv << "read:" << SSL_ERROR_WANT_READ << " write:" << SSL_ERROR_WANT_WRITE << errno);
                    continue;
                }
                else
                {
                    unsigned long errcode = ERR_get_error();
                    char errInfo[1024] = { '\0' };
                    ERR_error_string_n(errcode, errInfo, sizeof(errInfo));
                    JPSLOG_FATAL("For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "| read resposne data error! ret " << rv << "errno " << errno << "ssl errcode " << errcode << "error: " << errInfo);
                    rv = -3;
                }
            }
            break;
        }
        else if(rv == 0)
        {
            int eret = SSL_get_error(m_pSsl, rv);
            JPSLOG_FATAL("remote close connection For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "| read resposne data error! ret " << rv << " errno " << errno << " sslerror " << eret);
            rv = -4;
            break;
        }
    }

    m_remote_windows_size = nghttp2_session_get_remote_window_size(m_session);
    if(rvtotal != total)
    {
        JPSLOG_WARN("!!!BUG!!!!  For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << "| read total data length: " << total << " except length " << rvtotal);
        rv = -5;
    }
    if(rv < 0 || iForce != 0)
    {
        JPSLOG_DEBUG("Close For ssl sock |" << m_uiSslId << "| app|" << m_sAppkey << ":" << m_iPlatform << " rv:" << rv << " force" << iForce);
        SslDisconnect(iForce);
    }
    return rv;
}

int CApnSsl::ResendUnDeal(ApnNode *poNode)
{
    if(poNode->_mqResent)
    {
        return 0;
    }

    int resendCnt = 0;
    m_uiLastSendApnId = poNode->uiApnId;
    //get goaway!
    m_have_recv_goaway = true;
    ApnNode *pNode;
    int iRet = m_pApnNodeList->GetApnByApnId(&pNode, poNode->uiNextSentApnId);
    poNode->uiNextSentApnId = 0;
    if(iRet == 0)
    {
        pNode->uiPreviousSentApnId = 0;
    }
    while(iRet == 0)
    {
        if(pNode->cSendRet != 1)
        {
            JPSLOG_FATAL("Goaway Fail Resend ApnId|" << pNode->uiApnId << "| msgId|" << pNode->stPload()->_msgid << "| uid|" << pNode->uiToUid << "| from sslId " << m_uiSslId << " Cur stat(s:r)|" << m_uiSendCnt << ":" << m_uiRecvCnt << "|"
                             << "it has been re-send ret: " << pNode->cSendRet);
            break;
        }
        JPSLOG_INFO("Goaway Suc Resend ApnId|" << pNode->uiApnId << "| msgId|" << pNode->stPload()->_msgid << "| uid|" << pNode->uiToUid << "| from sslId " << m_uiSslId << " Cur stat(s:r)|" << m_uiSendCnt << ":" << m_uiRecvCnt << "|");
        //m_worker->PushNodeToUidList(pNode);
        MetricMonitor::incResendNotRspMsg();
        ++resendCnt;
        iRet = m_pApnNodeList->GetApnByApnId(&pNode, pNode->uiNextSentApnId);
    }
    m_uiSendCnt -= resendCnt;
    JPSLOG_INFO("For |" << m_sAppkey << "| from sslId " << m_uiSslId << " Cur stat(s:r)|" << m_uiSendCnt << ":" << m_uiRecvCnt << "| resend cnt|" << resendCnt << "|");
    return resendCnt;
}
int CApnSsl::ResendSentFail()
{
    ApnNode *pNode = NULL;
    int iRet = 0;
    CSendApnsWorkThread *pworker = this->worker();

    if(m_have_recv_goaway)
    {
        while(m_uiSendCnt > m_uiRecvCnt)
        {
            iRet = m_pApnNodeList->GetApnByApnId(&pNode, m_uiLastSendApnId);
            if(iRet == 0)
            {
                if(pNode->cSendRet == 1)
                {
                    pNode->cSendRet = 200;
                    this->IncRecvCnt();
                    string reason("unknow result");

                    CEnhancedApnSrv *pSrv = pworker->getReportMQHandle();
                    long nowTime = IceUtil::Time::now().toMilliSeconds();
                    JPSLOG_INFO("threadNo|" << pworker->getThreadIndex() << "|GET RESPONSE UNKNOW for apnId|" << pNode->uiApnId << "|msgId|" << pNode->stPload()->_msgid << "|uid|" << pNode->uiToUid << "|sslid|" << this->GetSslId() << "|ret|" << pNode->cSendRet << "|response|" << reason << "|resp-time|" << nowTime - pNode->send_aps_finished << "|ms, Total-time|" << nowTime - pNode->recived_from_mq << "|ms.");
                }
            }
            else
            {
                break;
            }

            m_uiLastSendApnId = pNode->uiPreviousSentApnId;
            pNode->uiPreviousSentApnId = 0;
        }
    }
    else
    {
        CEnhancedApnSrv *pSrv = pworker->getReportMQHandle();
        while(m_uiSendCnt > m_uiRecvCnt)
        {
            iRet = m_pApnNodeList->GetApnByApnId(&pNode, m_uiLastSendApnId);
            if(iRet == 0)
            {
                if(pNode->cSendRet == 1)
                {
                    --m_uiSendCnt;
                    if(pNode->send_times >= pSrv->GetMaxLimitResendTimes())
                    {
                        if(pSrv->GetRoleType() == APNS_ROLE_MASTER)
                        {
                            if(pSrv->GetResendAllAppKey())
                            {
                                //pSrv->SendResendMsgToMQ(pNode);
                            }
                            else
                            {
                                if(pSrv->IsResendAppKey(pNode->stPload()->_strAppKey))
                                {
                                    //pSrv->SendResendMsgToMQ(pNode);
                                }
                                else
                                {
                                    pNode->cSendRet = -200;
                                    string reason("unrecv response");
                                    pworker->sync2MsgCycleMqResponse(pNode, reason);
                                    JPSLOG_FATAL("threadNo|" << pworker->getThreadIndex() << "Disconnect Resend Over " << pSrv->GetMaxLimitResendTimes()
                                                 << "times ApnId|" << pNode->uiApnId << "|msgId|" << pNode->stPload()->_msgid << "|host|" << m_pApnSslMng->GetApsHost(pNode->stPload()->_ucPlatform, m_curHostIndex)
                                                 << "|uid|" << pNode->uiToUid << "|sslId|" << m_uiSslId << " Cur stat(s:r)|" << m_uiSendCnt << ":" << m_uiRecvCnt << "|");
                                }
                            }
                        }
                        else
                        {
                            pNode->cSendRet = -200;
                            string reason("unrecv response");
                            pworker->sync2MsgCycleMqResponse(pNode, reason);
                            JPSLOG_FATAL("threadNo|" << pworker->getThreadIndex() << "Disconnect Resend Over " << pSrv->GetMaxLimitResendTimes() << "times ApnId|"
                                         << pNode->uiApnId << "| msgId|" << pNode->stPload()->_msgid << "|host|" << m_pApnSslMng->GetApsHost(pNode->stPload()->_ucPlatform, m_curHostIndex)
                                         << "|uid|" << pNode->uiToUid << "|sslId|" << m_uiSslId << " Cur stat(s:r)|" << m_uiSendCnt << ":" << m_uiRecvCnt << "|");
                        }
                    }
                    else
                    {
                        JPSLOG_INFO("threadNo:|" << pworker->getThreadIndex() << "Disconnect Resend ApnId|" << pNode->uiApnId << "|msgId|" << pNode->stPload()->_msgid
                                    << "|host|" << m_pApnSslMng->GetApsHost(pNode->stPload()->_ucPlatform, m_curHostIndex) << "|uid|" << pNode->uiToUid << "|mqresendflag|" << (int)pNode->_mqResent
                                    << "|sslId|" << m_uiSslId << " Cur stat(s:r)|" << m_uiSendCnt << ":" << m_uiRecvCnt << "|");
                        if(!pNode->_mqResent)
                        {
                            MetricMonitor::incResendNotRspMsg();
                            //m_worker->PushNodeToUidList(pNode);
                        }
                    }
                }
            }
            else
            {
                break;
            }

            m_uiLastSendApnId = pNode->uiPreviousSentApnId;
            pNode->uiPreviousSentApnId = 0;
        }
    }

    return m_uiSendCnt - m_uiRecvCnt;
}

int CApnSsl::RecordSentFail()
{
    ApnNode *pNode = NULL;
    int iRet = 0;
    CSendApnsWorkThread *pworker = this->worker();
    CEnhancedApnSrv *pSrv = pworker->getReportMQHandle();
    unsigned int LastSendApnId = m_uiLastSendApnId;
    while(m_uiSendCnt > m_uiRecvCnt)
    {
        iRet = m_pApnNodeList->GetApnByApnId(&pNode, LastSendApnId);
        if(iRet == 0)
        {
            if(pNode->cSendRet == 1)
            {
                pNode->cSendRet = 200;
                this->IncRecvCnt();

                string reason("unknow result");
                long nowTime = IceUtil::Time::now().toMilliSeconds();
                JPSLOG_INFO("threadNo|" << pworker->getThreadIndex() << "|GET RESPONSE UNKNOW for apnId|" << pNode->uiApnId << "|msgId|"
                            << pNode->stPload()->_msgid << "|uid|" << pNode->uiToUid << "|sslid|" << this->GetSslId() << "|ret|" << pNode->cSendRet
                            << "|response|" << reason << "|resp-time|" << nowTime - pNode->send_aps_finished << "|ms, Total-time|" << nowTime - pNode->recived_from_mq << "|ms.");
            }
        }
        else
        {
            break;
        }

        LastSendApnId = pNode->uiPreviousSentApnId;
        pNode->uiPreviousSentApnId = 0;
    }

    return m_uiSendCnt - m_uiRecvCnt;
}

void CApnSsl::SslDisconnect(int iWithResponse)
{
    if(!m_iConnected)
    {
        JPSLOG_FATAL("Could not disconnect SSL: already disconnected");
        return;
    }

    m_iConnected = 0;
    //want to read an want to write equal zero!
    if(m_session)
    {
        nghttp2_session_del(m_session);
        m_session = NULL;
    }

    if(m_uiSendCnt > m_uiRecvCnt)
    {
        JPSLOG_FATAL("Err Close For Business ssl|" << m_uiSslId << "|appKey|" << m_sAppkey << "|platform|" << m_iPlatform
                     << "|SentNum|" << m_uiSendCnt << "|RecvNum|" << m_uiRecvCnt);
        if(m_pApnSslMng->GetResendFlag())
        {
            ResendSentFail();
        }
        else
        {
            RecordSentFail();
        }
    }
    JPSLOG_FATAL("Suc Close For ssl|" << m_uiSslId << "|appKey|" << m_sAppkey << "|platform|" << m_iPlatform
                 << "|SentNum|" << m_uiSendCnt << "|RecvNum|" << m_uiRecvCnt);

    m_pApnSslMng->ApnSslDidDisconnected(this);
    m_uiSendCnt = 0;
    m_uiRecvCnt = 0;
    m_local_streams = 0;
    m_remote_streams = MAX_CURRENT_STREAMS;
    m_uiLastSendApnId = 0;
    m_uiLastRecvApnId = 0;
    m_uiLastConnectedTime = 0;
    m_verify_open = 0;
    m_firstPush = true;
    CEnhancedApnSrv *pSrv = m_worker->getReportMQHandle();
    unsigned int uiTimeNow = pSrv->getTimeNowS();
#define CERT_INVALID_JUDGE_TIME (10) //should not disconnect with no response for valid cert in 10s since connect
    if(!iWithResponse && m_uiLastConnectedTime + CERT_INVALID_JUDGE_TIME > uiTimeNow)
    {
        BeginPunish();
    }

    int ret = 0;

    /* Shutdown the client side of the SSL connection */
    if(m_pSsl)
    {
        ret = SSL_shutdown(m_pSsl);
        if(ret == -1)
        {
            JPSLOG_FATAL("Could not shutdown SSL:" << SSL_get_error(m_pSsl, ret));
        }

        /* Free the SSL structure */
        SSL_free(m_pSsl);

        m_pSsl = NULL;
    }
    /* Terminate communication on a socket */
    if(-1 != m_iSocket)
    {
        ret = close(m_iSocket);
        if(ret == -1)
        {
            JPSLOG_FATAL("Could not close socket : " << errno);
        }

        m_iSocket = -1;
    }

    /* Free the SSL_CTX structure */
    if(m_pSslCtx)
    {
        SSL_CTX_free(m_pSslCtx);
        m_pSslCtx = NULL;
    }
}

bool CApnSsl::TryDisconnectH2LimitedSsl(unsigned int maxInterval)
{
    time_t interval = time(NULL) - m_lastSendTime;
    if (interval < maxInterval*3)
        return false;

    if (TryUniqUsed())
    {
        ReadResponseDataFromSock(1);
        UnUsed(EXCLUSION_USED);
        return true;
    }
    return false;
}

bool CApnSsl::IsAuthTokenExpired()
{
    CEnhancedApnSrv *pSrv = m_worker->getReportMQHandle();
    time_t tNow = pSrv->getTimeNowS();
    if((m_recv_authKey_expired) || (tNow - m_last_get_authKey_time >= MAX_JWT_EXPIRE_TIME))
    {
        return true;
    }

    return false;
}

int CApnSsl::SendEnhancedApn(ApnNode *pstApnNode)
{
    int iRet = 0;
    char strBinDeviceToken[128] = { 0 };
    int usDeviceTokenLen = 127;

    CEnhancedApnSrv *pSrv = m_worker->getReportMQHandle();
    m_lastUsedTime = pSrv->getTimeNowS();
    m_lastSendTime = m_lastUsedTime;
    if(!m_iConnected)
    {
        iRet = 1;
    }
    else
    {
        //Begin add by chenh.
        if((PUSH_USE_JWT == m_push_method) && (IsAuthTokenExpired()))
        {
            if(GetAuthTokenByKeyFile() < 0)
            {
                JPSLOG_FATAL("SendEnhancedApn Error, msgid|" << pstApnNode->stPload()->_msgid << "|uid|" << pstApnNode->uiToUid << "|Reason|GetAuthTokenByKeyFile Failed");
                return GET_SSL_KEY_FAILED;
            }
        }

        unsigned int pathlen = 0;
        char path[256] = { 0 };
        char apnsid[40] = { 0 };
        char apnsexpiration[20] = { 0 };


        snprintf(path, sizeof(path) - 1, "/3/device/%s", pstApnNode->strDeviceToken);
        snprintf(apnsid, sizeof(apnsid) - 1, "%s", IceUtil::generateUUID().c_str());
        //snprintf(apnsid, sizeof(apnsid) - 1, APNS_ID_FORMAT, pstApnNode->uiApnId);
        snprintf(apnsexpiration, sizeof(apnsexpiration) - 1, "%u", pstApnNode->stPload()->_uiExpireTime);
        nghttp2_nv nva[MAX_APNS_HEADERS_COUNT] = {
            MAKE_NV(APNS_METHOD_HEADER, APNS_DEFAULT_METHOD, APNS_NO_NAME_VALUE_CP),
            MAKE_NV(APNS_PATH_HEADER, path, APNS_NOIDX_NO_NAME_CP)
        };
        int h_cnt = 2;
        h_cnt += pstApnNode->stPload()->load_headers(nva + h_cnt, pstApnNode->_apns_push_type);
        if (pstApnNode->stPload()->_priority == 10 || pstApnNode->_apns_push_type == push_live_activity)
        {
            nva[h_cnt++] = MAKE_NV(APNS_PRIORITY_HEADER, APNS_PRIORITY_10, APNS_NO_NAME_VALUE_CP);
        } else {
            nva[h_cnt++] = MAKE_NV(APNS_PRIORITY_HEADER, APNS_PRIORITY_5, APNS_NO_NAME_VALUE_CP);
        }
        if(m_pApnSslMng->GetMockHttp2Flag()) {
            // (:scheme/host header) needed by nginx, for mock server
            nva[h_cnt++] = MAKE_NV(APNS_SCHEME_HEADER, APNS_SCHEME, APNS_NO_NAME_VALUE_CP);
            string nghost;
            if (m_iPlatform == 1)
                nghost = "api.push.apple.com:2197";
            else
                nghost = "api.development.push.apple.com:2197";
            nva[h_cnt++] = MAKE_NV(APNS_HOST_HEADER, nghost.data(), APNS_NO_NAME_CP);
        }

        // set apns-topic 
        std::string topic = m_apns_topic;
        if(!topic.empty() && pstApnNode->_apns_push_type == push_voip)
        {
            if(!boost::algorithm::ends_with(topic, std::string(".voip")))
            {
                topic += ".voip";
            }
        }
        if(!topic.empty() && pstApnNode->_apns_push_type == push_live_activity)
        {
            if(!boost::algorithm::ends_with(topic, std::string(".liveactivity")))
            {
                topic += ".push-type.liveactivity";
            }
        }

        if(!topic.empty())
        {
            nva[h_cnt++] = MAKE_NV(APNS_TOPIC_HEADER, topic.data(), APNS_NO_NAME_CP);
        }

        JPSLOG_TRACE("apns-topic: " << topic << " apnid:" << pstApnNode->uiApnId << " apns-id:" << apnsid
                         << " uid:" << pstApnNode->uiToUid << " msgid:" << pstApnNode->stPload()->_msgid);

        if(PUSH_USE_JWT == m_push_method)
        {
            nva[h_cnt++] = MAKE_NV(APNS_JWTAUTH_HEADER, m_authKey.data(), APNS_NOIDX_NO_NAME_CP);
        }
        if(m_firstPush)
        {
            nva[h_cnt++] = MAKE_NV(APNS_ID_HEADER, apnsid, APNS_NO_NAME_CP);
            nva[h_cnt++] = MAKE_NV(APNS_EXPIRATION_HEADER, apnsexpiration, APNS_NO_NAME_CP);
            nva[h_cnt++] = MAKE_NV(APNS_COLLAPSE_ID_HEADER, pstApnNode->stPload()->_apns_collapse_id, APNS_NO_NAME_VALUE_CP);
            m_firstPush = false;
        }
        else
        {
            nva[h_cnt++] = MAKE_NV(APNS_ID_HEADER, apnsid, APNS_NOIDX_NO_NAME_CP);
            nva[h_cnt++] = MAKE_NV(APNS_EXPIRATION_HEADER, apnsexpiration, APNS_NOIDX_NO_NAME_CP);
            nva[h_cnt++] = MAKE_NV(APNS_COLLAPSE_ID_HEADER, pstApnNode->stPload()->_apns_collapse_id, APNS_NOIDX_NO_NAME_VALUE_CP);
        }
        #if 1
        for (size_t i = 0; i < h_cnt; i++)
        {
            JPSLOG_TRACE("header name: " << nva[i].name << " value: " << nva[i].value);
        }
        #endif
        this->m_curApnNode = pstApnNode;
        nghttp2_data_provider data_prd;
        data_prd.source.ptr = (void *)pstApnNode;
        data_prd.read_callback = data_prd_read_callback;
        int stream_id = nghttp2_submit_request(m_session, NULL, nva, h_cnt, &data_prd, pstApnNode);
        pstApnNode->stream_id = m_stream_id;
        if(stream_id < 0)
        {
            JPSLOG_FATAL("SendEnhancedApn nghttp2_submit_request err " << stream_id << ", apnsid:" << pstApnNode->uiApnId
                             << ", uid:" << pstApnNode->uiToUid << ", msgId:" << pstApnNode->stPload()->_msgid);
            return 1;
        }

        ++m_local_streams;
        JPSLOG_TRACE("For sslid: " << m_uiSslId << " apnid: " << pstApnNode->uiApnId << " uid:" << pstApnNode->uiToUid << " stream-id " << stream_id << " last send stream id " << m_stream_id);
        string sdata;
        for(;;)
        {
            ssize_t n;
            const uint8_t *b;
            n = nghttp2_session_mem_send(m_session, &b);
            if(n == 0)
            {
                JPSLOG_TRACE("data over, total length: " << sdata.length());
                break;
            }
            else if(n < 0)
            {
                JPSLOG_FATAL("For sslid: " << m_uiSslId << " apnid: " << pstApnNode->uiApnId << " uid:" << pstApnNode->uiToUid << "Impossible error NO MEMORY total length: " << sdata.length() << " rtVal : " << n);
                return 2;
            }
            sdata.append((const char *)b, n);
        }

        if(sdata.length() == 0)
        {
            JPSLOG_FATAL("NGHTTP2-BUG for No data send to apns server,  msgId|" << pstApnNode->stPload()->_msgid << "|uid|" << pstApnNode->uiToUid
                         << "|sslid|" << m_uiSslId << "|payloadLen|" << pstApnNode->stPload()->_len << "|RemoteWindowsSize|" << m_remote_windows_size
                         << "|LocalStream|" << m_local_streams << "|RemoteStream|" << m_remote_streams);
            return 3; //can't used!
        }

        m_remote_windows_size = nghttp2_session_get_remote_window_size(m_session);
        m_stream_id = stream_id;
        int ret = SSL_write(m_pSsl, sdata.data(), sdata.length());
        int ssl_errcode = SSL_get_error(m_pSsl, ret);

        JPSLOG_TRACE("SSL_write sslId " << m_uiSslId << " ret " << ret << " : len " << sdata.length() << "  msg len " << pstApnNode->stPload()->_len
                     << " ssl_errcode " << ssl_errcode << " streamid" << m_stream_id << "msgId|" << pstApnNode->stPload()->_msgid << "|uid|" << pstApnNode->uiToUid);

        while(ret <= 0 && (SSL_ERROR_WANT_READ == ssl_errcode || SSL_ERROR_WANT_WRITE == ssl_errcode))
        {
            int err;
            int flag = 0;
            if(SSL_ERROR_WANT_READ == ssl_errcode)
            {
                flag |= EPOLLIN;
            }
            else if(SSL_ERROR_WANT_WRITE == ssl_errcode)
            {
                flag |= EPOLLOUT;
            }
            ret = epollProcess(m_iSocket, flag, m_pApnSslMng->GetSslSendTimeout());
            if(ret == 0)
            {
                err = errno;
                if(errno == 11)
                {
                    JPSLOG_FATAL("EAGAIN:Resource temporarily unavailable " << ret << " : " << err<< "msgId|" << pstApnNode->stPload()->_msgid << "|uid|" << pstApnNode->uiToUid);
                    ret = SSL_write(m_pSsl, sdata.data(), sdata.length());
                    ssl_errcode = SSL_get_error(m_pSsl, ret);
                    continue;
                }
                JPSLOG_FATAL("Ssl Write Fail epollProcess timeout errno|" << err<< "msgId|" << pstApnNode->stPload()->_msgid << "|uid|" << pstApnNode->uiToUid);
                return SSL_WRITE_SELECT_TIMEOUT;
            }
            else if(ret < 0)
            {
                err = errno;
                JPSLOG_FATAL("Ssl Write Fail epollProcess:" << ret << "| errno|" << err << "msgId|" << pstApnNode->stPload()->_msgid << "|uid|" << pstApnNode->uiToUid);
                return SSL_WRITE_SELECT_FAILED;
            }
            ret = SSL_write(m_pSsl, sdata.data(), sdata.length());
            ssl_errcode = SSL_get_error(m_pSsl, ret);
            JPSLOG_TRACE("SSL_write sslId " << m_uiSslId << " ret " << ret << " : len " << sdata.length() << "  msg len " << pstApnNode->stPload()->_len
                         << " ssl_errcode " << ssl_errcode << " error " << ERR_error_string(ERR_get_error(), NULL) << "msgId|" << pstApnNode->stPload()->_msgid << "|uid|" << pstApnNode->uiToUid);
        }

        if(ret == sdata.length())
        {
            JPSLOG_TRACE("SendEnhancedApn submit request, ssl|" << m_uiSslId << "|path|" << path << "|Stream id|" << m_stream_id << "|apns-id|"
                         << apnsid << "|apns-expiration|" << apnsexpiration << "|apns-topic|" << m_apns_topic << "|uid|" << pstApnNode->uiToUid
                         << "|msgid|" << pstApnNode->stPload()->_msgid << "|push_method|" << (int)m_push_method);

            ApnNode *pLastApnNode = NULL;
            iRet = m_pApnNodeList->GetApnByApnId(&pLastApnNode, m_uiLastSendApnId);
            if(0 == iRet)
            {
                pLastApnNode->uiNextSentApnId = pstApnNode->uiApnId;
                pstApnNode->uiPreviousSentApnId = pLastApnNode->uiApnId;
            }
            pstApnNode->uiNextSentApnId = 0;
            pstApnNode->uiSslId = m_uiSslId;
            pstApnNode->uiSentTime = pSrv->getTimeNowS();
            //m_uiLastActiveTime = uiTimeNow;
            //pstApnNode->uiSendIndex =
            ++m_uiSendCnt;
            m_uiLastSendApnId = pstApnNode->uiApnId;
            JPSLOG_TRACE("SSL send apn id " << pstApnNode->uiApnId << " succ to uid|" << pstApnNode->uiToUid << "|msgid|" << pstApnNode->stPload()->_msgid);
            ++(pstApnNode->send_times);
            pstApnNode->stream_id = m_stream_id;
            iRet = 0;
        }
        else if(ssl_errcode == SSL_ERROR_SYSCALL && 32 == errno)
        {
            //connect is closed by peer!
            JPSLOG_FATAL("SSL_write sslId " << m_uiSslId << " ret " << ret << " : len " << sdata.length() << "  msg len "
                         << pstApnNode->stPload()->_len << " uid|" << pstApnNode->uiToUid << "|msgid|" << pstApnNode->stPload()->_msgid
                         << " ssl_errcode " << ssl_errcode << " errno " << errno);
            iRet = 4;
        }
        else if(ret <= 0)
        {
            JPSLOG_FATAL("SSL_write sslId " << m_uiSslId << " ret " << ret << " : len " << sdata.length() << "  msg len " << pstApnNode->stPload()->_len
                         << " uid|" << pstApnNode->uiToUid << "|msgid|" << pstApnNode->stPload()->_msgid << " ssl_errcode " << ssl_errcode << " error "
                        << ERR_error_string(ERR_get_error(), NULL) << " errno " << errno );
            iRet = SSL_WRITE_ERROR;
        }
        else if(ret != sdata.length())
        {
            iRet = SSL_WRITE_ERROR;
            JPSLOG_INFO("SSL_write sslId " << m_uiSslId << " has send length " << ret << " : total length " << sdata.length() << "  msg len "
                        << pstApnNode->stPload()->_len << usDeviceTokenLen << " uid|" << pstApnNode->uiToUid << "|msgid|" << pstApnNode->stPload()->_msgid
                        << " ssl_errcode " << ssl_errcode << " error " << ERR_error_string(ERR_get_error(), NULL));
        }
    }

    return iRet;
}

int CApnSsl::UniqUsed()
{
    int can_used = CAN_NOT_USED;
    while(can_used == CAN_NOT_USED)
    {
        {
            IceUtil::RecMutex::Lock l(m_usedMutex);
            if(STILL_UNUSED == m_isused)
            {
                m_isused = EXCLUSION_USED;
                can_used = EXCLUSION_USED;
                break;
            }
        }

        usleep(10000);
    }
    return can_used;
}

int CApnSsl::TryUniqUsed()
{
    int can_used = CAN_NOT_USED;
    {
        IceUtil::RecMutex::TryLock l(m_usedMutex);
        if(l.acquired())
        {
            if(STILL_UNUSED == m_isused)
            {
                m_isused = EXCLUSION_USED;
                can_used = EXCLUSION_USED;
            }
        }
    }
    return can_used;
}

//non thread safe
int CApnSsl::ShareUsed()
{
    int can_used = CAN_NOT_USED;
    {
        IceUtil::RecMutex::Lock l(m_usedMutex);
        if(STILL_UNUSED == m_isused)
        {
            m_isused = SHARED_USED;
            can_used = MANAGE_SHARED_USED;
        }
        else if(SHARED_USED == m_isused)
        {
            can_used = SHARED_USED;
        }
    }
    return can_used;
}

void CApnSsl::UnUsed(int priority)
{
    if((priority & EXCLUSION_USED) || (priority & MANAGE_SHARED_USED))
    {
        IceUtil::RecMutex::Lock l(m_usedMutex);
        m_isused = STILL_UNUSED;
    }
}
