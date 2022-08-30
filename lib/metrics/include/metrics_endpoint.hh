#pragma once

#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <list>
#include <chrono>
#include <measurements.hh>

#include <string>

namespace metrics {

struct MetricsEndpoint {
    virtual ~MetricsEndpoint();
    virtual void publish(const std::vector<std::string>& records) = 0;
};


class InfluxDb : MetricsEndpoint
{
public:
    // Support influx 2.0 API
    InfluxDb(const std::string &influx_url,
             const std::string &org,
             const std::string &bucket,
             const std::string &auth_token);
    ~InfluxDb();

    void publish(const std::vector<std::string>& records) override;

private:
    CURL *handle;
};

}