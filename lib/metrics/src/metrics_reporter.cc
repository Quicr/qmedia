
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <string>

#include "metrics_reporter.hh"

using namespace metrics;


MetricsReporter::MetricsReporter()
{
    // Start the service thread
    metrics_thread = std::thread(&MetricsReporter::process_loop, this);
    metrics_thread.detach();
}

MetricsReporter::~MetricsReporter()
{
    shutdown = true;
    if (metrics_thread.joinable())
    {
        metrics_thread.join();
    }
}

void MetricsReporter::process_loop()
{
    std::chrono::time_point<std::chrono::steady_clock>
        current_time,                           // Current system time
        next_time;                              // Next time to emit metrics


    std::unique_lock<std::mutex> lock(push_mutex);

    // Initialize then metrics emitter time
    next_time = std::chrono::steady_clock::now() +
                std::chrono::milliseconds(report_interval);

    while (!shutdown)
    {
        cv.wait_until(lock, next_time, [&]() -> bool {
                          return shutdown;
                      });

        if(shutdown) {
            break;
        }
        lock.unlock();

        publish_metrics();

        lock.lock();
        current_time = std::chrono::steady_clock::now();
        next_time += std::chrono::milliseconds(report_interval);
        if (next_time < current_time)
        {
            next_time = current_time;
        }
    }
    std::cerr << "Metrics Thread is down" << std::endl;
}

void MetricsReporter::publish_metrics()
{
    std::unique_lock<std::mutex> lock(push_mutex);
    auto to_report = std::map<MeasurementType, std::vector<std::string>>{};
    // collect metrics
    for (const auto &measurement : measurements)
    {
        if (!to_report.count(measurement->type())) {
            to_report[measurement->type()] = {};
        }
        to_report[measurement->type()].push_back(measurement->toString());
    }

    lock.unlock();

    // publish to the endpoint
    for (const auto& [type, records] : to_report) {
        metric_endpoints.at(type)->publish(records);
    }
}

void MetricsReporter::push(std::shared_ptr<Measurement> measurement)
{
    std::lock_guard<std::mutex> lock(push_mutex);
    measurements.push_back(measurement);
}



