
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <string>

#include <metrics/metrics.hh>
#include <metrics/measurements.hh>

namespace metrics
{

// factory methods
std::shared_ptr<Metrics> Metrics::create(const InfluxConfig& config)
{
    if (config.url.empty() || config.auth_token.empty())
    {
        return nullptr;
    }

    // manipulate url properties for use with CURL
    std::string adjusted_url = config.url + "/api/v2/write?org=" + config.org +
                               "&bucket=" + config.bucket + "&precision=ns";

    std::clog << "influx url:" << adjusted_url << std::endl;

    // initial curl
    curl_global_init(CURL_GLOBAL_ALL);
    auto handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_POST, 1L);
    curl_easy_setopt(handle, CURLOPT_URL, adjusted_url.c_str());
    curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    struct curl_slist *headerlist = nullptr;
    std::string token_str = "Authorization: Token " + config.auth_token;
    headerlist = curl_slist_append(headerlist, token_str.c_str());
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
    // do not allow unlimited redirects
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 50L);
    // enable TCP keep-alive probing
    curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);
    // Start the service thread
    return std::make_shared<Metrics>(handle);

}


Metrics::Metrics(CURL* handle_in)
{
    handle = handle_in;
    metrics_thread = std::thread(&Metrics::pusher, this);
    metrics_thread.detach();
}


}

