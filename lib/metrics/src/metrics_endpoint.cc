#include <vector>
#include <string>
#include <metrics_endpoint.hh>
#include <iostream>

//TODO: Add logger interface

namespace metrics {

InfluxDb::InfluxDb(const std::string &influx_url,
                   const std::string &org,
                   const std::string &bucket,
                   const std::string &auth_token)
{
    // don't run push thread or run curl if url not provided
    if (influx_url.empty()) {
        return;
    }

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
}

InfluxDb::~InfluxDb()
{
    curl_global_cleanup();
}

void InfluxDb::publish(const std::vector<std::string>& records)
{
    std::string points;
    auto print_error = true;
    std::clog << "[InfluxDb:Publish] Num records" << records.size() << std::endl;

    for (const auto &record : records)
    {
        points += record;
        if (points.empty())
        {
            continue;
        }

        std::clog << points << std::endl;
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE_LARGE, points.length());
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, points.c_str());
        CURLcode res = curl_easy_perform(handle);
        if (res != CURLE_OK && print_error)
        {
            std::clog << "Unable to post metrics:"
                      << curl_easy_strerror(res) << std::endl;
            print_error = false;
            continue;
        }

        long response_code;
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code < 200 || response_code >= 300)
        {
            std::cerr << "Http error posting to influx: " << response_code
                      << std::endl;
            break;
        }
    }
}

}

