#pragma once

#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <list>
#include <chrono>

#include "measurements.hh"

namespace metrics
{

struct InfluxConfig
{
    static const std::string url;
    static const std::string org;
    static const std::string bucket;
    static const std::string auth_token;
};

class Metrics
{
public:
    // influxdb factory
    static std::shared_ptr<Metrics> create(const InfluxConfig& config);

    void pusher();
    void push();        // push(std::string & name) - specific push

    // created via factory
    Metrics(CURL* handle);
    ~Metrics() = default;

    bool shutdown = false;
    bool push_signals = false;
    std::mutex metrics_mutex;
    std::condition_variable cv;
    std::mutex push_mutex;
    std::thread metrics_thread;
    std::map<std::string, std::shared_ptr<Measurement>> measurements;
    // make it RAII
    CURL *handle;
};

}
