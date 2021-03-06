#pragma once

#include "logger.hh"

namespace neo_media
{
using bytes = std::vector<uint8_t>;

class NetTransport
{
public:
    struct PeerConnectionInfo
    {
        struct sockaddr_storage addr;
        socklen_t addrLen;
        bytes transport_connection_id;        // used for quic
    };

    struct Data
    {
        std::string data;
        PeerConnectionInfo peer;
        uint64_t source_id = 0;
        bool empty() { return data.empty(); }
    };

    enum Type
    {
        QUICR
    };

    virtual bool ready() = 0;
    virtual void close() = 0;
    virtual void shutdown() = 0;
    virtual bool doSends() = 0;
    virtual bool doRecvs() = 0;
    virtual PeerConnectionInfo getConnectionInfo() = 0;
    virtual void setLogger(std::string name, const LoggerPointer &logger)
    {
        this->logger = std::make_shared<Logger>(name, logger);
    }

protected:
    LoggerPointer logger;
};

}        // namespace neo_media
