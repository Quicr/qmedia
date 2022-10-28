
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <metrics/metrics.hh>
#include <metrics/measurements.hh>

namespace metrics
{

Metrics::Metrics(CURL* handle_in)
{
    handle = handle_in;
    metrics_thread = std::thread(&Metrics::push_loop, this);
    metrics_thread.detach();
}

Metrics::~Metrics()
{
    shutdown = true;
    if (metrics_thread.joinable()) {
        metrics_thread.join();
    }

    curl_global_cleanup();
}


void Metrics::add_measurement(const std::string& name, std::shared_ptr<Measurement> measurement) {
    measurements.insert(std::pair<std::string, std::shared_ptr<Measurement>>(name, measurement));
}

void Metrics::sendMetrics(const std::vector<std::string>& collected_metrics)
{
    CURLcode res;               // curl response
    std::string payload;        // accumulated statements
    long response_code;         // http response code

    //std::cerr << "SendMetrics: count:" << collected_metrics.size() << std::endl;

    // Iterate over the vector of strings
    for (auto &statement : collected_metrics)
    {
        // Concatenate this string to the payload
        payload += statement;
    }

    std::cerr << "[metrics]: " << payload << std::endl;


    // std::clog << "Points\n" << influx_payload << std::endl;
    // set the payload, which is a collection of influx statements
    curl_easy_setopt(
        handle, CURLOPT_POSTFIELDSIZE_LARGE, payload.size());
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, payload.c_str());

    // perform the request
    res = curl_easy_perform(handle);

    if (res != CURLE_OK) {
        std::clog << "Unable to post metrics:"
                  << curl_easy_strerror(res) << std::endl;
        return;
    }

    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);

    std::cerr << "Metrics write result: " << response_code << std::endl;

    if (response_code < 200 || response_code >= 300)
    {
        std::cerr << "Http error posting to influx: " << response_code
                  << std::endl;
        return;
    }
}


void Metrics::emitMetrics()
{
    auto collected_metrics = std::vector<std::string>{};

    // Lock the mutex while collecting metrics data
    std::unique_lock<std::mutex> lock(metrics_mutex);

    for (const auto &[type, measurement] : measurements)
    {
        auto points = measurement->serialize();

        if (points.empty())
        {
            continue;
        }

        collected_metrics.emplace_back(points);
    }

    // release the lock
    lock.unlock();

    sendMetrics(collected_metrics);
}

void Metrics::push_loop()
{
    constexpr auto period = 1000;
    std::chrono::time_point<std::chrono::steady_clock> next_time;

    // Lock the mutex before starting work
    std::unique_lock<std::mutex> lock(metrics_mutex);

    // Initialize then metrics emitter time
    next_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(period);

    while (!shutdown)
    {
        // Wait until alerted
        cv.wait_until(lock, next_time, [&]() { return shutdown; });

        // Were we told to terminate?
        if (shutdown) {
            std::clog << "Metrics is shutdown " << std::flush;
            break;
        }

        // unlock the mutex
        lock.unlock();

        emitMetrics();

        // Re-lock the mutex
        lock.lock();

        next_time = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(period);
    }
}

///
/// factory
///

static const std::string influx_url = "";
static const std::string influx_auth_token = "";
static const std::string influx_org = "";
static const std::string influx_bucket = "";

std::map<MetricProvider, std::shared_ptr<Metrics>> MetricsFactory::metric_providers = {};

// factory methods
std::shared_ptr<Metrics> MetricsFactory::GetInfluxProvider()
{
    if (metric_providers.count(MetricProvider::influx) > 0) {
        return metric_providers[MetricProvider::influx];
    }

    if (influx_url.empty() || influx_auth_token.empty())
    {
        return nullptr;
    }

    // manipulate url properties for use with CURL
    std::string adjusted_url = influx_url + "/api/v2/write?org=" + influx_org +
                               "&bucket=" + influx_bucket + "&precision=ns";

    std::clog << "influx url:" << adjusted_url << std::endl;

    // initial curl
    curl_global_init(CURL_GLOBAL_ALL);
    auto handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_POST, 1L);
    curl_easy_setopt(handle, CURLOPT_URL, adjusted_url.c_str());
    curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    struct curl_slist *headerlist = nullptr;
    std::string token_str = "Authorization: Token " + influx_auth_token;
    headerlist = curl_slist_append(headerlist, token_str.c_str());
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
    // do not allow unlimited redirects
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 50L);
    // enable TCP keep-alive probing
    curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);
    metric_providers[MetricProvider::influx] = std::make_shared<Metrics>(handle);
    return metric_providers[MetricProvider::influx];
}

}

