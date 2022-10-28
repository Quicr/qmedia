#pragma once
#include <memory>

struct MetricsHelper {
    std::shared_ptr<MetricsHelper> create();

private:
    MetricsHelper(uint64_t client_id_in, uint64_t source_id_in);
    ~MetricsHelper() = default;

    uint64_t client_id;
    uint64_t source_id;
};