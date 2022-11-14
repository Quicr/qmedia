
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include <metrics/metrics.hh>
#include <metrics/measurements.hh>

// TODO: Support logger

namespace metrics
{

static std::string get_env_var(const std::string& key) {
    return std::getenv(key.c_str());
}

Metrics::Metrics(CURL* handle_in)
{
    if (!handle_in) {
        throw std::runtime_error("Invalid curl handle");
    }

    handle = handle_in;
    //metrics_thread = std::thread(&Metrics::push_loop, this);
    //metrics_thread.detach();
}

Metrics::~Metrics()
{
    shutdown = true;

    std::cerr << "Submitting Metrics before Closing Metrics" << std::endl;
    // push all the accumulated metrics
    emitMetrics();

    // TODO: make it part of RAII
    curl_global_cleanup();
}


void Metrics::add_measurement(std::shared_ptr<Measurement> measurement) {
    std::unique_lock<std::mutex> lock(metrics_mutex);
    measurements.push_back(measurement);
}

///
/// Private Implementation
///

void Metrics::sendMetrics(const std::vector<std::string>& collected_metrics)
{
    CURLcode res;               // curl response
    std::string payload;        // accumulated statements
    long response_code;         // http response code

    std::cerr << "[SendMetrics]: count:" << collected_metrics.size() << std::endl;

    // Iterate over the vector of strings
    for (auto &statement : collected_metrics)
    {
        // Concatenate this string to the payload
        payload += statement;
    }

    std::cerr << "[SendMetrics]: Payload Size: " << payload.size() << std::endl;


    curl_easy_setopt(
        handle, CURLOPT_POSTFIELDSIZE_LARGE, payload.size());
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, payload.c_str());

    // perform the request
    res = curl_easy_perform(handle);

    if (res != CURLE_OK) {
        std::clog << "[SendMetrics]: Unable to post metrics:"
                  << curl_easy_strerror(res) << std::endl;
        return;
    }

    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);

    std::cerr << "[SendMetrics]: Metrics write result: " << response_code << std::endl;

    if (response_code < 200 || response_code >= 300)
    {
        std::cerr << "[SendMetrics]: Http error posting to influx: " << response_code
                  << std::endl;
        return;
    }
}


void Metrics::emitMetrics()
{
    auto collected_metrics = std::vector<std::string>{};
    // Lock the mutex while collecting metrics data
    std::unique_lock<std::mutex> lock(metrics_mutex);

    for (const auto measurement : measurements)
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
}

///
/// factory
///

static const std::string INFLUX_URL = "INFLUX_URL";
static const std::string INFLUX_AUTH_TOKEN = "INFLUX_AUTH_TOKEN";

std::map<MetricProvider, std::shared_ptr<Metrics>> MetricsFactory::metric_providers = {};

// factory methods
std::shared_ptr<Metrics> MetricsFactory::GetInfluxProvider()
{

    if (metric_providers.count(MetricProvider::influx) > 0) {
        return metric_providers[MetricProvider::influx];
    }

    //auto influx_url = get_env_var(INFLUX_URL);
    //auto influx_auth_token = get_env_var(INFLUX_AUTH_TOKEN);
    auto influx_url = std::string{"http://relay.us-east-2.quicr.ctgpoc.com:8086"};
    auto influx_auth_token = std::string{"cisco-cto-media10x"};
    if (influx_url.empty() || influx_auth_token.empty())
    {
        return nullptr;
    }

    // manipulate url properties for use with CURL
    std::string adjusted_url = influx_url + "/api/v2/write?org=Cisco&bucket=Media10x&precision=ns";

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

