
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "metrics.hh"

using namespace neo_media;

const std::string MetricsConfig::URL = "http://influxdb-quicr-uswest-2.ctgpoc.com:8086";
const std::string MetricsConfig::ORG = "CTO";
const std::string MetricsConfig::BUCKET = "Media10x";
const std::string MetricsConfig::AUTH_TOKEN = "N2nuOJYyurlqYGkE-yBAPYar-iqn1G0RgQ1o3D98eZPv-k3qeNRBL51269nElFvnLtvCNmyMsVwx1p4TtKcvNA==";

Metrics::Metrics(const std::string &influx_url,
                 const std::string &org,
                 const std::string &bucket,
                 const std::string &auth_token)
{
    // don't run push thread or run curl if url not provided
    if (influx_url.empty()) return;

    // manipulate url properties for use with CURL
    std::string adjusted_url = influx_url + "/api/v2/write?org=" + org +
                               "&bucket=" + bucket + "&precision=ns";

    std::clog << "influx url:" << adjusted_url << std::endl;

    // initial curl
    curl_global_init(CURL_GLOBAL_ALL);

    // get a curl handle
    handle = curl_easy_init();

    // set the action to POST
    curl_easy_setopt(handle, CURLOPT_POST, 1L);

    // set the url
    curl_easy_setopt(handle, CURLOPT_URL, adjusted_url.c_str());

    assert(!auth_token.empty());

    curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);

    struct curl_slist *headerlist = NULL;
    std::string token_str = "Authorization: Token " + auth_token;
    headerlist = curl_slist_append(headerlist, token_str.c_str());

    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerlist);

    // verify certificate
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);

    // verify host
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);

    // do not allow unlimited redirects
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 50L);

    // enable TCP keep-alive probing
    curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);

    // Start the service thread
    metrics_thread = std::thread(&Metrics::push_loop, this);
    metrics_thread.detach();
}

Metrics::~Metrics()
{
    if (!push_signals) push();

    shutdown = true;
    if (metrics_thread.joinable())
    {
        metrics_thread.join();
    }
    curl_global_cleanup();
}


void Metrics::sendMetrics(const std::vector<std::string>& collected_metrics)
{
    CURLcode res;                      // curl response
    std::string influx_payload;        // accumulated statements
    long response_code;                // http response code

    std::cerr << "SendMetrics: count:" << collected_metrics.size() << std::endl;

    // Iterate over the vector of strings
    for (auto &statement : collected_metrics)
    {
        // Concatenate this string to the payload
        influx_payload += statement;
    }

    std::clog << "Points\n" << influx_payload << std::endl;
    // set the payload, which is a collection of influx statements
    curl_easy_setopt(
        handle, CURLOPT_POSTFIELDSIZE_LARGE, influx_payload.size());
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, influx_payload.c_str());

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

    for (const auto &mes : measurements)
    {
        auto mlines = mes.second->lineProtocol();
        std::cerr << "Measurement: " << mes.first << ", has "
                  << mlines.size() <<  " points " << std::endl;
        if(mes.first == "Jitter" || mes.first == "jitter") {
            // todo fix this
            continue ;
        }

        if (mlines.empty())
        {
            continue;
        }

        for (const auto &point : mlines)
        {
            collected_metrics.emplace_back(point);
        }
        std::cerr << "Collected metrics count: " << collected_metrics.size() << std::endl;
    }

    // release the lock
    lock.unlock();

    sendMetrics(collected_metrics);
}

void Metrics::push_loop()
{
    constexpr auto period = 1000;
    std::chrono::time_point<std::chrono::steady_clock> current_time, next_time;

    // Lock the mutex before starting work
    std::unique_lock<std::mutex> lock(metrics_mutex);

    // Initialize then metrics emitter time
    next_time = std::chrono::steady_clock::now() +
                std::chrono::milliseconds(period);

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

void Metrics::push()
{
    push_signals = true;
    cv.notify_all();
}

Metrics::MeasurementPtr Metrics::createMeasurement(std::string name,
                                                   Measurement::Tags tags)
{
    std::lock_guard<std::mutex> lock(metrics_mutex);
    auto frame = std::make_shared<Measurement>(name, tags);
    measurements.insert(std::pair<std::string, MeasurementPtr>(name, frame));
    return frame;
}

Metrics::Measurement::Measurement(std::string &name, Measurement::Tags &tags) :
    fieldIndex(0)
{
    this->name = name;
    this->tags = tags;
}

Metrics::Measurement::~Measurement()
{
}

void Metrics::Measurement::set(std::chrono::system_clock::time_point now,
                               Field field)
{
    Fields fields;
    fields.emplace_back(field);
    set(now, fields);
}

void Metrics::Measurement::set_time_entry(
    std::chrono::system_clock::time_point now,
    TimeEntry &&entry)
{
    long long time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         now.time_since_epoch())
                         .count();
    TimeSeriesEntry tse;
    tse.first = time;
    tse.second = entry;
    {
        std::lock_guard<std::mutex> lock(series_lock);
        series.emplace_back(tse);
    }
}

// we will probably need to set multiple fields in a single entry?
void Metrics::Measurement::set(std::chrono::system_clock::time_point now,
                               Fields fields)
{
    long long time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         now.time_since_epoch())
                         .count();
    time_entry entry;
    entry.first = time;
    entry.second = fields;
    {
        std::lock_guard<std::mutex> lock(series_lock);
        time_series.emplace_back(entry);
    }
}

std::string Metrics::Measurement::lineProtocol_nameAndTags()
{
    return lineProtocol_nameAndTags(tags);
}

std::string Metrics::Measurement::lineProtocol_nameAndTags(Tags &tag_set)
{
    if (name.empty()) return "";

    std::string m = name;

    if (!tag_set.empty())
    {
        for (const auto &tag : tag_set)
        {
            m += ",";
            m += tag.first;
            m += "=";
            m += std::to_string(tag.second);
        }
    }
    return m;
}

std::string Metrics::Measurement::lineProtocol_fields(Fields &fields)
{
    bool first = true;
    std::string line = " ";
    for (auto &field : fields)
    {
        if (!first)
        {
            line += ",";
        }
        line += field.first;
        line += "=";
        line += std::to_string(field.second);
        first = false;
    }

    return line;
}

std::list<std::string> Metrics::Measurement::lineProtocol()
{
    std::list<std::string> lines;
    std::string name_tags = lineProtocol_nameAndTags(tags);

    {
        std::lock_guard<std::mutex> lock(series_lock);
        if (name_tags.empty() && series.empty()) return lines;

        for (auto entry : time_series)
        {
            std::string line = name_tags;
            line += lineProtocol_fields(entry.second);
            line += " ";
            line += std::to_string(entry.first);        // time
            line += "\n";
            lines.emplace_back(line);
        }

        time_series.clear();

        // add series generated via TimeSeriesEntries
        std::for_each(
            series.begin(),
            series.end(),
            [&lines, this](auto &entry)
            {
                // gen tags
                std::string line = lineProtocol_nameAndTags(entry.second.tags);
                if (line.empty()) return;
                line += lineProtocol_fields(entry.second.fields);
                line += " ";
                line += std::to_string(entry.first);        // time
                line += "\n";
                lines.emplace_back(line);
            });
        series.clear();

    }        // lock guard

    return lines;
}
