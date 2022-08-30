#pragma once

#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <list>
#include <chrono>

#include "metrics_endpoint.hh"
#include "measurements.hh"

namespace metrics
{


class MetricsReporter
{
public:
    MetricsReporter();
    ~MetricsReporter();

    void push(std::shared_ptr<Measurement> measurement);

    void register_endpoint(MetricsEndpoint& ep);

private:
    void process_loop();
    void publish_metrics();

    const int report_interval = 1000;
    std::mutex metrics_mutex;
    std::condition_variable cv;
    std::mutex push_mutex;
    std::thread metrics_thread;
    bool shutdown;
    bool push_signals;
    std::vector<std::shared_ptr<Measurement>> measurements;
    std::map<MeasurementType, std::shared_ptr<MetricsEndpoint>> metric_endpoints;
};

}
