#include <cassert>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string.h>        // memcpy
#include <thread>
#include <sstream>

#if defined(__linux) || defined(__APPLE__)
#include <arpa/inet.h>
#include <netdb.h>
#endif
#if defined(__linux__)
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <net/if_dl.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "netTransportQuicR.hh"
#include "transport_manager.hh"

#include "picoquic.h"
#include "picoquic_packet_loop.h"
#include "picoquic_internal.h"
#include "picoquic_logger.h"
#include "picoquic_utils.h"
#include "picosocks.h"
#include "picotls.h"
#include "autoqlog.h"
#include "performance_log.h"

using namespace neo_media;

#define SERVER_CERT_FILE "cert.pem"
#define SERVER_KEY_FILE "key.pem"

// QuicrQ object api
#define USE_OBJECT_API 1

///
// Utility
///

static std::string to_hex(const std::vector<uint8_t> &data)
{
    std::stringstream hex(std::ios_base::out);
    hex.flags(std::ios::hex);
    for (const auto &byte : data)
    {
        hex << std::setw(2) << std::setfill('0') << int(byte);
    }
    return hex.str();
}

///
/// Quicr/Quic Stack callback handlers.
///

int transport_close_reason(picoquic_cnx_t *cnx)
{
    uint64_t last_err = 0;
    int ret = 0;
    if ((last_err = picoquic_get_local_error(cnx)) != 0)
    {
        fprintf(stdout,
                "Connection end with local error 0x%" PRIx64 ".\n",
                last_err);
        ret = -1;
    }

    if ((last_err = picoquic_get_remote_error(cnx)) != 0)
    {
        fprintf(stdout,
                "Connection end with remote error 0x%" PRIx64 ".\n",
                last_err);
        ret = -1;
    }

    if ((last_err = picoquic_get_application_error(cnx)) != 0)
    {
        fprintf(stdout,
                "Connection end with application error 0x%" PRIx64 ".\n",
                last_err);
        ret = -1;
    }

    return ret;
}

int quicrq_app_loop_cb_check_fin(TransportContext *cb_ctx)
{
    int ret = 0;

    /* if a client, exit the loop if connection is gone. */
    quicrq_cnx_ctx_t *cnx_ctx = quicrq_first_connection(cb_ctx->qr_ctx);
    if (cnx_ctx == nullptr || quicrq_is_cnx_disconnected(cnx_ctx))
    {
        ret = PICOQUIC_NO_ERROR_TERMINATE_PACKET_LOOP;
    }
    else if (!quicrq_cnx_has_stream(cnx_ctx))
    {
        // todo: don't close if no media has been posted yet
        // ret = quicrq_close_cnx(cnx_ctx);
    }

    return ret;
}

void quicrq_app_wake_up_sources(TransportContext *cb_ctx, uint64_t current_time)
{
    // wake up the source. This needs following in our case
    // 1. check the queue for the data to be present
    // 2. Somehow map/extract the source context from that info
    if (cb_ctx->transportManager->hasDataToSendToNet())
    {
        // we wake up all the sources.
        // Todo: be smart on picking up a right source to wake up
        cb_ctx->transport->wake_up_all_sources();
    }
}

void quicrq_app_check_source_time(TransportContext *cb_ctx,
                                  packet_loop_time_check_arg_t *time_check_arg)
{
    // metric: log when this function gets called as influx point
    if (cb_ctx->transportManager->hasDataToSendToNet())
    {
        time_check_arg->delta_t = 0;
        // log delta
        return;
    }
    else if (time_check_arg->delta_t > 5000)
    {
        // is this a good choice?
        time_check_arg->delta_t = 5000;
    }
    // log here delta
}

// invoked under following flows
// 1. local media subscribe as part of post
// 2. remote subcribers for this publisher
static void *media_publisher_subscribe(void *v_srce_ctx)
{
    auto pub_ctx = (PublisherContext *) v_srce_ctx;
    return pub_ctx;
}

// Callback from quicr stack
#if not defined(USE_OBJECT_API)
static int media_frame_publisher_fn(quicrq_media_source_action_enum action,
                                    void *media_ctx,
                                    uint8_t *data,
                                    size_t data_max_size,
                                    size_t *data_length,
                                    int *is_last_segment,
                                    int *is_media_finished,
                                    int *is_still_active,
                                    uint64_t current_time)
{
    int ret = 0;
    auto pub_ctx = (PublisherContext *) media_ctx;
    auto logger = pub_ctx->transport->logger;

    // log the delta for processing
    if (action == quicrq_media_source_get_data)
    {
        *is_media_finished = 0;
        *is_last_segment = 0;
        *data_length = 0;

        if (pub_ctx->transportManager->shutDown)
        {
            *is_still_active = 0;
            // todo handle gracefully.
            // todo handle media finished setting
            throw std::runtime_error("can't publish data, transport manager is "
                                     "down");
        }

        *data_length = pub_ctx->transportManager->hasDataToSendToNet();
        if (*data_length > data_max_size)
        {
            logger->debug << "Transport Buffer Small: transport buffer size="
                          << data_max_size << ", "
                          << "data size=" << *data_length << std::flush;
            *data_length = 0;
            *is_still_active = 1;
            return 0;
        }
        if (data != nullptr)
        {
            NetTransport::Data send_packet;
            NetTransport::PeerConnectionInfo peer_info;
            send_packet.data.resize(*data_length);
            auto got = pub_ctx->transportManager->getDataToSendToNet(
                send_packet.data, &send_packet.peer, &send_packet.peer.addrLen);
            if (got)
            {
                logger->debug
                    << "Copied data to the quicr transport:" << *data_length
                    << std::flush;
                std::copy(
                    send_packet.data.begin(), send_packet.data.end(), data);
                *is_last_segment = 1;
                *is_still_active = 1;
            }
            else
            {
                *is_still_active = 0;
            }
        }
        else
        {
            *is_last_segment = 1;
        }
        ret = 0;
    }
    else if (action == quicrq_media_source_close)
    {
        /* todo close the context */
    }
    // delta end
    return ret;
}
#endif

static int
media_consumer_frame_ready(void *media_ctx,
                           uint64_t current_time,
                           uint64_t frame_id,
                           const uint8_t *data,
                           size_t data_length,
                           quicrq_reassembly_object_mode_enum frame_mode)
{
    int ret = 0;
    auto *cons_ctx = (ConsumerContext *) media_ctx;
    auto logger = cons_ctx->transport->logger;

    logger->debug << "[frame_ready: id:" << frame_id
                  << ", frame_mode:" << (int) frame_mode
                  << ",data_len:" << data_length << "]" << std::flush;

    if (frame_mode ==
        quicrq_reassembly_object_mode_enum::quicrq_reassembly_object_peek)
    {
        logger->debug << "[frame_ready:quicrq_reassembly_frame_peek, ignoring"
                      << std::flush;
        return 0;
    }

    // log the delta until the frame is handed over to the app
    struct sockaddr_storage stored_addr;
    struct sockaddr *peer_addr = nullptr;
    quicrq_get_peer_address(cons_ctx->cnx_ctx, &stored_addr);

    NetTransport::PeerConnectionInfo peer_info;
    // TODO: support IPV6
    memcpy(&peer_info.addr, (sockaddr *) &stored_addr, sizeof(sockaddr_in));
    peer_info.addrLen = sizeof(struct sockaddr_storage);
    bytes cnx_id_bytes = {};
    peer_info.transport_connection_id = std::move(cnx_id_bytes);
    auto recv_data = std::string(data, data + data_length);
    cons_ctx->transportManager->recvDataFromNet(recv_data,
                                                std::move(peer_info));
    return ret;
}

static int media_datagram_input(void *media_ctx,
                                uint64_t current_time,
                                const uint8_t *data,
                                uint64_t frame_id,
                                uint64_t offset,
                                int is_last_segment,
                                size_t data_length)
{
    ConsumerContext *cons_ctx = (ConsumerContext *) media_ctx;
    return 0;
}

int media_consumer_learn_final_frame_id(void *media_ctx,
                                        uint64_t final_frame_id)
{
    int ret = 0;
    auto *cons_ctx = (ConsumerContext *) media_ctx;
    //ret = quicrq_reassembly_learn_final_object_id(&cons_ctx->reassembly_ctx,
    //                                              final_frame_id);
    return ret;
}

int media_consumer_fn(quicrq_media_consumer_enum action,
                      void *media_ctx,
                      uint64_t current_time,
                      const uint8_t *data,
                      uint64_t frame_id,
                      uint64_t offset,
                      uint64_t queue_delay,
                      int is_last_segment,
                      size_t data_length)
{
    int ret = 0;
    auto *cons_ctx = (ConsumerContext *) media_ctx;

    switch (action)
    {
        case quicrq_media_datagram_ready:
            ret = media_datagram_input(media_ctx,
                                       current_time,
                                       data,
                                       frame_id,
                                       (size_t) offset,
                                       is_last_segment,
                                       data_length);

            if (ret == 0 && cons_ctx->reassembly_ctx.is_finished)
            {
                ret = quicrq_consumer_finished;
            }
            break;
        case quicrq_media_final_object_id:
            media_consumer_learn_final_frame_id(media_ctx, frame_id);
            if (ret == 0 && cons_ctx->reassembly_ctx.is_finished)
            {
                ret = quicrq_consumer_finished;
            }
            break;
        case quicrq_media_close:
            // todo cleanup
            break;
        default:
            ret = -1;
            break;
    }
    return ret;
}

// media consumer object callback from quicr stack
int object_stream_consumer_fn(
    quicrq_media_consumer_enum action,
    void *object_consumer_ctx,
    uint64_t current_time,
    uint64_t group_id,
    uint64_t object_id,
    const uint8_t *data,
    size_t data_length,
    quicrq_object_stream_consumer_properties_t *properties)
{
    auto cons_ctx = (ConsumerContext *) object_consumer_ctx;
    auto &logger = cons_ctx->transport->logger;
    logger->debug << cons_ctx->url
                 << ": object_stream_consumer_fn: action:" << (int) action
                 << ",data_length:" << data_length << std::flush;
    int ret = 0;
    switch (action)
    {
        case quicrq_media_datagram_ready:
        {
            // logger->info << "quicrq_media_datagram_ready, object:" <<
            // object_id
            //              << std::flush;
            struct sockaddr_storage stored_addr;
            struct sockaddr *peer_addr = nullptr;
            quicrq_get_peer_address(cons_ctx->cnx_ctx, &stored_addr);
            NetTransport::PeerConnectionInfo peer_info;
            // TODO: support IPV6
            memcpy(&peer_info.addr,
                   (sockaddr *) &stored_addr,
                   sizeof(sockaddr_in));
            peer_info.addrLen = sizeof(struct sockaddr_storage);
            bytes cnx_id_bytes = {};
            peer_info.transport_connection_id = std::move(cnx_id_bytes);
            auto recv_data = std::string(data, data + data_length);
            cons_ctx->transportManager->recvDataFromNet(recv_data,
                                                        std::move(peer_info));
        }
        break;
        case quicrq_media_close:
            /* Remove the reference to the media context, as the caller will
             * free it. */
            cons_ctx->object_consumer_ctx = nullptr;
            /* Close streams and other resource */
            assert(0);
            break;
        default:
            ret = -1;
            break;
    }

    return ret;
}

// main packet loop for the application
int quicrq_app_loop_cb(picoquic_quic_t *quic,
                       picoquic_packet_loop_cb_enum cb_mode,
                       void *callback_ctx,
                       void *callback_arg)
{
    int ret = 0;
    auto *cb_ctx = (TransportContext *) callback_ctx;
    auto &logger = cb_ctx->transport->logger;

    if (cb_ctx == nullptr)
    {
        return PICOQUIC_ERROR_UNEXPECTED_ERROR;
    }
    else
    {
        switch (cb_mode)
        {
            case picoquic_packet_loop_ready:
                if (callback_arg != nullptr)
                {
                    auto *options = (picoquic_packet_loop_options_t *)
                        callback_arg;
                    options->do_time_check = 1;
                }
                if (cb_ctx->transport)
                {
                    std::lock_guard<std::mutex> lock(
                        cb_ctx->transport->quicConnectionReadyMutex);
                    cb_ctx->transport->quicConnectionReady = true;
                    auto cnx = picoquic_get_first_cnx(quic);
                    // save the connection information
                    auto cnx_id = picoquic_get_client_cnxid(cnx);
                    auto cnx_id_bytes = bytes(cnx_id.id,
                                              cnx_id.id + cnx_id.id_len);
                    if (cnx->client_mode)
                    {
                        cb_ctx->transport->local_connection_id = std::move(
                            cnx_id_bytes);
                    }
                }
                ret = 0;
                break;
            case picoquic_packet_loop_after_receive:
                /* Post receive callback */
                ret = quicrq_app_loop_cb_check_fin(cb_ctx);
                break;
            case picoquic_packet_loop_after_send:
                /* Post send callback. Check whether sources need to be awakened
                 */
#if not defined(USE_OBJECT_API)
                quicrq_app_wake_up_sources(cb_ctx,
                                           picoquic_get_quic_time(quic));
#endif
                /* if a client, exit the loop if connection is gone. */
                ret = quicrq_app_loop_cb_check_fin(cb_ctx);
                break;
            case picoquic_packet_loop_port_update:
                break;
            case picoquic_packet_loop_time_check:
            {
                /* check local test sources */
                quicrq_app_check_source_time(
                    cb_ctx, (packet_loop_time_check_arg_t *) callback_arg);

#if defined(USE_OBJECT_API)
                NetTransport::Data send_packet;
                auto got = cb_ctx->transportManager->getDataToSendToNet(
                    send_packet);
                if (!got || send_packet.empty())
                {
                    break;
                }
                if (!send_packet.source_id)
                {
                    break;
                }
                auto &publish_ctx = cb_ctx->transport->get_publisher_context(
                    send_packet.source_id);
                assert(publish_ctx.object_source_ctx);
                uint64_t group_id = 0;
                uint64_t object_id = 0;
                ret = quicrq_publish_object(
                    publish_ctx.object_source_ctx,
                    reinterpret_cast<uint8_t *>(send_packet.data.data()),
                    send_packet.data.size(),
                    1,
                    nullptr,
                    &group_id,
                    &object_id);
                assert(ret == 0);
#endif
            }
            break;
            default:
                ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
                break;
        }
    }

    return ret;
}

NetTransportQUICR::~NetTransportQUICR()
{
    close();
}

void NetTransportQUICR::close()
{
    quicrq_delete(quicr_ctx);
}

bool NetTransportQUICR::doRecvs()
{
    return false;
}

bool NetTransportQUICR::doSends()
{
    return false;
}

void NetTransportQUICR::wake_up_all_sources()
{
    for (const auto &ctx : publishers)
    {
        logger->debug << "[W]";
        //quicrq_source_wakeup(ctx.second.source_ctx);
    }
}

void NetTransportQUICR::publish(uint64_t source_id,
                                Packet::MediaType media_type,
                                const std::string &url)
{
    if (!quicr_ctx)
    {
        throw std::runtime_error("quicr context is empty\n");
    }

    auto pub_context = new PublisherContext{
        source_id, media_type, url, nullptr, transportManager, this};

#if defined(USE_OBJECT_API)
    // TODO: Set object source property
    auto obj_src_context = quicrq_publish_object_source(
        quicr_ctx,
        reinterpret_cast<uint8_t *>(const_cast<char *>(url.data())),
        url.length(),
        nullptr);
    assert(obj_src_context);
    pub_context->object_source_ctx = obj_src_context;
    // enable publishing
    auto ret = quicrq_cnx_post_media(
        cnx_ctx,
        reinterpret_cast<uint8_t *>(const_cast<char *>(url.data())),
        url.length(),
        true);
    assert(ret == 0);
#else
    quicrq_media_source_ctx_t *src_ctx = nullptr;
    src_ctx = quicrq_publish_source(
        quicr_ctx,
        reinterpret_cast<uint8_t *>(const_cast<char *>(url.data())),
        url.length(),
        pub_context,
        media_publisher_subscribe,
        media_frame_publisher_fn,
        [](void *pub_ctx) { free(pub_ctx); });
    assert(src_ctx);
    // save the source
    pub_context->source_ctx = src_ctx;
    // enable publishing
    auto ret = quicrq_cnx_post_media(
        cnx_ctx,
        reinterpret_cast<uint8_t *>(const_cast<char *>(url.data())),
        url.length(),
        true);
    assert(ret == 0);
#endif
    logger->info << "Added source [" << source_id << " Url: " << url << "]"
                 << std::flush;
    publishers[source_id] = *pub_context;
}

void NetTransportQUICR::remove_source(uint64_t source_id)
{
#if defined(USE_OBJECT_API)
    auto src_ctx = publishers[source_id];
    if (src_ctx.object_source_ctx)
    {
        quicrq_publish_object_fin(src_ctx.object_source_ctx);
        quicrq_delete_object_source(src_ctx.object_source_ctx);
        logger->info << "Removed source [" << source_id << std::flush;
    }
#else
    throw std::runtime_error("unimplemented");
#endif
}

void NetTransportQUICR::subscribe(uint64_t source_id,
                                  Packet::MediaType media_type,
                                  const std::string &url)
{
    auto consumer_media_ctx = new ConsumerContext{};
    memset(consumer_media_ctx, 0, sizeof(ConsumerContext));
    consumer_media_ctx->media_type = media_type;
    consumer_media_ctx->url = url;
    consumer_media_ctx->transport = this;
    consumer_media_ctx->transportManager = transportManager;
    consumer_media_ctx->cnx_ctx = cnx_ctx;
#if defined(USE_OBJECT_API)
    constexpr auto use_datagram = true;
    constexpr auto in_order = true;
    consumer_media_ctx->object_consumer_ctx = quicrq_subscribe_object_stream(
        cnx_ctx,
        reinterpret_cast<uint8_t *>(const_cast<char *>(url.data())),
        url.length(),
        use_datagram,
        in_order,
        object_stream_consumer_fn,
        consumer_media_ctx);
    assert(consumer_media_ctx->object_consumer_ctx);
#else
    quicrq_reassembly_init(&consumer_media_ctx->reassembly_ctx);

    auto ret = quicrq_cnx_subscribe_media(
        cnx_ctx,
        reinterpret_cast<uint8_t *>(const_cast<char *>(url.data())),
        url.length(),
        true,
        media_consumer_fn,
        consumer_media_ctx);
#endif
    consumers[source_id] = *consumer_media_ctx;
    logger->info << "Subscriber URL:" << url << std::flush;
}

NetTransportQUICR::NetTransportQUICR(TransportManager *t,
                                     std::string sfuName,
                                     uint16_t sfuPort,
                                     const LoggerPointer &logger_in) :
    transportManager(t),
    quicConnectionReady(false),
    quicr_ctx(quicrq_create_empty()),
    logger(logger_in)
{
    logger->info << "Quicr Client Transport" << std::flush;
    picoquic_config_init(&config);
    picoquic_config_set_option(&config, picoquic_option_ALPN, QUICRQ_ALPN);
    debug_set_stream(stdout);
    quic = picoquic_create_and_configure(
        &config, quicrq_callback, quicr_ctx, picoquic_current_time(), NULL);

    if (!quic)
    {
        throw std::runtime_error("unable to create picoquic context");
    }

    logger->info << "Created QUIC handle" << std::flush;
    picoquic_set_key_log_file_from_env(quic);

    picoquic_set_mtu_max(quic, config.mtu_max);
    config.qlog_dir = "/Users/snandaku/Downloads/logs";
    if (config.qlog_dir != NULL)
    {
        picoquic_set_qlog(quic, config.qlog_dir);
    }

    if (config.performance_log != NULL)
    {
        picoquic_perflog_setup(quic, config.performance_log);
    }

    quicrq_set_quic(quicr_ctx, quic);

    struct sockaddr_storage addr = {0};
    int is_name = 0;
    char const *sni = NULL;

    int ret = picoquic_get_server_address(
        sfuName.c_str(), sfuPort, &addr, &is_name);
    if (ret != 0)
    {
        throw std::runtime_error("Cannot find the servr address");
    }
    else if (is_name != 0)
    {
        sni = sfuName.c_str();
    }

    if ((cnx_ctx = quicrq_create_client_cnx(
             quicr_ctx, sni, (struct sockaddr *) &addr)) == NULL)
    {
        throw std::runtime_error("cannot create connection to the server");
    }

    xport_ctx.transport = this;
    xport_ctx.transportManager = transportManager;
    xport_ctx.qr_ctx = quicr_ctx;
    xport_ctx.cn_ctx = cnx_ctx;
    xport_ctx.port = sfuPort;

    quicr_client_ctx.port = sfuPort;
    memcpy(&quicr_client_ctx.server_address, &addr, addr.ss_len);
    quicr_client_ctx.server_address_len = sizeof(
        quicr_client_ctx.server_address);
    assert(ret == 0);
}

void NetTransportQUICR::start()
{
    quicTransportThread = std::thread(quicTransportThreadFunc, this);
}

bool NetTransportQUICR::ready()
{
    bool ret;
    {
        std::lock_guard<std::mutex> lock(quicConnectionReadyMutex);
        ret = quicConnectionReady;
    }
    if (ret)
    {
        logger->info << "NetTransportQUICR::ready()" << std::flush;
    }
    return ret;
}

// Main quic process thread
// 1. check for incoming packets
// 2. check for outgoing packets
int NetTransportQUICR::runQuicProcess()
{
    // run the packet loop
    int ret = picoquic_packet_loop(quic,
                                   0,
                                   0,
                                   config.dest_if,
                                   config.socket_buffer_size,
                                   config.do_not_use_gso,
                                   quicrq_app_loop_cb,
                                   &xport_ctx);

    logger->info << "Quicr loop Done " << std::flush;
    /* free all the media sources */
    // quicrq_app_free_sources(&quicr_client_context);
    /* Free the quicrq context */
    quicrq_delete(quicr_client_ctx.qr_ctx);
    return 0;
}